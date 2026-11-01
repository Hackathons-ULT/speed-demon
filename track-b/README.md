# B: Index

The query file is read first. Only words required by those queries are interned; only BOOL terms receive document posting lists. Token lookup uses a compact open-addressed table whose hash matches are checked against the complete token bytes, so collisions never become false matches.

Documents are tokenized as bytes: only ASCII A–Z lowercases, only ASCII letters and digits form tokens, and every byte at or above 0x80 is a separator. Unknown tokens reset phrase state and do not disappear between neighbouring query terms. Stopwords, leading zeros, repeated tokens, punctuation boundaries, and UTF-8 splitting therefore retain the specification's exact meaning.

All PHRASE patterns share a token-level Aho–Corasick automaton. Failure links find overlapping and suffix patterns; repeated query patterns share a terminal summary. During the corpus scan, each terminal is counted at most once per document. This avoids a full positional index and per-query document retokenization. Phrase state resets at every document boundary, so no match crosses documents.

Four workers scan disjoint, line-aligned corpus ranges. All token/trie tables are immutable; each worker owns its postings, deduplication stamps and phrase summaries. Corpus ranges follow ascending document IDs. Concatenating their posting lists preserves sorted unique IDs, and combining summaries in that same order preserves the smallest ten matching IDs. Query workers then apply BOOL operations strictly left to right using sorted set intersection, union, and difference; NOT is binary difference. Each query writes one private result slot, and joining workers before output preserves query order. Counts and document sums use 64-bit integers.

On Linux the two inputs are read-only mappings. Windows uses a private buffer for development checks. All indexing and matching occurs inside the timed run; nothing derived from any corpus is precomputed or retained between runs. The implementation uses the C++ standard library and POSIX mapping APIs with zero non-stdlib runtime package dependencies. The original published Python reference is retained unchanged for provenance.

## Build and run

```sh
g++ -std=c++17 -O3 -DNDEBUG -static -pthread main.cpp -o solution
./solution /path/to/data /path/to/out
```

Omitting arguments uses `/data` and `/out`. Exactly one file, `results.txt`, is written. The digest-pinned Dockerfile builds offline from the event's allowlisted GCC and Debian images and runs as uid 65534.

Validation has matched the published full seed-1 output SHA-256 `f86973c210228141bcb6f1f1b10188ad2a9b65f71e7cd7cf4590b38de4e80927` and sample seed-1 SHA-256 `eeeec1e9319da98a6f3c664c78f93e7078fd603c25c2bb3027174e81e3638a8f`. The official worked example and six handwritten edge-test methods pass, as do 8,000 independently generated random BOOL/PHRASE queries over 20 seeds. More generator-seed checks are logged in PERF.md as they complete. Official Board measurement is pending; local timings are not claimed as competition scores.

With another 24 hours, measure six workers versus four, profile sparse automaton transitions, and compare per-query set intersection strategies. Retain changes only if the genuine Board coefficient improves while all output and sandbox gates pass.
