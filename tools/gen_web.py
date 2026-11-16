# PlatformIO pre-script: gzip web/config.html into src/net/config_html.h (gitignored).
# Regenerates only when the HTML changed; writes an empty placeholder if the page is missing.
import gzip
import hashlib
import os
Import("env")  # noqa: F821

root = env["PROJECT_DIR"]  # noqa: F821
src = os.path.join(root, "web", "config.html")
out = os.path.join(root, "src", "net", "config_html.h")
os.makedirs(os.path.dirname(out), exist_ok=True)

if os.path.exists(src):
    html = open(src, "rb").read()
    gz = gzip.compress(html, 9, mtime=0)  # mtime 0: reproducible
    tag = "// sha1:" + hashlib.sha1(html).hexdigest() + "\n"
else:
    gz, tag = b"\0", "// sha1:none\n"

if not (os.path.exists(out) and open(out).readline() == tag):
    body = ",".join(str(b) for b in gz)
    with open(out, "w") as f:
        f.write(tag + "#pragma once\n#include <Arduino.h>\n"
                "static const uint8_t CONFIG_HTML_GZ[] PROGMEM = {" + body + "};\n"
                "static const size_t CONFIG_HTML_GZ_LEN = " + str(0 if gz == b"\0" else len(gz)) + ";\n")
