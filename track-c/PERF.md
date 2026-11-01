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

Board run ID for this C++ step: pending. Add the actual run ID, wall time, peak memory, K and points after the submission branch runs. No run ID has been fabricated. Human review of this explanation remains necessary; AI assistance does not establish human understanding by itself.

Development steps: the first C++ state parser used stream peek/get and per-country string sets (sample seed 1: 2.130265 s). A buffered reader plus the same sets matched the full hash in 10.621314 s. Global name interning and growable per-country bitmaps reduced repeated name storage; the final full run is reported above. These are local experimental observations, not Board measurements.
