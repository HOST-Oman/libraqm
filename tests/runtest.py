from __future__ import print_function

import ast
import difflib
import os
import subprocess
import sys
import io

# Special exit code that tells automake that a test has been skipped,
# eg. when we needs a newer HarfBuzz version than what is available.
SKIP_EXIT_STATUS = 77

def cmd(command):
    p = subprocess.Popen(command, stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    p.wait()
    print(p.stderr.read().decode("utf-8"))
    return p.stdout.read().decode("utf-8").strip(), p.returncode


def diff(expected, actual):
    expected = expected.splitlines(1)
    actual = actual.splitlines(1)
    diff = difflib.unified_diff(expected, actual)
    return "".join(diff)


srcdir = os.environ.get("srcdir", ".")
builddir = os.environ.get("builddir", ".")
testtool = os.path.join(builddir, "raqm-test")

fails = 0
skips = 0
for filename in sys.argv[1:]:
    print("Testing %s..." % filename)

    with io.open(filename, newline="") as fp:
        lines = [l.strip("\n") for l in fp.readlines()]
    font = lines[0]
    text = ast.literal_eval("'%s'" % lines[1])
    opts = lines[2] and lines[2].split(" ") or []
    expected = "\n".join(lines[3:])
    if "," in font:
        fonts = []
        for f in font.split(","):
            if f.endswith(".ttf"):
                f = os.path.join(srcdir, f)
            fonts.append(f)
        fonts = ",".join(fonts)
        command = [testtool, "--text", text] + opts + ["--fonts", fonts]
    else:
        font = os.path.join(srcdir, font)
        command = [testtool, "--text", text] + opts + ["--font", font]

    actual, ret = cmd(command)
    expected = expected.strip()
    actual = actual.strip()
    if ret == SKIP_EXIT_STATUS:
        # platform is missing a requirement to run the test, eg. old HarfBuzz
        skips += 1
    elif ret or actual != expected:
        print(diff(expected, actual))
        fails += 1

if fails:
    print("%d tests failed." % fails)
    sys.exit(1)
elif skips:
    print("%d tests skipped." % skips)
    sys.exit(SKIP_EXIT_STATUS)
else:
    print("All tests passed.")
    sys.exit(0)
