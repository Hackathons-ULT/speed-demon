# Track A: performance log

Every step from the reference to your final version, each with the live-board run ID that measured it. Judges check that the run IDs exist and that each change moved the way you say it did.

| Step | Change | Run ID | T (s) | M (MB) | Points |
|---|---|---|---|---|---|
| 0 | The reference, unchanged | | 32.05 | 228 | 0 |
| 1 | Replaced the reference with a stdlib-only optimised parser: no `datetime` (hour = first 13 bytes of the timestamp), one `bytes.split()` per ~256 KB chunk, forked workers pulling line-aligned tasks from a pipe, whole-hour segments found with `bisect`, a C-level 5xx scan, compact `array('I')` latency buffers, and parallel rank selection (histogram or partial sort) with a fallback to the reference for anything unexpected. | run-d501b265e44a | 1.88 | 801 | 18.34 |


## November 1: compact C++ candidate (Codex assisted)

The approach and correctness argument are in README.md. The new solution uses no direct runtime dependencies. All new solution code was written during the event on November 1; the organiser's references and earlier teammate work predate this change and are preserved in Git history.

Local development check: Windows x86-64, GCC 16.1.0 MinGW-w64, C++17 -O3 -DNDEBUG -static. Full seed-1 output matched both the unmodified official reference and published SHA-256 `495ca09682b9673ef836a506496177680bb4a3af54255174befddb1f83481cd3`. The final local solution run took 3.257132 s. This is one development run, not a controlled scoring measurement. Memory has not been measured. No official score is inferred from this time.

Differential checks also pass on tiny seeds 1, 7, 42 and sample seeds 1, 7. Additional fixtures cover CSV quoting, BOMs within values, end-of-file, status/uint32 boundaries, empty input, and 12,500 independently computed graph-query answers. The three official reference test suites pass 43 tests. `validation.json` records the evidence.

Official Board result for the C++ step, commit `738832835b87dd40b567dac9cfd6ac4db2e9aa06`:

| Run ID | T (s) | Peak memory (bytes) | Source lines | D | K | Machine points |
|---|---:|---:|---:|---:|---:|---:|
| `run-d530eae56018` | 1.007284 | 65634304 | 58 | 1 | 7.197606 | 42.859403 |

The official run passed check, build, correctness, timeout and sandbox gates on the full public seed-1 dataset. This is the live practice Board, not the final holdout score. See `board-results.json` for the recorded receipt. Human review of this explanation remains necessary; AI assistance does not establish human understanding by itself.

The earlier scored Python implementation is preserved under `experiments/track-a-python/`; its existing performance log above is unchanged. The compact candidate avoids the per-process parsing/ranking machinery and stores only one 32-bit latency per request. Whether it improves the coefficient must be established on the same Board hardware.


## Iteration 3: memory layout and throughput experiment

Genuine baseline: `run-19c6d1657d8d` at `7491483ff1e0fe3f3d2b84f3e610310207751da6`, T 0.952371 s, peak 65605632 bytes, K 7.402849, machine points 43.469944; all gates passed.

The candidate maps input read-only on Linux, interns endpoint views once, and uses 72 direct hour buckets per endpoint instead of constructing an hour/endpoint string key per record. The fixed time span is an explicit specification guarantee shared by every seed, not an input fingerprint. Timestamp prefixes are retained for exact output. A bounded stack buffer plus locale-independent integer conversion reduces output formatting overhead. Full sorting and exact nearest ranks remain.

Handwritten edge fixtures and all 12,500 independently computed random shortest-path answers pass, including parallel, zero-weight, unreachable and over-uint32 paths. Existing seed-1 full data and multiple tiny/sample seeds remain correctness gates. Expanded Linux CI will additionally exercise the POSIX mapping path and offline container sandbox.

Official Board result for iteration 3: pending. No timing, memory, coefficient or run ID for this new candidate is claimed yet.

## Track A agent: disjoint endpoint shards and progressive rank selection

The coordinator verified the mapped-input iteration-3 baseline on the official Board: `run-6e7dcf2a13d6`, T **0.481109 s**, peak **35,405,824 bytes**, S **154**, D **1**, K **10.177465**, machine points **50.381980**; all gates passed. This is the comparison baseline for the candidate below.

Four workers scan the shared read-only mapping and own disjoint endpoint-hash shards. Only a record's owner parses its time/status/latency and writes its group. Every latency is stored once; no group locks, atomic counters or cross-worker latency copying are needed. Groups up to 64 items retain sorting; larger groups use three progressive exact nearest-rank selections. README.md explains the ownership and rank invariants.

Local Windows GCC16.1 validation matched the unmodified official reference on full seed1; samples1/7/42; tiny seeds1/7/42/99/1009; empty input; and a 10,201-line randomized contract fixture with duplicates, all 72 hours, status values spanning100–599, zero/UINT32_MAX latencies, max-length endpoints and an isolated101-item rank group. The randomized fixture was repeated three times with identical bytes. The full seed1 output remains `495ca09682b9673ef836a506496177680bb4a3af54255174befddb1f83481cd3`.

Alternating full-data development runs under concurrent host load gave base501 median **3.217355 s** and this candidate median **2.509646 s** (three runs each). A smaller selection-only experiment gave about8% local median improvement. Host timings were noisy, so neither observation is an official speedup or a coefficient claim. The extra scans, code and rank fields are priced trade-offs and need a genuine Board comparison.

Official Board result for the four-shard candidate: **pending**. Do not replace the baseline based on local time alone; the coordinator should retain it only after Linux sandbox/correctness checks and a measured K improvement.

## Track A agent: contiguous parse, flat latency storage, parallel output

The four-shard candidate subsequently passed the genuine Board: T **0.249200 s**, peak **36,372,480 bytes**, S **200**, K **13.524502**, machine points **56.556065**. The full official run ID is pending the coordinator's retained receipt; it is deliberately not invented here.

The next experiment replaces repeated whole-file endpoint scans with six contiguous line-aligned parser ranges and immutable concurrently interned keys. Atomic counts plus prefix sums and atomic scatter cursors create one flat latency array without per-group vector growth. Progressive exact ranks are selected in parallel. Six formatters create consecutive hour ranges in private strings, which the main thread writes in order. README.md gives the publication, cursor-ownership and join invariants.

The packed-record variant is byte-correct against the official reference for full seed1; samples1/7/42; tiny1/7/42/99/1009; empty input; a randomized10,201-line contract fixture repeated three times; a100,000-record single-group contention fixture repeated ten times; and the common interning/scatter core passed forced endpoint-hash collisions repeated ten times (test binary changes only the initial hash slot). Output for full1 is unchanged at `495ca09682b9673ef836a506496177680bb4a3af54255174befddb1f83481cd3`.

Alternating Windows full-data medians (three runs each) were **1.324640 s** for base501 and **0.542404 s** for packed records plus parallel output. A diagnostic phase profile before parallel output measured load0.367s / parse0.102s / prefix0.001s / scatter0.018s / rank0.010s / serial output0.071s. These are local development observations under variable host load, not official score claims. The packed buffer trades peak memory for parsing once; glibc arena retention can affect the real peak.

Official Board result for the packed-throughput candidate: **pending**. Linux CI and genuine K comparison remain required.
