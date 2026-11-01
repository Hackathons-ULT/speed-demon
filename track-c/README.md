# C: Join

A buffered byte reader parses logical CSV records. Quoted fields consume doubled quotes as one literal quote and retain embedded LF, commas, trailing spaces, and UTF-8 bytes. BOM bytes are skipped only at a logical record boundary, so a BOM within a quoted value remains data. CRLF and LF terminators and an unterminated final record are supported.

The region hash map joins numeric IDs to country strings. Missing regions use `_UNMATCHED`. Counters and money values use signed 64-bit integers. Money is parsed in exact cents, including negative sub-unit amounts, then formatted with two fractional digits. A global hash map interns each exact unescaped name once; each country keeps a growable bitmap of those IDs and a distinct-name counter. Quoted and unquoted empty names compare equal, while trailing-space twins remain distinct. Byte lengths are measured after quote unescaping.

Countries are sorted and only created by actual records, omitting unused countries. Input is streamed; memory depends on the region lookup, global distinct names, and country/name bitmap capacity. Only the C++ standard library is used: zero direct runtime dependencies.

With another 24 hours, benchmark a compact numeric region lookup and hash-based membership for unusually sparse country/name combinations. Keep parser fixtures and multi-seed differential tests as correctness gates.
