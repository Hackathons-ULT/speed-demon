# Exact local CSV partition anchors

Both text columns lie between commas. A quoted field's opening delimiter therefore immediately follows a comma, and its closing delimiter immediately precedes one. Unquoted fields cannot contain a quote. Literal quotes in quoted data are doubled.

Consider a maximal consecutive run of quote bytes. An interior run consists entirely of doubled quotes and has even length. A run containing only the opening delimiter plus initial literal quotes, or trailing literal quotes plus the closing delimiter, has odd length. A whole field consisting entirely of literal quotes, including its two delimiters, has even length. Thus every odd run has exactly one structural delimiter.

If the preceding nonquote byte is a comma and the following nonquote byte is not, the odd run must be an opening delimiter; the state after the complete run is inside a quoted field. If only the following byte is a comma, it must be closing; the state afterward is outside. A comma on both sides is ambiguous and is skipped. This proof also covers literal commas and quotes at either end of the decoded value.

The cut search begins at its byte target. When that target falls inside a quote run, the search first includes the entire prefix of the maximal run using the full input bounds. It also examines the run's entire suffix. Starting from an unambiguous run's known state, every later quote toggles parity, including both bytes of each escaped pair. The next LF at even parity terminates a logical record. Its successor is therefore a safe cut at or after the target; end of input is also safe when the final record lacks a terminator. Cuts are monotonic, and repeated cuts simply leave empty worker ranges. No records are omitted or duplicated.

Only quote runs discovered within a 64 KiB search window are considered. A run itself may extend outside that window. If the search finds no anchor, the algorithm discards its partial cut list and computes every cut with a full scalar parity scan from the known record-stream start. This preserves correctness for empty, quote-only, comma-only, entirely unquoted and otherwise ambiguous inputs. It does not assume bounded text length, lowercase note contents, or at most four lines. Generic quoted CR, LF, UTF-8, and BOM data remain data; only outside LF terminates a record.

## Development verification

GCC 16.1.0 MinGW-w64, C++17, `-O3 -DNDEBUG -Wall -Wextra -static -pthread`; the solution compiles without warnings. All cached official reference bytes match: full seed 1, sample seeds 1 and 7, tiny seeds 1, 7 and 42. Full SHA-256 remains `d8088b1cd42ba1338b1345b107b55db7b96ef1f5066f3ff64fd26a8912a0f536`.

Official worked/rules fixtures, 6,000 independently generated generic CSV rows, and a hand-computed 110,001-name growth case also match. Direct cut checks use independently known record ends on 39 layouts, for every worker count from one through twelve. Cases include missing final terminators, BOMs and embedded CR/LF, long multiline notes containing fake numeric records, empty/comma-only/quote-only text, 150,000-byte quote runs, and 150,000-byte ambiguous comma fields. Every cut is a known record boundary at or after its target; all ranges are monotonic and collectively cover the full record stream.

The full public dataset, generic mixed-CSV fuzz, and 110,001-name case also produce identical bytes with one, two, four, six, and twelve workers. Six repeated twelve-worker runs of 20,000 empty-name records retain exactly one distinct value and the correct totals.

An eight-round alternating full-seed-1 comparison was serialized with the shared benchmark lock. Uninstrumented Windows median elapsed time was 0.595994 s for the unchanged parallel baseline and 0.554993 s for this candidate (6.88% lower). A separate instrumented experiment was effectively tied overall: the old prepass's input warming offset much of the local cut saving. These are development measurements with a Windows input copy, not Linux scoring measurements. The replacement removes five physical source lines overall. Linux CI and a genuine Board coefficient comparison are required before retaining the candidate; no new official result is claimed here.

