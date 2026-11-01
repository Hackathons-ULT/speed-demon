# A: Ingest

The timestamp grammar fixes the UTC hour in the first 13 bytes, so we copy the prefix instead of parsing a calendar date per request. A hash table groups the hour and exact endpoint bytes. Unsigned 32-bit latencies are sorted per group, and integer nearest ranks `(p*n+99)/100-1` select percentiles. Statuses beginning with 5 increment errors; error ppm uses a 64-bit product and integer division.

Group keys sort bytewise. Fixed-width ISO hours give chronological order across the month boundary. Input order is irrelevant, duplicate lines count separately, and UINT32_MAX latency remains exact. Empty input produces an empty file.

Input is streamed. Storage is O(requests + groups); expected grouping time is linear, followed by O(sum n_g log n_g) latency sorting and group-key sorting. Only the C++ standard library is used: zero direct runtime dependencies.

With another 24 hours, compare three-rank selection with full sorting, then measure thread-local aggregation and endpoint interning on the Board. These are proposals, not measured improvements.
