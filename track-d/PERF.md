# Track D: performance log

Every step from the reference to your final version, each with the live-board run ID that measured it. Judges check that the run IDs exist and that each change moved the way you say it did.

| Step | Change | Run ID | T (s) | M (MB) | Points |
|---|---|---|---|---|---|
| 0 | The reference, unchanged | | 72.69 | 893 | 0 |
| 1 | | | | | |


## November 1: compact C++ candidate (Codex assisted)

The approach and correctness argument are in README.md. The new solution uses no direct runtime dependencies. All new solution code was written during the event on November 1; the organiser's references and earlier teammate work predate this change and are preserved in Git history.

Local development check: Windows x86-64, GCC 16.1.0 MinGW-w64, C++17 -O3 -DNDEBUG -static. Full seed-1 output matched both the unmodified official reference and published SHA-256 `a23dbdf0a3e49cc8901310a9ee1dc6a9a569a85297b6da75c2a7002cb2ed277e`. The final local solution run took 18.877821 s. This is one development run, not a controlled scoring measurement. Memory has not been measured. No official score is inferred from this time.

Differential checks also pass on tiny seeds 1, 7, 42 and sample seeds 1, 7. Additional fixtures cover CSV quoting, BOMs within values, end-of-file, status/uint32 boundaries, empty input, and 12,500 independently computed graph-query answers. The three official reference test suites pass 43 tests. `validation.json` records the evidence.

Official Board result for the C++ step, commit `738832835b87dd40b567dac9cfd6ac4db2e9aa06`:

| Run ID | T (s) | Peak memory (bytes) | Source lines | D | K | Machine points |
|---|---:|---:|---:|---:|---:|---:|
| `run-632362c7b696` | 5.841748 | 161996800 | 147 | 1 | 4.127217 | 30.782867 |

The official run passed check, build, correctness, timeout and sandbox gates on the full public seed-1 dataset. This is the live practice Board, not the final holdout score. See `board-results.json` for the recorded receipt. Human review of this explanation remains necessary; AI assistance does not establish human understanding by itself.

The primary change is forward/reverse CSR plus bidirectional Dijkstra and per-query epoch stamps. This removes Python edge tuples, avoids clearing all distance labels for short queries, and explores from both query endpoints. Zero weights and paths above 2^32 were validated explicitly.


## Iteration 2: remove avoidable per-item overhead

Measured baseline at `b9b454633b4a952a230852e5f35eaa275a018ec1`: `run-abdc1c0e6d1f`, T 5.857109 s, peak 162013184 bytes, K 4.121718, machine points 30.753915. All gates passed.

The candidate replaces each binary priority queue with a monotone 64-bit radix heap. The invariant is per-queue: non-negative edge weights make every inserted key at least the last popped key. Stale entries are still discarded. Redistribution changes tie order only, and equal-distance vertex order cannot change shortest-path answers. Local full-data output remains byte-identical; the development run took about 9.8 s versus earlier binary-heap runs of 14.4 to 18.9 s. These are not official scores.

Expanded Linux CI checks tiny seeds 1/7/42, sample seeds 1/7/42, handwritten CSV boundary cases, and 12,500 independently computed shortest-path answers, then runs offline container builds and unprivileged no-network/read-only sandbox checks. These seed variations test generality; they do not reveal the organiser's private holdout seed. Candidate CI: https://github.com/Hackathons-ULT/speed-demon/actions/runs/41308100814.

Official Board result for iteration 2: pending. Keep or revise this candidate based on genuine measured K after all gates pass.
