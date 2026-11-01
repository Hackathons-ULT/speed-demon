# Encoded-view interning candidate validation

Candidate replaces per-record decoded name-string construction and node-based name interning with exact encoded payload views and a growing flat table. No cached answers, seed-dependent shortcuts, new runtime dependencies, or changes to CSV interpretation are used.

Compiled with GCC 16.1.0 MinGW-w64, C++17, `-O3 -DNDEBUG -Wall -Wextra -static`; no compiler warnings.

Output matched cached unmodified official reference bytes on full seed 1, sample seeds 1 and 7, and tiny seeds 1, 7 and 42. Full output SHA-256: `d8088b1cd42ba1338b1345b107b55db7b96ef1f5066f3ff64fd26a8912a0f536`.

Additional checks passed:

- Official worked example and broader parsing-rules fixture, including BOM bytes within quoted data and missing final newline.
- 6,000 independently generated rows across seeds 11, 73 and 909, compared with the official parser/aggregator. They include quoted/unquoted representations of the same values, empty values, one/two/three literal quotes, commas, embedded LF, BOMs within fields and at record boundaries, trailing-space twins, mixed LF/CRLF, negative money and unknown regions. These include valid grammar cases beyond the generator's narrower normal distribution.
- A hand-computed 110,001-distinct-name case forces the flat table and dense name-vector capacity to grow; exact count, ID sum, amount totals, distinct count and byte lengths match.

Interleaved full-data Windows development timings were noisy because other agents were generating/compiling concurrently. Baseline: 15.4423 and 5.2484 s. Flat candidate: 7.1494 and 4.4479 s. These observations justify an official comparison but are not a competition timing claim. The mmap path still requires coordinator Linux CI and genuine Board measurement before retention.
