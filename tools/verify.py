"""Compare compiled solutions with an unmodified official checkout.

python3 tools/verify.py --official ../official --scale sample --seeds 1 7
Build each main.cpp to solution (or solution.exe on Windows) first.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument("--official",type=Path,default=Path("../official"))
parser.add_argument("--scale",choices=["tiny","sample","full"],default="tiny")
parser.add_argument("--seeds",type=int,nargs="+",default=[1,7,42])
args=parser.parse_args()
root=Path(__file__).resolve().parent.parent
official=args.official.resolve()
rows=[]
tracks={"a":("a_ingest","summary.txt"),"c":("c_join","aggregates.txt"),"d":("d_traverse","distances.txt")}
for track,(module,filename) in tracks.items():
    for seed in args.seeds:
        data=root/".validation"/f"{track}-{args.scale}-{seed}"
        ref,actual=data/"reference",data/"actual"
        if not data.exists():
            subprocess.run([sys.executable,str(official/"generators"/f"{module}.py"),"--seed",str(seed),"--scale",args.scale,"--out",str(data)],check=True)
        ref.mkdir(parents=True,exist_ok=True)
        start=time.perf_counter()
        subprocess.run([sys.executable,str(official/"reference"/f"{module}.py"),"--data",str(data),"--out",str(ref)],check=True)
        reference_seconds=time.perf_counter()-start
        binary=root/f"track-{track}"/("solution.exe" if sys.platform=="win32" else "solution")
        start=time.perf_counter()
        subprocess.run([str(binary),str(data),str(actual)],check=True)
        seconds=time.perf_counter()-start
        result=(actual/filename).read_bytes()
        assert result==(ref/filename).read_bytes(),(track,seed,args.scale,"output mismatch")
        row=dict(track=track,seed=seed,scale=args.scale,sha256=hashlib.sha256(result).hexdigest(),reference_seconds=reference_seconds,solution_seconds=seconds)
        rows.append(row)
        (root/".validation"/"last-run.json").write_text(json.dumps(rows,indent=2)+"\n")
        print(json.dumps(row),flush=True)
