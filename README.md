# Injector Blind: Speed Demon 2026

The team submission is the `speed-demon` branch of https://github.com/Hackathons-ULT/speed-demon. The current candidates implement Tracks A (Ingest), C (Join), and D (Traverse) in zero-dependency C++17. Track B is still the unchanged organiser reference and is not a scored solution yet.

Each track is self-contained. It reads `/data`, writes exactly the required file to `/out`, and exits. Dockerfiles compile offline using allowlisted, digest-pinned GCC and Debian images. There are no precomputed answers, input recognition, network calls, or runtime packages.

| Track | Inputs | Output | Approach |
|---|---|---|---|
| A Ingest | `requests.log` | `summary.txt` | Copy the fixed-width UTC hour, hash `(hour, endpoint)` groups, sort 32-bit latencies, select exact nearest ranks, and emit sorted groups. |
| C Join | `records.csv`, `regions.csv` | `aggregates.txt` | A buffered CSV state machine handles quoting, embedded newlines and BOMs. Hash-join regions, aggregate exact integer cents, intern names globally, and count distinct names with country bitmaps. |
| D Traverse | `graph.txt`, `queries.txt` | `distances.txt` | Build forward/reverse CSR arrays and run bidirectional Dijkstra with 64-bit distances and epoch stamps. Preserve parallel edges, zero weights, and query order. |

## Build and validate

```sh
g++ -std=c++17 -O3 -DNDEBUG -static track-a/main.cpp -o track-a/solution
g++ -std=c++17 -O3 -DNDEBUG -static track-c/main.cpp -o track-c/solution
g++ -std=c++17 -O3 -DNDEBUG -static track-d/main.cpp -o track-d/solution
git clone https://github.com/hackathon-raptors/speed-demon-2026 ../official
python3 tools/edge_cases.py ../official
python3 tools/verify.py --official ../official --scale sample --seeds 1 7
```

Each executable accepts optional data and output paths for local checks. With no arguments it uses the container paths. The retained organiser `check.py` runs offline builds and the sandbox, for example `python3 check.py c --data data/c --expected EXPECTED_SHA256`. CI also checks Linux builds, multiple seeds, fixtures, and offline container execution. Local Windows validation is recorded in `validation.json`; it is not an official Board score.

For a full seed-1 container check, first generate inputs (Python 3.11 or newer; Docker required for `check.py`):

```sh
python3 ../official/generators/a_ingest.py --seed 1 --scale full --out data/a
python3 ../official/generators/c_join.py --seed 1 --scale full --out data/c
python3 ../official/generators/d_traverse.py --seed 1 --scale full --out data/d
python3 check.py a --data data/a --expected 495ca09682b9673ef836a506496177680bb4a3af54255174befddb1f83481cd3
python3 check.py c --data data/c --expected d8088b1cd42ba1338b1345b107b55db7b96ef1f5066f3ff64fd26a8912a0f536
python3 check.py d --data data/d --expected a23dbdf0a3e49cc8901310a9ee1dc6a9a569a85297b6da75c2a7002cb2ed277e
```

These are the organiser's published full-output hashes. The local candidates match all three. Sample and full use different records, so use each dataset's own reference hash when changing scale or seed. Only the official scoring machine determines competition scores.

Soorya Dev's earlier scored Python Track A is preserved byte for byte under `experiments/track-a-python/` and in Git history. Its actual Board results remain in `track-a/PERF.md`. C++ changes were written and tested with Codex assistance during the event. Read each track's README.md and PERF.md for its algorithm, correctness argument, measurements, dependencies, and proposed next experiments. Human understanding has not been asserted by the agent.

All human registrations are confirmed by the entrant. The Board already watches this repository under the spelling "Injector Bind"; project metadata follows the entrant's confirmed "Injector Blind" spelling. No new team-form response is needed to change the tracked branch contents. Keep the Board App installed through freeze.

Code freeze is November 2, 2026 at 15:00 UTC. The repository must be public under its MIT licence by 16:00 UTC. Repository visibility has not been changed. Current rules: https://speeddemon.dev.
