# Track C: performance log

Every step from the reference to your final version, each with the live-board run ID that measured it. Judges check that the run IDs exist and that each change moved the way you say it did.

| Step | Change | Run ID | T (s) | M (MB) | Points |
|---|---|---|---|---|---|
| 0 | The reference, unchanged | | 14.44 | 367 | 0 |
| 1 | | | | | |


## November 1: compact C++ candidate (Codex assisted)

The approach and correctness argument are in README.md. The new solution uses no direct runtime dependencies. All new solution code was written during the event on November 1; the organiser's references and earlier teammate work predate this change and are preserved in Git history.

Local development check: Windows x86-64, GCC 16.1.0 MinGW-w64, C++17 -O3 -DNDEBUG -static. Full seed-1 output matched both the unmodified official reference and published SHA-256 `d8088b1cd42ba1338b1345b107b55db7b96ef1f5066f3ff64fd26a8912a0f536`. The final local solution run took 7.890878 s. This is one development run, not a controlled scoring measurement. Memory has not been measured. No official score is inferred from this time.

Differential checks also pass on tiny seeds 1, 7, 42 and sample seeds 1, 7. Additional fixtures cover CSV quoting, BOMs within values, end-of-file, status/uint32 boundaries, empty input, and 12,500 independently computed graph-query answers. The three official reference test suites pass 43 tests. `validation.json` records the evidence.

Official Board result for the C++ step, commit `738832835b87dd40b567dac9cfd6ac4db2e9aa06`:

| Run ID | T (s) | Peak memory (bytes) | Source lines | D | K | Machine points |
|---|---:|---:|---:|---:|---:|---:|
| `run-b5d77a1cbee5` | 2.974006 | 28790784 | 150 | 1 | 3.491075 | 27.147959 |

The official run passed check, build, correctness, timeout and sandbox gates on the full public seed-1 dataset. This is the live practice Board, not the final holdout score. See `board-results.json` for the recorded receipt. Human review of this explanation remains necessary; AI assistance does not establish human understanding by itself.

Development steps: the first C++ state parser used stream peek/get and per-country string sets (sample seed 1: 2.130265 s). A buffered reader plus the same sets matched the full hash in 10.621314 s. Global name interning and growable per-country bitmaps reduced repeated name storage; the final full run is reported above. These are local experimental observations, not Board measurements.


## Iteration 2: remove avoidable per-item overhead

Measured baseline at `b9b454633b4a952a230852e5f35eaa275a018ec1`: `run-71dd05e958f9`, T 2.991594 s, peak 28844032 bytes, K 3.479511, machine points 27.075913. All gates passed.

The candidate maps the read-only input on Linux, parses numeric fields directly, uses memchr to scan quoted spans, counts decoded note bytes without allocating a note string, and replaces region hashing plus country-tree searching with a numeric lookup vector and direct aggregate IDs. Exact name interning and membership bitmaps remain. No catalogues, region IDs, names or answers are embedded. Local Windows full-data correctness remains byte-identical and runtime fell from about 7.9 s to 3.4 s; those local values are not an official score.

Expanded Linux CI checks tiny seeds 1/7/42, sample seeds 1/7/42, handwritten CSV boundary cases, and 12,500 independently computed shortest-path answers, then runs offline container builds and unprivileged no-network/read-only sandbox checks. These seed variations test generality; they do not reveal the organiser's private holdout seed. Candidate CI: https://github.com/Hackathons-ULT/speed-demon/actions/runs/41308100814.

Official Board result for iteration 2: pending. Keep or revise this candidate based on genuine measured K after all gates pass.
