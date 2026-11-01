# Track A: performance log

Every step from the reference to your final version, each with the live-board run ID that measured it. Judges check that the run IDs exist and that each change moved the way you say it did.

| Step | Change | Run ID | T (s) | M (MB) | Points |
|---|---|---|---|---|---|
| 0 | The reference, unchanged | | 32.05 | 228 | 0 |
| 1 | Replaced the reference with a stdlib-only optimised parser: no `datetime` (hour = first 13 bytes of the timestamp), one `bytes.split()` per ~256 KB chunk, forked workers pulling line-aligned tasks from a pipe, whole-hour segments found with `bisect`, a C-level 5xx scan, compact `array('I')` latency buffers, and parallel rank selection (histogram or partial sort) with a fallback to the reference for anything unexpected. | run-d501b265e44a | 1.88 | 801 | 18.34 |


## November 1: compact C++ candidate (Codex assisted)

The approach and correctness argument are in README.md. The new solution uses no direct runtime dependencies. All new solution code was written during the event on November 1; the organiser's references and earlier teammate work predate this change and are preserved in Git history.

Local development check: Windows x86-64, GCC 16.1.0 MinGW-w64, C++17 -O3 -DNDEBUG -static. Full seed-1 output matched both the unmodified official reference and published SHA-256 `495ca09682b9673ef836a506496177680bb4a3af54255174befddb1f83481cd3`. The final local solution run took 3.257132 s. This is one development run, not a controlled scoring measurement. Memory has not been measured. No official score is inferred from this time.

Differential checks also pass on tiny seeds 1, 7, 42 and sample seeds 1, 7. Additional fixtures cover CSV quoting, BOMs within values, end-of-file, status/uint32 boundaries, empty input, and 12,500 independently computed graph-query answers. The three official reference test suites pass 43 tests. `validation.json` records the evidence.

Official Board result for the C++ step, commit `738832835b87dd40b567dac9cfd6ac4db2e9aa06`:

| Run ID | T (s) | Peak memory (bytes) | Source lines | D | K | Machine points |
|---|---:|---:|---:|---:|---:|---:|
| `run-d530eae56018` | 1.007284 | 65634304 | 58 | 1 | 7.197606 | 42.859403 |

The official run passed check, build, correctness, timeout and sandbox gates on the full public seed-1 dataset. This is the live practice Board, not the final holdout score. See `board-results.json` for the recorded receipt. Human review of this explanation remains necessary; AI assistance does not establish human understanding by itself.

The earlier scored Python implementation is preserved under `experiments/track-a-python/`; its existing performance log above is unchanged. The compact candidate avoids the per-process parsing/ranking machinery and stores only one 32-bit latency per request. Whether it improves the coefficient must be established on the same Board hardware.


## Iteration 3: memory layout and throughput experiment

Genuine baseline: `run-19c6d1657d8d` at `7491483ff1e0fe3f3d2b84f3e610310207751da6`, T 0.952371 s, peak 65605632 bytes, K 7.402849, machine points 43.469944; all gates passed.

The candidate maps input read-only on Linux, interns endpoint views once, and uses 72 direct hour buckets per endpoint instead of constructing an hour/endpoint string key per record. The fixed time span is an explicit specification guarantee shared by every seed, not an input fingerprint. Timestamp prefixes are retained for exact output. A bounded stack buffer plus locale-independent integer conversion reduces output formatting overhead. Full sorting and exact nearest ranks remain.

Handwritten edge fixtures and all 12,500 independently computed random shortest-path answers pass, including parallel, zero-weight, unreachable and over-uint32 paths. Existing seed-1 full data and multiple tiny/sample seeds remain correctness gates. Expanded Linux CI will additionally exercise the POSIX mapping path and offline container sandbox.

Official Board result for iteration 3: pending. No timing, memory, coefficient or run ID for this new candidate is claimed yet.

## Track A agent: disjoint endpoint shards and progressive rank selection

The coordinator verified the mapped-input iteration-3 baseline on the official Board: `run-6e7dcf2a13d6`, T **0.481109 s**, peak **35,405,824 bytes**, S **154**, D **1**, K **10.177465**, machine points **50.381980**; all gates passed. This is the comparison baseline for the candidate below.

Four workers scan the shared read-only mapping and own disjoint endpoint-hash shards. Only a record's owner parses its time/status/latency and writes its group. Every latency is stored once; no group locks, atomic counters or cross-worker latency copying are needed. Groups up to 64 items retain sorting; larger groups use three progressive exact nearest-rank selections. README.md explains the ownership and rank invariants.

Local Windows GCC16.1 validation matched the unmodified official reference on full seed1; samples1/7/42; tiny seeds1/7/42/99/1009; empty input; and a 10,201-line randomized contract fixture with duplicates, all 72 hours, status values spanning100–599, zero/UINT32_MAX latencies, max-length endpoints and an isolated101-item rank group. The randomized fixture was repeated three times with identical bytes. The full seed1 output remains `495ca09682b9673ef836a506496177680bb4a3af54255174befddb1f83481cd3`.

Alternating full-data development runs under concurrent host load gave base501 median **3.217355 s** and this candidate median **2.509646 s** (three runs each). A smaller selection-only experiment gave about8% local median improvement. Host timings were noisy, so neither observation is an official speedup or a coefficient claim. The extra scans, code and rank fields are priced trade-offs and need a genuine Board comparison.

Official Board result for the four-shard candidate: **pending**. Do not replace the baseline based on local time alone; the coordinator should retain it only after Linux sandbox/correctness checks and a measured K improvement.

## Track A agent: contiguous parse, flat latency storage, parallel output

The four-shard candidate subsequently passed the genuine Board: T **0.249200 s**, peak **36,372,480 bytes**, S **200**, K **13.524502**, machine points **56.556065**. The official run ID is `run-c3f2089a89a2` at commit `3733f64dcbb74ead8e38c4034a51b5fee6adc4a6`.

The next experiment replaces repeated whole-file endpoint scans with six contiguous line-aligned parser ranges and immutable concurrently interned keys. Atomic counts plus prefix sums and atomic scatter cursors create one flat latency array without per-group vector growth. Progressive exact ranks are selected in parallel. Six formatters create consecutive hour ranges in private strings, which the main thread writes in order. README.md gives the publication, cursor-ownership and join invariants.

The packed-record variant is byte-correct against the official reference for full seed1; samples1/7/42; tiny1/7/42/99/1009; empty input; a randomized10,201-line contract fixture repeated three times; a100,000-record single-group contention fixture repeated ten times; and the common interning/scatter core passed forced endpoint-hash collisions repeated ten times (test binary changes only the initial hash slot). Output for full1 is unchanged at `495ca09682b9673ef836a506496177680bb4a3af54255174befddb1f83481cd3`.

Alternating Windows full-data medians (three runs each) were **1.324640 s** for base501 and **0.542404 s** for packed records plus parallel output. A diagnostic phase profile before parallel output measured load0.367s / parse0.102s / prefix0.001s / scatter0.018s / rank0.010s / serial output0.071s. These are local development observations under variable host load, not official score claims. The packed buffer trades peak memory for parsing once; glibc arena retention can affect the real peak.

Official Board result for the packed-throughput candidate: **pending**. Linux CI and genuine K comparison remain required.

## Lower-memory throughput variant: count then reparse

This default variant removes the eight-byte temporary record array. Six workers count groups/errors in their line-aligned ranges, a prefix sum creates exact flat latency regions, then the same six ranges are reread to parse and atomically scatter latencies. The mapped input is immutable, so endpoint keys and group identities are unchanged between passes. Key publication uses explicit acq_rel/acquire CAS ordering; a single atomic claim per hour supplies its output prefix, read only after worker joins. Ranks and output formatting keep the parallel phase ownership described above.

This exact variant passed all cached full/sample/tiny oracle comparisons and fresh samples42/tiny99/1009, empty input and the randomized contract fixture. It also passed the100,000-record single-group contention case ten times and the forced-all-endpoints-collide version ten times. All comparisons are whole-file byte equality against the unmodified official reference. The full1 SHA-256 remains `495ca09682b9673ef836a506496177680bb4a3af54255174befddb1f83481cd3`.

Three alternating local Windows full-data runs each gave base501 median **1.426016 s** and two-pass median **0.543406 s**. Its diagnostic phase profile was load0.174s / count0.082s / prefix0.0015s / parse-and-scatter0.071s / rank0.011s / parallel format-and-write0.015s. Packed records plus parallel output had essentially the same local median (**0.542404 s**) in a separate run, but retain roughly25MB of temporary records and may retain allocator pages; the two-pass variant avoids that allocation. Metadata, latencies and formatting suggest roughly30–35MB auxiliary peak for the two-pass version, an analytical estimate rather than an observed Board memory value.

Official Board result for the two-pass throughput variant: **pending**. The coordinator should compare real time, peak, source and K with the verified four-shard baseline before keeping it. No official score is claimed from these local measurements.

## Verified four-agent Board receipts

These exact practice Board receipts resolve the pending measurements above. All check, build, correctness, timeout and sandbox gates passed.

| Commit | Run ID | Time (s) | Peak bytes | Source | K | Machine points |
|---|---|---:|---:|---:|---:|---:|
| `3733f64` | `run-c3f2089a89a2` | 0.249200 | 36372480 | 200 | 13.524502 | 56.556065 |
| `ee4c7f9` | `run-857a5cabbd8f` | 0.100145 | 42950656 | 254 | 19.909976 | 64.953537 |

The measured candidate is retained. Linux validation, including offline builds and sandbox execution, passed at [CI 41313474697](https://github.com/Hackathons-ULT/speed-demon/actions/runs/41313474697). Holdout correctness and final human judging remain separate from these public practice measurements.
## Track A independent challenge: release storage at its last phase

The coordinator verified the two-pass baseline at commit `ee4c7f93fecc931685df611fc7bff97e3aadfd6d`: official run `run-857a5cabbd8f`, T **0.100145 s**, peak **42,950,656 bytes**, S **254**, K **19.909976**, machine points **64.953537**; all gates passed. This is the comparison baseline, not a result for the lifetime candidate.

The harness measures the run's parent cgroup `memory.peak`, and output-file pages are charged there. The previous candidate kept latency values, prefix offsets and groups alive throughout output. This small candidate releases values/offsets immediately after rank selection joins, groups after formatting joins, and each formatted string after its synchronous write. Three reviewers inspected those phase boundaries; no view, cursor or worker uses released storage. Exact parsing, interning, ranks and formatting remain unchanged.

Whole-file comparisons against retained unmodified official references pass for full seed1, samples1/7 and tiny1/7/42. Full output SHA-256 remains `495ca09682b9673ef836a506496177680bb4a3af54255174befddb1f83481cd3`. Development commands and receipts are retained in the isolated `agents/a-challenger` workspace, outside the submission.

Four alternating Windows full-data runs per version gave baseline median **0.557931 s** and candidate median **0.585992 s**, with one candidate outlier at 1.057128 s. No local speed improvement is claimed. Two Windows peak-working-set observations per version were **273,661,952 / 273,637,376 bytes** for baseline and **263,225,344 / 263,221,248 bytes** for the candidate. Windows includes a 238.8 MB private input copy and does not measure the Linux cgroup's output page cache, so these are development memory observations only.

Official Board result for this storage-lifetime candidate: **pending**. Linux CI and a genuine coefficient comparison must price any time/source cost against measured peak-memory savings.

## Verified lifetime result and rejected alternatives

The lifetime change at `3c2fd576c90e3396bcdc4d04ce8677021436188d` passed official run `run-5203d23e4e97`: T **0.122014 s**, M **29028352 bytes**, S **259**, K **19.450919**, machine points **64.447006**, all gates passed. Compared with `run-857a5cabbd8f` (0.100145 s, 42,950,656 bytes, K19.909976), memory decreased but the first-run coefficient is lower. This is a tradeoff under comparison, not a verified score gain; a repeat is required before retention. The 6,775-row shuffled fixture also matches the unmodified official reference and passes Linux CI.

Independent alternatives were measured and rejected before integration. Per-worker count/error arrays added 14,155,776 bytes of metadata without a consistent speed gain (four alternating Windows full-data runs: baseline median0.593316 s, private0.601527 s). A count-only alternative was inconclusive. Hardware CRC32C hashing passed 8,000 independent guard-page comparisons and full output checks, but four alternating full-data runs measured0.745396 s versus0.688773 s for the existing hash. It also added source cost. These separate development experiments used different host conditions and are not official score claims. Neither alternative changed the retained code.

Distribution-aware planning remains exact: seed-dependent Zipf ranks do not identify a fixed hot endpoint; latency-tail shape cannot justify clipping or approximate ranks; interleaved shards provide no bound on disorder. The implementation retains all hour groups and full uint32 latencies.

## Identical-code repeat: retain lifetime cleanup

Commit `3885b0234e7da070a64dd4cd8b15e2e67d5ec123` changes only documentation/receipts from `3c2fd57`; the implementation is byte-identical. Official repeat `run-2dc57cb646ef` passed every gate: T **0.101086 s**, M **28717056 bytes**, S **259**, K **21.415881**, machine points **66.536797**. The baseline was0.100145 s /42,950,656 bytes /K19.909976. The first lifetime run's0.122014 s /29,028,352 bytes /K19.450919 is retained above, not discarded. The repeat restores comparable runtime while confirming the lower memory; retain this small lifetime change. Public practice runs remain noisy and do not establish final holdout performance.

## Worker-count experiment: eight workers

This candidate changes only `Workers` from six to eight. Every input range, rank group and output-hour range still has exactly one owner in each parallel phase. The existing lifetime releases and exact uint32/parser/collision behavior remain unchanged. More threads can cost stack/allocator memory, so this is a Board experiment rather than a claimed coefficient gain.

A serialized Windows GCC16.1 full-seed1 sweep, with four alternating runs per configuration, measured medians: three workers0.722648s, four0.636345s, six0.601167s, eight0.532784s, twelve0.562270s. A separate six-run-per-configuration diagnostic measured six-worker median0.583419s and eight-worker0.563659s. Median count/prefix and scatter phases were0.115615/0.062053s versus0.099069/0.055734s respectively. The Windows private input copy, filesystem variation and host scheduling prevent treating these as official timing or K measurements. Every full output matched SHA-256 `495ca09682b9673ef836a506496177680bb4a3af54255174befddb1f83481cd3`.

Eight workers also matched whole-file official-reference outputs for cached tiny1/7/42 and samples1/7, fresh tiny7919 and sample20261101, empty input, and the deterministic6,775-row adversarial fixture. The fixture is independently checked against the official reference. Local scripts and raw evidence remain outside the submission at `agents/a-primary2/.validation/bench-workers-compact.json`, `profiles-compact.json` and `validation-compact-stream.json`.

Separate compact-endpoint metadata and bounded-output pipeline prototypes remain experimental. In six alternating runs each, baseline0.629041s, compact0.714149s and compact plus bounded writer0.665136s did not show a reliable speed gain. Windows peak private bytes were261,369,856 /259,043,328 /256,477,184 respectively, including the input copy. Their memory effect on Linux cgroup scoring is unverified; neither prototype is included in this worker-count candidate.

Official eight-worker result: pending native Linux CI and genuine Board comparison. Source/dependency measurement must remain unchanged apart from any formatter effects; no new runtime package dependency was introduced.
