# D: Traverse

A buffered decimal parser reads the directed edge list. Prefix sums build forward and reverse compressed sparse row adjacency arrays. Parallel edges, duplicates, zero weights, and self-loops are preserved. Raw construction triples are released before queries.

Bidirectional Dijkstra searches forward from source and backward from target. Both monotone radix heaps hold tentative 64-bit distances and discard stale entries. Each queue's inserted keys are at least its last popped key, because all weights are non-negative. Buckets group keys by the highest differing bit from that key; redistributing the first nonempty bucket to its minimum preserves exact minimum order without binary-heap sifting. Equal-key node order does not affect distances. Expanding the side with the smaller frontier minimum concentrates work near the endpoints. Whenever a node has distances from both sides, their sum describes a real path and improves the upper bound if smaller.

Stop when frontier minima sum to at least the upper bound: a shorter path would still require a frontier crossing with a smaller sum. If either frontier empties, no remaining improvement exists. An infinite bound means unreachable. Equal endpoints return zero. Distance sums exceed 32 bits safely; edge weights and vertex IDs use 32 bits.

Up to four query workers share immutable adjacency arrays. Each worker owns its distance arrays, epoch stamps and heaps. An atomic counter assigns each query index to exactly one worker, and workers write distinct answer slots. Joining every worker before output gives deterministic input-order emission without locks in the search loop. Epoch stamps avoid O(vertices) clearing per query. This trades additional private label memory for parallel search throughput.

For W workers, graph/label memory is O(edges + W * vertices); stale relaxations add up to O(W * edges) heap items in the worst case. Each key moves through at most 64 radix buckets, a fixed bound for the 64-bit distance domain. No shortest-path library or direct runtime dependency is used. Build with `-pthread`.

With another 24 hours, tune worker count against measured memory cost and test compact graph ordering. Keep a change only when the genuine Board coefficient improves.
