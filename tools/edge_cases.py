import heapq
import importlib.util
from pathlib import Path
import random
import subprocess
import sys
import unittest

root = Path(__file__).resolve().parent.parent
official=Path(sys.argv[1] if len(sys.argv)>1 else "../official").resolve()
def run(track, name, files, expected):
    data=root/".validation"/"edge-data"/name
    data.mkdir(parents=True, exist_ok=True)
    for file,content in files.items(): (data/file).write_bytes(content)
    subprocess.run([str(root/f"track-{track}"/("solution.exe" if sys.platform=="win32" else "solution")),str(data),str(data/"out")], check=True)
    output={"a":"summary.txt","c":"aggregates.txt","d":"distances.txt"}[track]
    assert (data/"out"/output).read_bytes()==expected,name
    print("PASS",name,flush=True)

spec=importlib.util.spec_from_file_location("c_tests",official/"tests"/"test_c_join.py")
c=importlib.util.module_from_spec(spec);spec.loader.exec_module(c)
run("c","c-worked",{"records.csv":c.WORKED_RECORDS,"regions.csv":c.WORKED_REGIONS},c.WORKED_OUTPUT)
run("c","c-rules",{"records.csv":c.RULES_RECORDS,"regions.csv":c.RULES_REGIONS},c.RULES_OUTPUT)
run("a","a-empty",{"requests.log":b""},b"")
run("a","a-extreme",{"requests.log":b"2026-11-01T00:00:00.000000Z GET / 499 4294967295\n2026-11-01T00:00:00.000000Z GET / 599 0\n"},b"2026-11-01T00:00:00Z / 2 0 4294967295 4294967295 1 500000\n")
for seed in range(20):
    rng=random.Random(seed);n=25
    edges=[(rng.randrange(n),rng.randrange(n),rng.choice([0,1,2,100,4294967295])) for _ in range(100)]
    adj=[[] for _ in range(n)]
    for u,v,w in edges:adj[u].append((v,w))
    answers=[]
    for source in range(n):
        dist=[10**30]*n;dist[source]=0;queue=[(0,source)]
        while queue:
            d,u=heapq.heappop(queue)
            if d!=dist[u]:continue
            for v,w in adj[u]:
                if d+w<dist[v]:dist[v]=d+w;heapq.heappush(queue,(d+w,v))
        answers.extend(-1 if d==10**30 else d for d in dist)
    graph=f"{n} {len(edges)}\n"+"".join(f"{u} {v} {w}\n" for u,v,w in edges)
    queries="".join(f"{u} {v}\n" for u in range(n) for v in range(n))
    run("d",f"d-fuzz-{seed}",{"graph.txt":graph.encode(),"queries.txt":queries.encode()},"".join(f"{x}\n" for x in answers).encode())

if (root/"track-b"/"main.cpp").exists():
    spec=importlib.util.spec_from_file_location("b_tests",official/"tests"/"test_b_index.py")
    b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)
    def candidate_search(docs,queries):
        data=root/".validation"/"edge-data"/"b-official"
        data.mkdir(parents=True,exist_ok=True)
        (data/"docs.txt").write_bytes(docs)
        (data/"queries.txt").write_bytes(queries)
        binary=root/"track-b"/("solution.exe" if sys.platform=="win32" else "solution")
        subprocess.run([str(binary),str(data),str(data/"out")],check=True)
        return (data/"out"/"results.txt").read_bytes()
    b.solve=candidate_search
    suite=unittest.TestSuite([b.WorkedExample("test_reference_matches_worked_example")])
    suite.addTests(unittest.defaultTestLoader.loadTestsFromTestCase(b.EdgeCases))
    if not unittest.TextTestRunner(verbosity=2).run(suite).wasSuccessful():
        raise SystemExit("Track B official fixtures failed")
