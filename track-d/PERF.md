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

Board run ID for this C++ step: pending. Add the actual run ID, wall time, peak memory, K and points after the submission branch runs. No run ID has been fabricated. Human review of this explanation remains necessary; AI assistance does not establish human understanding by itself.

The primary change is forward/reverse CSR plus bidirectional Dijkstra and per-query epoch stamps. This removes Python edge tuples, avoids clearing all distance labels for short queries, and explores from both query endpoints. Zero weights and paths above 2^32 were validated explicitly.
