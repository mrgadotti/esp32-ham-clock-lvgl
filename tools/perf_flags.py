# PlatformIO pre-script: the build is -Os; re-enable -O2 only on the rendering hot path
# (LVGL software blend/draw, refresh, TFT_eSPI) so Settings scroll fps stays up.
# Library envs are cloned from this env later, so the middleware must be added in a pre: script.
import fnmatch
import os

Import("env")  # noqa: F821

# fnmatch patterns on the forward-slash source path.
HOT = (
    "*/lvgl/src/draw/sw/blend/lv_draw_sw_blend.c",
    "*/lvgl/src/draw/sw/blend/lv_draw_sw_blend_to_rgb565.c",
    "*/lvgl/src/draw/sw/lv_draw_sw_fill.c",
    "*/lvgl/src/draw/sw/lv_draw_sw_letter.c",
    "*/lvgl/src/draw/sw/lv_draw_sw_mask*.c",
    "*/lvgl/src/draw/sw/lv_draw_sw_border.c",
    "*/lvgl/src/draw/sw/lv_draw_sw_img.c",
    "*/lvgl/src/core/lv_refr.c",
    "*/TFT_eSPI/TFT_eSPI.cpp",
)


def o2(e, node):  # two-argument form: PlatformIO passes the env being built (a lib's own env)
    path = os.path.normpath(node.srcnode().get_path()).replace("\\", "/")  # same match on Windows
    if not any(fnmatch.fnmatchcase(path, pat) for pat in HOT):
        return node
    return e.Object(node, CCFLAGS=e["CCFLAGS"] + ["-O2"])


env.AddBuildMiddleware(o2)  # noqa: F821
