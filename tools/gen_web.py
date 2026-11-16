# PlatformIO pre-script: placeholder web header until web/config.html exists (Task 9).
import os
Import("env")  # noqa: F821

root = env["PROJECT_DIR"]  # noqa: F821
if not os.path.exists(os.path.join(root, "web", "config.html")):
    out = os.path.join(root, "src", "net", "config_html.h")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, "w") as f:
        f.write("#pragma once\n#include <Arduino.h>\n"
                "static const uint8_t CONFIG_HTML_GZ[] PROGMEM = {0};\n"
                "static const size_t CONFIG_HTML_GZ_LEN = 0;\n")
