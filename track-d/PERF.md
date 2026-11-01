# Track D: performance log

Every step from the reference to your final version, each with the live-board run ID that measured it. Judges check that the run IDs exist and that each change moved the way you say it did.

| Step | Change | Run ID | T (s) | M (MB) | Points |
|---|---|---|---|---|---|
| 0 | The reference, unchanged | | 72.69 | 893 | 0 |
| 1 | | | | | |


## November 1: compact C++ candidate (Codex assisted)

The approach and correctness argument are in README.md. The new solution uses no direct runtime dependencies. All new solution code was written during the event on November 1; the organiser's references and earlier teammate work predate this change and are preserved in Git history.

Local development check: Windows x86-64, GCC 16.1.0 MinGW-w64, C++17 -O3 -DNDEBUG -static. Full seed-1 output matched both the unmodified official reference and published SHA-256 `a23dbdf0a3e49cc8901310a9ee1dc6a9a569a85297b6da75c2a7002cb2ed277e`. The final local solution run took 18.877821 s. This is one development run, not a controlled scoring measurement. Memory has not been measured. No official score is inferred from this time.

Differential checks also pass on tiny seeds 1, 7, 42 and sample seeds 1, 7. Additional fixtures cover CSV quoting, BOMs within values, end-of-file, status/uint32 boundaries, empty input, and 12,500 independently computed graph-query answers. The three official reference test suites pass 43 tests. `validation.json` records the evidence.

Official Board result for the C++ step, commit `738832835b87dd40b567dac9cfd6ac4db2e9aa06`:

| Run ID | T (s) | Peak memory (bytes) | Source lines | D | K | Machine points |
|---|---:|---:|---:|---:|---:|---:|
| `run-632362c7b696` | 5.841748 | 161996800 | 147 | 1 | 4.127217 | 30.782867 |

The official run passed check, build, correctness, timeout and sandbox gates on the full public seed-1 dataset. This is the live practice Board, not the final holdout score. See `board-results.json` for the recorded receipt. Human review of this explanation remains necessary; AI assistance does not establish human understanding by itself.

The primary change is forward/reverse CSR plus bidirectional Dijkstra and per-query epoch stamps. This removes Python edge tuples, avoids clearing all distance labels for short queries, and explores from both query endpoints. Zero weights and paths above 2^32 were validated explicitly.


## Iteration 2: remove avoidable per-item overhead

Measured baseline at `b9b454633b4a952a230852e5f35eaa275a018ec1`: `run-abdc1c0e6d1f`, T 5.857109 s, peak 162013184 bytes, K 4.121718, machine points 30.753915. All gates passed.

The candidate replaces each binary priority queue with a monotone 64-bit radix heap. The invariant is per-queue: non-negative edge weights make every inserted key at least the last popped key. Stale entries are still discarded. Redistribution changes tie order only, and equal-distance vertex order cannot change shortest-path answers. Local full-data output remains byte-identical; the development run took about 9.8 s versus earlier binary-heap runs of 14.4 to 18.9 s. These are not official scores.

Expanded Linux CI checks tiny seeds 1/7/42, sample seeds 1/7/42, handwritten CSV boundary cases, and 12,500 independently computed shortest-path answers, then runs offline container builds and unprivileged no-network/read-only sandbox checks. These seed variations test generality; they do not reveal the organiser's private holdout seed. Candidate CI: https://github.com/Hackathons-ULT/speed-demon/actions/runs/41308100814.

Official iteration-2 result: `run-9433ca9e282a`, commit `7491483ff1e0fe3f3d2b84f3e610310207751da6`, T **3.003655 s**, peak **161878016 bytes**, S 178, D 1, **K 5.593737**, machine points **37.385101**. All check/build/correctness/timeout/sandbox gates passed. This improved the measured coefficient, so the change is retained.


## Iteration 3: memory layout and throughput experiment

Genuine baseline: `run-9433ca9e282a` at `7491483ff1e0fe3f3d2b84f3e610310207751da6`, T 3.003655 s, peak 161878016 bytes, K 5.593737, machine points 37.385101; all gates passed.

The candidate keeps the measured radix-heap search and runs at most four independent queries in parallel. Adjacency arrays are immutable, every worker has private labels/epochs/heaps, an atomic index gives each answer slot one writer, and all workers join before ordered output. Extra label memory is a priced trade-off. The Board's K, rather than local throughput alone, decides whether this version is retained. Local full-data output matches the reference; a development run dropped from about 9.8 s to 6.7 s, which is not an official measurement.

Handwritten edge fixtures and all 12,500 independently computed random shortest-path answers pass, including parallel, zero-weight, unreachable and over-uint32 paths. Existing seed-1 full data and multiple tiny/sample seeds remain correctness gates. Expanded Linux CI will additionally exercise the POSIX mapping path and offline container sandbox.

Official Board result for iteration 3: `run-b601407e09bb` at `501bd050342bba076e29b250f560f614230c3e5d`, T **1.517466 s**, peak **276451328 bytes**, K **6.980301**, machine points **42.193707**. All gates passed. The measured K improved over the single-worker radix version despite the extra label memory, so the four-worker version was retained.

## Cache-local numbering and cached queue frontiers

The coordinator measured the four-worker radix baseline at commit `501bd050342bba076e29b250f560f614230c3e5d`: `run-b601407e09bb`, T **1.517466 s**, peak **276451328 bytes**, K **6.980301**, machine points **42.193707**, with every gate passing.

This candidate builds a forward CSR, visits every component with BFS, and assigns consecutive vertex IDs in discovery order. It applies the resulting bijection to every edge endpoint and every query endpoint, then rebuilds both CSR directions. Parallel edges, weights, self-loops, isolated vertices and directed reachability are unchanged. No graph-derived state is retained between executions. All relabeling and rebuilding is included in the timed process. The permutation and BFS order are freed before query-worker labels are allocated.

After stale heap entries are discarded, each search iteration caches its two valid frontier entries instead of repeatedly querying the same radix buckets. The stopping rule and expansion choice remain unchanged.

Validation against the unmodified official reference passes the published full seed-1 hash `a23dbdf0a3e49cc8901310a9ee1dc6a9a569a85297b6da75c2a7002cb2ed277e`, sample seeds 1, 7 and 42, and tiny seeds 1, 7, 42, 31337 and 65537. The extra sample-42 hash is `e003ecd1553a9010cca836975f759aa8588403070e83ed359cc3fd991a2275a6`. A separate heapq oracle verifies 15,000 all-pairs answers over 24 random directed multigraphs, including zero and UINT32_MAX weights, disconnected vertices, duplicate/parallel edges and self-loops.

Local Windows timing is development evidence only. Under concurrent agent load, three alternating full-data runs before frontier caching measured baseline **10.445, 19.003, 17.261 s** and BFS candidate **14.314, 13.867, 10.859 s**. The medians favor BFS by roughly 20%, but the large spread rules out a precise claim. A later cached-frontier correctness run took **3.748 s** under lighter load and is not comparable to those earlier runs. Raw local evidence is in the isolated worktree's ignored `.validation/benchmark-interleaved.json` and `.validation/benchmark.json`.

Official Board measurement for this cache-local candidate: pending. Its extra preprocessing and source lines must earn their cost through measured K before the coordinator retains it.

## Balance frontier work rather than distance scales

The coordinator identified a pathological expansion order: an island query can leave a UINT32_MAX-weight forward spur while the reverse frontier lies in the giant component. Always choosing the smaller distance then exhausts the giant component before the tiny forward side finishes. A no-in source with huge outgoing weights has a similar imbalance risk.

Only the expansion scheduling heuristic changes: expand the side with fewer queued heap entries. Each direction still pops its own exact minimum, and the termination rule remains `min_forward + min_reverse >= best`. All crossing-label updates remain. Arbitrary scheduling between the two Dijkstra searches does not alter that lower-bound proof; stale entries in the size count affect scheduling rather than correctness.

Full seed-1 instrumentation found **14,313,226** expansions with distance balancing, including four queries that each expanded **1,740,378** vertices. Size balancing reduced the total to **7,123,603** and eliminated those outliers. A separate unreachable-island case with a large-weight spur fell from **10,002** expansions to **3**, retaining the exact `-1` answer. This demonstrates the mechanism independently of wall-clock noise.

Local full-data development runs with the same BFS layout measured **3.210 s** for one worker, **2.435 s** for two, and **2.065 s** for four, all with the published expected hash. These are not official scores, and the worker/memory trade-off still needs a Board measurement. All cached tiny/sample datasets and the independent random-query oracle remain correctness gates. Official measurement of size balancing: pending.
