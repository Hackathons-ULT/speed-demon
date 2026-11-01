# Injector Bind: Speed Demon 2026

The team submission is the `speed-demon` branch of https://github.com/Hackathons-ULT/speed-demon. The current candidates implement Tracks A (Ingest), C (Join), and D (Traverse) in zero-dependency C++17. Track B is still the unchanged organiser reference and is not a scored solution yet.

Each track is self-contained. It reads `/data`, writes exactly the required file to `/out`, and exits. Dockerfiles compile offline using allowlisted, digest-pinned GCC and Debian images. There are no precomputed answers, input recognition, network calls, or runtime packages.

| Track | Inputs | Output | Approach |
|---|---|---|---|
| A Ingest | `requests.log` | `summary.txt` | Map input read-only, intern endpoint views, use the specified 72 UTC-hour buckets, sort 32-bit latencies, select exact nearest ranks, and write bounded integer-formatted output. |
| C Join | `records.csv`, `regions.csv` | `aggregates.txt` | A read-only mapping and quote-aware span parser handle embedded newlines and BOMs. Direct numeric region lookup feeds exact integer-cent aggregates; globally interned names use country bitmaps. |
| D Traverse | `graph.txt`, `queries.txt` | `distances.txt` | Build forward/reverse CSR arrays and run bidirectional Dijkstra with radix queues, private epoch labels and up to four query workers. Preserve parallel edges, zero weights and query order. |

## Verified live Board results

The official Board scored all three active tracks at commit `501bd050342bba076e29b250f560f614230c3e5d` on November 1. All correctness, build, timeout, and sandbox gates passed. Expanded Linux CI, including sample seeds 1/7/42 and offline sandbox execution, also [passed](https://github.com/Hackathons-ULT/speed-demon/actions/runs/41309598714).

| Track | Run ID | Time (s) | Peak memory (MB, decimal) | K | Machine points |
|---|---|---:|---:|---:|---:|
| A | `run-6e7dcf2a13d6` | 0.481109 | 35.41 | 10.177465 | 50.381980 |
| C | `run-51af415ad600` | 0.961571 | 16.74 | 6.548947 | 40.808574 |
| D | `run-b601407e09bb` | 1.517466 | 276.45 | 6.980301 | 42.193707 |

A improved from 0.952371 s / K 7.402849 to 0.481109 s / K 10.177465. D improved from 3.003655 s / K 5.593737 to 1.517466 s / K 6.980301, with a measured memory cost for its four workers. C is unchanged from its verified best 0.748500 s / K 7.419147 run; its latest single practice run varied to 0.961571 s. Keep the single-run noise separate from actual code changes.

Run history and baseline comparisons remain in PERF.md and `board-results-history.json`. These are practice machine scores; holdout data and human judging determine final results. Three scored tracks meet the overall track-count requirement. Track B remains unchanged pending its agent implementation.

## Build and validate

```sh
g++ -std=c++17 -O3 -DNDEBUG -static track-a/main.cpp -o track-a/solution
g++ -std=c++17 -O3 -DNDEBUG -static track-c/main.cpp -o track-c/solution
g++ -std=c++17 -O3 -DNDEBUG -static -pthread track-d/main.cpp -o track-d/solution
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

All human registrations are confirmed by the entrant. The Board watches this repository under the confirmed team name "Injector Bind". No new team-form response is needed to change the tracked branch contents. Keep the Board App installed through freeze.

Code freeze is November 2, 2026 at 15:00 UTC. The repository must be public under its MIT licence by 16:00 UTC. Repository visibility has not been changed. Current rules: https://speeddemon.dev.
