# Spot table fixes (after the custom-table rewrite)

Verified on the board with `cyd_test` (`fill N` fake rows, `swipe` to drag-scroll); the screenshots were read by eye.

1. APRS rows at the bottom. `spotTableSet` called `scroll_by(-over)` whenever `lv_obj_get_scroll_bottom` was negative. A list shorter than the view is always negative, so the rows were pushed to the bottom. Now it only undoes a real scroll (`min(-over, scroll_y)`, only if `scroll_y > 0`). Affects any short list: APRS, POTA, HamAlert, DX.
2. Zebra out of sync. `onDraw` narrows `layer->_clip_area` to each cell for its label and never restored it before the next stripe fill, so every stripe was cut to the previous cell's clip and showed only in part, at 40 px chunk edges. The clip is now reset before each fill. Stripes are kept, and they line up in the static view and after two drag-scrolls.
3. DX time was `HHMM`. New `formatHmCell` (lib/core/text_cut, unit-tested) gives `HH:MM`. It is shared by DX, POTA and HamAlert. The UTC column is now 42 px and the last column 100 px, per `check_text_fit.py` (0 overflow). APRS keeps `HHMM` as before (separate code, not requested).

Checks: native tests 178 pass, firmware.bin cyd 1,473,920 B and 38pin 1,473,424 B (limit 1,550,000), idle `mem` in the harness lvfree 31.8 KB, largest 34.8 KB (harness build; no heap-affecting change was made).
