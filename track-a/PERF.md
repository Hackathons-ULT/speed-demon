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
