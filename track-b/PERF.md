# Track B: performance log

Every step from the reference to your final version, each with the live-board run ID that measured it. Judges check that the run IDs exist and that each change moved the way you say it did.

| Step | Change | Run ID | T (s) | M (MB) | Points |
|---|---|---|---|---|---|
| 0 | The reference, unchanged | | 56.35 | 2,200 | 0 |
| 1 | | | | | |


## November 1: query-first C++ index and shared phrase matcher

The previous Track B folder was the unchanged published reference, which the Board skipped. This candidate replaces the active runtime with zero-dependency C++17 while retaining the published reference files unchanged. All new solution code was written during the event with Codex assistance; human understanding has not been asserted by the agent.

Development stages:

1. Read queries first and intern their exact words. Build sorted document postings only for BOOL terms. Build a token-level Aho–Corasick automaton for all phrases, deduplicate identical patterns, and summarize matches during the single corpus scan instead of retaining positions or retokenizing candidates.
2. Freeze the query vocabulary into a compact flat lookup table. Full-byte equality checks resolve hash collisions exactly. Corpus tokenization uses a bounded lowercase byte buffer with no token allocation.
3. Scan four disjoint complete-document ranges with private postings/stamps/phrase summaries, concatenate sorted ranges, and answer independent queries with four workers. The immutable tables are shared and every output slot has one writer.

Correctness evidence: all eight official worked-example/handwritten fixture tests pass (including the example's textual consistency test). Twenty independent fuzz corpora and 400 queries each give 8,000 exact differential answers against the unmodified official reference. Cases include ASCII-only case conversion, UTF-8 separators and normalization traps, empty/no-token documents, zero-padded digits, duplicate terms and phrase overlaps, left-associative mixed operators, binary NOT, first-ten ordering, and document boundaries. Tiny seed 1 and sample seed 1 match the reference byte for byte. The sample's published output hash is `eeeec1e9319da98a6f3c664c78f93e7078fd603c25c2bb3027174e81e3638a8f`.

The coordinator downloaded the official full seed-1 inputs and verified their published SHA-256 values before testing. This candidate matches the published full output hash `f86973c210228141bcb6f1f1b10188ad2a9b65f71e7cd7cf4590b38de4e80927` (1,459,301 output bytes). These are computed results, not cached answers.

Local development timings on Windows x86-64, GCC 16.1.0, C++17 -O3 -DNDEBUG -static: full single-worker flat-table version 20.807110 s; four-worker candidate 10.305907 s. A later sample run took 0.289790 s. These are single development runs amid other work, not controlled or official measurements, and no competition points are inferred. Windows also copies input into a private buffer, unlike the Linux mapping path. Peak memory is not yet measured.

Official Linux/container CI and live Board run ID: pending coordinator integration. Record the actual run, time, memory, source size, K and points after all gates pass; no Board ID or score has been fabricated. The organiser's holdout seed remains unknown; varied local seeds and random fixtures test semantics rather than predict that seed.
