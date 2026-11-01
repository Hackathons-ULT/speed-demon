# D: Traverse

A buffered decimal parser reads the directed edge list. Prefix sums build forward and reverse compressed sparse row adjacency arrays. Parallel edges, duplicates, zero weights, and self-loops are preserved. Raw construction triples are released before queries.

Bidirectional Dijkstra searches forward from source and backward from target. Both monotone radix heaps hold tentative 64-bit distances and discard stale entries. Each queue's inserted keys are at least its last popped key, because all weights are non-negative. Buckets group keys by the highest differing bit from that key; redistributing the first nonempty bucket to its minimum preserves exact minimum order without binary-heap sifting. Equal-key node order does not affect distances. Expanding the side with the smaller frontier minimum concentrates work near the endpoints. Whenever a node has distances from both sides, their sum describes a real path and improves the upper bound if smaller.

Stop when frontier minima sum to at least the upper bound: a shorter path would still require a frontier crossing with a smaller sum. If either frontier empties, no remaining improvement exists. An infinite bound means unreachable. Equal endpoints return zero. Distance sums exceed 32 bits safely; edge weights and vertex IDs use 32 bits.

Epoch stamps distinguish visits between queries, avoiding O(vertices) clearing per query. Answers are emitted in input order. Graph/label memory is O(vertices + edges); stale queued relaxations can add O(edges) items. Each key moves through at most 64 radix buckets, a fixed bound for the 64-bit distance domain. This is single-threaded and uses no shortest-path library or direct runtime dependency.

With another 24 hours, compare a bounded pool of parallel query workers against their additional distance-array memory and test compact graph ordering. Measure both speed and memory before adopting either.
