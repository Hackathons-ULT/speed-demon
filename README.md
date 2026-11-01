# Injector Bind: Speed Demon 2026

The team submission is the `speed-demon` branch of https://github.com/Hackathons-ULT/speed-demon. The current candidates implement all four tracks in zero-dependency C++17: A Ingest, B Index, C Join and D Traverse. The original organiser references and earlier teammate work are preserved in Git history; the Python Track A experiment is archived separately.

Each track is self-contained. It reads `/data`, writes exactly the required file to `/out`, and exits. Dockerfiles compile offline using allowlisted, digest-pinned GCC and Debian images. There are no precomputed answers, input recognition, network calls, or runtime packages.

| Track | Inputs | Output | Approach |
|---|---|---|---|
| A Ingest | `requests.log` | `summary.txt` | Map input read-only, intern endpoint views, use the specified 72 UTC-hour buckets, count and scatter 32-bit latencies with six line-aligned workers, select exact nearest ranks, and write bounded integer-formatted output. |
| B Index | `docs.txt`, `queries.txt` | `results.txt` | Read queries first; tokenize ASCII bytes once per corpus range, build only needed BOOL postings, and match all phrases with a shared token-level Aho-Corasick automaton. Apply exact left-associative set operations and emit count, ID sum and ten smallest IDs. |
| C Join | `records.csv`, `regions.csv` | `aggregates.txt` | A quote-parity scan creates safe ranges for six CSV parsers. Direct region lookup, exact shared name interning and atomic country bitmaps feed private integer-cent aggregates. |
| D Traverse | `graph.txt`, `queries.txt` | `distances.txt` | Build BFS-reordered forward/reverse CSR arrays and run bidirectional Dijkstra with radix queues, heap-count balancing, private epoch labels and four query workers. Preserve parallel edges, zero weights and query order. |

## Verified live Board results

The official Board scored all four tracks at commit `3c2fd576c90e3396bcdc4d04ce8677021436188d` on November 1. All check, build, correctness, timeout and sandbox gates passed. The Track A lifetime change and independent adversarial fixture passed expanded Linux CI and offline container execution: [CI 41315064433](https://github.com/Hackathons-ULT/speed-demon/actions/runs/41315064433).

| Track | Run ID | Time (s) | Peak memory (MB, decimal) | K | Machine points |
|---|---|---:|---:|---:|---:|
| A | `run-5203d23e4e97` | 0.122014 | 29.03 | 19.450919 | 64.447006 |
| B | `run-605129b1a988` | 1.436493 | 230.16 | 7.849115 | 44.741033 |
| C | `run-8010ef37f93a` | 0.185159 | 13.18 | 14.109089 | 57.474948 |
| D | `run-0948b9b60deb` | 0.887435 | 272.69 | 8.927937 | 47.537555 |

A's two-pass parser improved from 0.249200 s / K 13.524502 to 0.100145 s / K 19.909976. The subsequent lifetime change measured 0.122014 s, 29.03 MB and K 19.450919, a lower first-run coefficient despite lower memory. A repeat comparison is pending before retention. The code for B/C/D is unchanged since `ee4c7f9`; differences in their latest practice times are run variation.

The earlier four-agent batch raised best-three machine points from 48.541813 to 56.795715. These are practice machine scores; holdout data and human judging determine final results. Exact receipts and earlier baselines remain in `board-results.json`, `board-results-history.json` and each PERF.md. All four published full seed-1 hashes match. B/D alternative experiments remain isolated while current work focuses on A.

## Build and validate

```sh
g++ -std=c++17 -O3 -DNDEBUG -static -pthread track-a/main.cpp -o track-a/solution
g++ -std=c++17 -O3 -DNDEBUG -static -pthread track-b/main.cpp -o track-b/solution
g++ -std=c++17 -O3 -DNDEBUG -static -pthread track-c/main.cpp -o track-c/solution
g++ -std=c++17 -O3 -DNDEBUG -static -pthread track-d/main.cpp -o track-d/solution
git clone https://github.com/hackathon-raptors/speed-demon-2026 ../official
git -C ../official checkout --detach 158894dcefde61f459c0adfae841900458dbeb7f
python3 tools/edge_cases.py ../official
python3 tools/verify.py --official ../official --scale sample --seeds 1 7
```

Each executable accepts optional data and output paths for local checks. With no arguments it uses the container paths. The retained organiser `check.py` runs offline builds and the sandbox, for example `python3 check.py c --data data/c --expected EXPECTED_SHA256`. CI also checks Linux builds, multiple seeds, fixtures, and offline container execution. Local Windows validation is recorded in `validation.json`; it is not an official Board score.

For a full seed-1 container check, first generate inputs (Python 3.11 or newer; Docker required for `check.py`):

```sh
python3 ../official/generators/a_ingest.py --seed 1 --scale full --out data/a
python3 ../official/generators/b_index.py --seed 1 --scale full --out data/b
python3 ../official/generators/c_join.py --seed 1 --scale full --out data/c
python3 ../official/generators/d_traverse.py --seed 1 --scale full --out data/d
python3 check.py a --data data/a --expected 495ca09682b9673ef836a506496177680bb4a3af54255174befddb1f83481cd3
python3 check.py b --data data/b --expected f86973c210228141bcb6f1f1b10188ad2a9b65f71e7cd7cf4590b38de4e80927
python3 check.py c --data data/c --expected d8088b1cd42ba1338b1345b107b55db7b96ef1f5066f3ff64fd26a8912a0f536
python3 check.py d --data data/d --expected a23dbdf0a3e49cc8901310a9ee1dc6a9a569a85297b6da75c2a7002cb2ed277e
```

These are the organiser's published full-output hashes. The candidates match all four published full-size output hashes, with reference comparisons and additional seed/edge-case tests described in each PERF.md. Sample and full use different records, so use each dataset's own reference hash when changing scale or seed. Only the official scoring machine determines competition scores.

Soorya Dev's earlier scored Python Track A is preserved byte for byte under `experiments/track-a-python/` and in Git history. Its actual Board results remain in `track-a/PERF.md`. C++ changes were written and tested with Codex assistance during the event. Read each track's README.md and PERF.md for its algorithm, correctness argument, measurements, dependencies, and proposed next experiments. Human understanding has not been asserted by the agent.

All human registrations are confirmed by the entrant. The Board watches this repository under the confirmed team name "Injector Bind". No new team-form response is needed to change the tracked branch contents. Keep the Board App installed through freeze.

Code freeze is November 2, 2026 at 15:00 UTC. The repository must be public under its MIT licence by 16:00 UTC. Repository visibility has not been changed. Current rules: https://speeddemon.dev.
