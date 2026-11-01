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

## Follow-up: bounded SSE2 token spans

The coordinator's genuine first Board baseline is `run-278ebe10c352`: T 1.533255 s, M 228073472 bytes, S 382, K 7.692432, machine points 44.303185; all gates passed. A follow-up replaces per-byte classification/folding on full spans with baseline x86-64 SSE2. It still performs exact token equality, never hashes alone, and uses guarded loads plus scalar tails.

The follow-up passes the same eight official fixture tests, 8,000 random differential queries, tiny seeds 1/42, sample seeds 1/7 and the full published output hash. An additional 1,634 queries over 564 documents exercise token lengths 1 through 33, sixteen-byte alignments, repeated tokens, digits, UTF-8 separators and nonmatching overlength prefixes. Sample-7 hash is `7f41992e4446b5b88cf7a46ca6bae971f5bafb7984208164c3be3d6dda5c4139`; tiny-42 hash is `ed3989207068087274a213c952af0c8a11dc782886e772874db633c3b1b73934`.

Alternating full Windows development runs were 2.388/2.252 s for the scalar version and 2.012/2.037 s for SSE2. These are local observations only; the follow-up's actual Board K and memory are pending. Six/eight-worker timing experiments and sparse-set search experiments were inconclusive, so they were not added to this candidate.

## Verified four-agent Board receipts

These exact practice Board receipts resolve the pending measurements above. All check, build, correctness, timeout and sandbox gates passed.

| Commit | Run ID | Time (s) | Peak bytes | Source | K | Machine points |
|---|---|---:|---:|---:|---:|---:|
| `3733f64` | `run-278ebe10c352` | 1.533255 | 228073472 | 382 | 7.692432 | 44.303185 |
| `ee4c7f9` | `run-13409d072f12` | 1.363277 | 229441536 | 410 | 8.062158 | 45.322565 |

The measured candidate is retained. Linux validation, including offline builds and sandbox execution, passed at [CI 41313474697](https://github.com/Hackathons-ULT/speed-demon/actions/runs/41313474697). Holdout correctness and final human judging remain separate from these public practice measurements.
