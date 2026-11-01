# Encoded-view interning candidate validation

Candidate replaces per-record decoded name-string construction and node-based name interning with exact encoded payload views and a growing flat table. No cached answers, seed-dependent shortcuts, new runtime dependencies, or changes to CSV interpretation are used.

Compiled with GCC 16.1.0 MinGW-w64, C++17, `-O3 -DNDEBUG -Wall -Wextra -static`; no compiler warnings.

Output matched cached unmodified official reference bytes on full seed 1, sample seeds 1 and 7, and tiny seeds 1, 7 and 42. Full output SHA-256: `d8088b1cd42ba1338b1345b107b55db7b96ef1f5066f3ff64fd26a8912a0f536`.

Additional checks passed:

- Official worked example and broader parsing-rules fixture, including BOM bytes within quoted data and missing final newline.
- 6,000 independently generated rows across seeds 11, 73 and 909, compared with the official parser/aggregator. They include quoted/unquoted representations of the same values, empty values, one/two/three literal quotes, commas, embedded LF, BOMs within fields and at record boundaries, trailing-space twins, mixed LF/CRLF, negative money and unknown regions. These include valid grammar cases beyond the generator's narrower normal distribution.
- A hand-computed 110,001-distinct-name case forces the flat table and dense name-vector capacity to grow; exact count, ID sum, amount totals, distinct count and byte lengths match.

Interleaved full-data Windows development timings were noisy because other agents were generating/compiling concurrently. Baseline: 15.4423 and 5.2484 s. Flat candidate: 7.1494 and 4.4479 s. These observations justify an official comparison but are not a competition timing claim. The mmap path still requires coordinator Linux CI and genuine Board measurement before retention.

## Parallel follow-on candidate

The follow-on replaces the flat table with an exact concurrent chained interner, shared growable atomic membership bits, and worker-private numeric aggregates. It adds quote-parity record partitioning, AVX2 scanning with a scalar fallback, and a direct country-code lookup. No reference outputs are embedded. The earlier encoded-view candidate remains intact in commit `07aa1e8c26ecba052c753fe30e9f3b47848ac1c5`.

All cached reference datasets and official/fuzz/growth cases above pass with six workers. The expanded independent fuzz also includes literal CR and CRLF within quoted data.

Additional concurrency/partition checks passed:

- The full public dataset, 2,000-row mixed CSV fuzz, and hand-computed 110,001-name case give exact same output with 1, 2, 4, 6 and 12 workers.
- Six repeated 12-worker runs of 20,000 records with one shared empty name/country give one distinct value, exact ID/amount sums, and zero decoded name/note bytes. This stresses first-insertion, bitmap publication, and membership races.
- Scalar and AVX2 cuts agree with independently known record ends on 32 adversarial layouts at every worker count from 1 through 12. Long quoted fields span partition targets; escaped quotes, BOM bytes, and embedded newlines cross 32-byte vector boundaries. Missing final terminators are included.

One consecutive full-data Windows development sweep of the same binary measured: 1 worker 2.6918 s; 2 workers 1.3346 s; 4 workers 0.7641 s; 6 workers 0.6047 s; 12 workers 0.5602 s. These include Windows's private input-buffer allocation/read and are not Linux timings or official scores. Six workers is the conservative default for the scoring machine's six P-cores. Additional source and atomic-coherence costs remain subject to genuine Board measurement.


## Independent inline-identity follow-up

The current candidate retains record partitioning, CSV interpretation, exact membership and country aggregation, while replacing the common name lookup path with compact concurrent slots plus a lazy exact chained overflow. All historical cached fixture checks above pass. The full public output retains the published SHA-256.

New checks pass against the unmodified official reference:

- Tiny seed 20261102 with 1, 2, 6 and 12 workers; output SHA-256 `5549ebd82860d1f4e54f89ad30c0de91c4c639e59165cd53d7b957c351950f78`.
- 280003 distinct names across two countries with 1, 2, 6 and 12 workers, forcing many more names than inline capacity; SHA-256 `0a8a9a62c7448276d930393525810c5c2371e0435d347140b977a90afcf45cb4`.
- All hashes forced to zero for 1200 names and 12000 records with 1, 6 and 12 workers. Complete-byte equality and the overflow store remain exact; SHA-256 `f2985137a9dca5e68da29be4e4459f29add324107f99c3ced558d78a15ef5637`.
- Header-only input yields empty output with twelve workers.

Shared-lock alternating full-data benchmarking and Windows memory observations are recorded in PERF.md. These justify a Linux/Board comparison and are not an official performance result.

Publication-delay stress also passes: a temporary test-only build forced every name hash to zero, yielded 64 times after reserving an inline slot and eight times before chained-node publication, and ran twelve workers six times on each of a 1200-name collision case and a 30000-record single-empty-name case. Production code contains no delay.
