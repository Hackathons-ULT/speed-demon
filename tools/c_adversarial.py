"""Compact Track C differential checks for partitioning and concurrent names.

Usage: python tools/c_adversarial.py TEAM_ROOT OFFICIAL_ROOT [--compiler g++]
The normal solution must already be built. All generated files stay below
TEAM_ROOT/.validation/c-adversarial, or the explicit --work-dir.
"""

import argparse
import csv
import importlib.util
import os
from pathlib import Path
import subprocess
import time


BOM = b"\xef\xbb\xbf"
HEADER = BOM + b"id,name,note,region_id,amount\r\n"
REGIONS = b"region_id,country\n1,AAA\n3,BBB\n7,AAA\n999999,ZZZ\n"


def letters(value):
    result = b""
    while True:
        result = bytes([65 + value % 26]) + result
        value = value // 26 - 1
        if value < 0:
            return result


def field(value, quoted):
    if quoted:
        return b'"' + value.replace(b'"', b'""') + b'"'
    return value


def records(rows):
    result = bytearray(HEADER)
    for index, (name, note, region) in enumerate(rows, 1):
        if index > 1 and index % 17 == 0:
            result.extend(BOM)
        result.extend(
            str(index).encode() + b"," + field(name, bool(name) or index % 2 == 0)
            + b"," + field(note, bool(note) or index % 3 == 0) + b","
            + str(region).encode() + (b",-0.01" if index % 3 else b",99999.99")
        )
        result.extend(b"\r\n" if index % 2 else b"\n")
    return bytes(result)


def fixtures():
    # Capacity and probe limits are performance choices, never input limits.
    names = [b"Name" + letters(i) for i in range(150001)]
    high = [(name, b'quote" and\nline', 1 if i % 2 else 3)
            for i, name in enumerate(names)]
    high.extend((name, b"", 7) for name in names[::2])
    yield "high-distinct", records(high), False

    # These generic 2.1 values deliberately exceed the generator's note shape.
    # Numeric-looking rows within notes must never become partition starts.
    fake = b'first\n123,"fake","row",1,1.00\nlast'
    boundary_rows = [
        (b'"' * 70000, fake, 1),
        (b',"', b'",', 1),
        (b'Alpha' + b' ' * 65537, b'"' * 70001 + fake, 3),
        (b'', b'', 7),
        (b'caf\xc3\xa9' + BOM, b'x\r\ny' + BOM, 9999999),
    ] * 3
    boundary = records(boundary_rows)
    yield "anchor-boundaries", boundary[:-2], False  # EOF without terminator

    # No odd quote run can anchor this file; the exact fallback is required.
    empty = bytearray(HEADER)
    for i in range(1, 7001):
        empty.extend(f"{i},,,1,-0.01\r\n".encode())
    yield "no-anchor", bytes(empty), False

    colliding = [(b"Name" + letters(i % 1200), b'"note\nline"',
                  1 if i % 2 else 3) for i in range(12000)]
    colliding.extend((b"", b"", 7) for _ in range(4000))
    yield "forced-collisions-hot-empty", records(colliding), True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("team_root", type=Path)
    parser.add_argument("official_root", type=Path)
    parser.add_argument("--binary", type=Path)
    parser.add_argument("--collision-binary", type=Path)
    parser.add_argument("--compiler", default=os.environ.get("CXX", "g++"))
    parser.add_argument("--work-dir", type=Path)
    args = parser.parse_args()
    root, official = args.team_root.resolve(), args.official_root.resolve()
    work = (args.work_dir or root / ".validation/c-adversarial").resolve()
    work.mkdir(parents=True, exist_ok=True)
    suffix = ".exe" if os.name == "nt" else ""
    binary = (args.binary or root / "track-c" / ("solution" + suffix)).resolve()
    collision = (args.collision_binary or work / ("collisions" + suffix)).resolve()
    started = time.perf_counter()
    if args.collision_binary is None:
        source = root / "track-c/main.cpp"
        if "C_FORCE_COLLISIONS" not in source.read_text(encoding="utf-8"):
            raise RuntimeError("Track C lacks the forced-collision validation hook")
        subprocess.run([args.compiler, "-std=c++17", "-O3", "-DNDEBUG", "-static", "-pthread",
                        "-DC_FORCE_COLLISIONS", str(source), "-o", str(collision)],
                       check=True)
    spec = importlib.util.spec_from_file_location("c_adversarial_reference",
                                                official / "reference/c_join.py")
    reference = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(reference)
    csv.field_size_limit(1 << 24)
    checks = 0
    for name, content, force in fixtures():
        data = work / name
        data.mkdir(parents=True, exist_ok=True)
        (data / "records.csv").write_bytes(content)
        (data / "regions.csv").write_bytes(REGIONS)
        reference.main(str(data), str(data / "reference"))
        expected = (data / "reference/aggregates.txt").read_bytes()
        for workers in (1, 12):
            env = dict(os.environ, C_WORKERS=str(workers))
            out = data / f"actual-{workers}"
            subprocess.run([str(collision if force else binary), str(data), str(out)],
                           env=env, check=True, timeout=30)
            actual = (out / "aggregates.txt").read_bytes()
            if actual != expected:
                raise AssertionError(f"{name}, C_WORKERS={workers}: output bytes differ")
            checks += 1
        print(f"PASS C {name} (workers 1,12)", flush=True)
    print(f"PASS C adversarial: {checks} checks in {time.perf_counter()-started:.3f}s",
          flush=True)


if __name__ == "__main__":
    main()
