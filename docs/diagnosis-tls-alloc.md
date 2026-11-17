# Diagnosis: SSL - Memory allocation failed (-32512) at startup

## Root cause
A TLS handshake needs about 47 KB of 8-bit heap at peak (free drops from 62 KB to 13 KB during the first fetch) and
two 16.6 KB record buffers. After the first handshake a small persistent allocation splits the 61 KB block, so the
idle largest block is 34.8 KB (sometimes 40.9 or 49 KB). While another consumer is transient (UI work, socket setup)
it dips to 32.7 KB, and a handshake started there can fail. The previous guard (28 KB threshold) let those dips
through, and the single quick retry (4 s) then also failed, leaving weather/propagation idle for 2 and 5 minutes.

## Evidence (115200 serial, [HTTP]/[MEM] lines)
- Baseline, 5 boots: first fetch largest=61428, later fetches at 34804 or 32756; idle `largest=34804 minfree=45704`.
  One baseline-equivalent boot (fix build, guard 31 KB): `[HTTP] GET api.pota.app -> -1 (398 ms) largest=32756->32756`
  with `SSL - Memory allocation failed`, recovered only on the next round.
- Forced fragmentation (temporary hog, largest 30708): `www.hamqsl.com -> -1` four times, retries at +4, +8, +16, +32 s
  (31.6, 35.6, 43.7, 59.7 s uptime), success right after the hog was released. Handshake fails at 30708.
- Heap dump with `heap_caps_dump` trips the task watchdog (heap lock held while printing), so the exact persistent
  allocation was not identified.

## Fix
1. `kTlsMinBlock` 28 KB -> 33 KB (between the failing 32756/30708 and the always-working 34804). The gate defers
   2 s, up to 5 times, then goes anyway. The 32756 dip was transient in every observed boot.
2. Allocation failures now retry with doubling delay (4, 8, 16, 32, 60 s, capped) until a fetch succeeds,
   instead of a single 4 s retry (`tlsAllocRetryMs`, unit tested). Failed fetches never touch the stored data.

## Before / after (device, 4+ boots each)
| | before | after |
|---|---|---|
| failed handshakes in first 60 s | 1 in 5 boots (POTA at 32756) | 0 in 5 boots (3 of them deferred 2-4 s at 32756) |
| retry after alloc failure | once, 4 s, then 2-5 min | 4/8/16/32/60 s until success |
| idle largest block | 34804 | 34804 (unchanged) |
| minfree | ~45 KB | ~44-46 KB |
| flash | 1,481,856 B image | 1,481,888 B image |

## Not done
The 45 KB idle-largest budget is not met: it needs a persistent 26 KB-offset allocation moved or the LVGL draw
buffers (25.6 KB) shrunk; the second costs the scroll performance measured in perf-scroll-report.md.
