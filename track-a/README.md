# A: Ingest

The timestamp grammar fixes the UTC hour in the first 13 bytes. Every seed has the specification's same 72-hour UTC span, so parsing its month/day/hour yields a direct bucket index instead of hashing a calendar string for every request. Endpoint strings are interned as views into a read-only input mapping on Linux, and each endpoint owns 72 groups. No endpoint catalogue or input answers are embedded. Windows uses a private input buffer for development checks.

Unsigned 32-bit latencies are sorted per group, and integer nearest ranks `(p*n+99)/100-1` select percentiles. Statuses beginning with 5 increment errors; error ppm uses a 64-bit product and integer division. Output numbers use locale-independent `to_chars` into a bounded stack buffer, with one buffered write per line.

Endpoint keys sort bytewise once, then hours iterate in chronological bucket order. Original timestamp prefixes supply output hour bytes, including the month boundary. Input order is irrelevant, duplicate lines count separately, and UINT32_MAX latency remains exact. Empty input produces an empty file.

Auxiliary storage is O(requests + endpoints * 72); expected grouping time is linear, followed by O(sum n_g log n_g) latency sorting and endpoint-key sorting. The input mapping is read-only and no derived state persists across runs. C++ standard library and POSIX mapping calls add no runtime package dependency.

With another 24 hours, compare three-rank selection with full sorting, then measure thread-local aggregation on the Board. These are proposals, not measured improvements.
