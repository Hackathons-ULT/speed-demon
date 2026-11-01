# Track A: performance log

Every step from the reference to your final version, each with the live-board run ID that measured it. Judges check that the run IDs exist and that each change moved the way you say it did.

| Step | Change | Run ID | T (s) | M (MB) | Points |
|---|---|---|---|---|---|
| 0 | The reference, unchanged | | 32.05 | 228 | 0 |
| 1 | Replaced the reference with a stdlib-only optimised parser: no `datetime` (hour = first 13 bytes of the timestamp), one `bytes.split()` per ~256 KB chunk, forked workers pulling line-aligned tasks from a pipe, whole-hour segments found with `bisect`, a C-level 5xx scan, compact `array('I')` latency buffers, and parallel rank selection (histogram or partial sort) with a fallback to the reference for anything unexpected. | run-d501b265e44a | 1.88 | 801 | 18.34 |
