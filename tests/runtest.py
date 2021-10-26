import difflib
import os
import subprocess
import sys

# Special exit code that tells automake that a test has been skipped,
# eg. when we needs a newer HarfBuzz version than what is available.
SKIP_EXIT_STATUS = 77

def cmd(command):
    print(command)
    p = subprocess.Popen(command, stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE, universal_newlines=True)
    out, err = p.communicate()
    print(f"Error:\n{err}")
    return out.strip(), p.returncode


def diff(expected, actual):
    expected = expected.splitlines(1)
    actual = actual.splitlines(1)
    diff = difflib.unified_diff(expected, actual)
    return "".join(diff)


srcdir = sys.argv[1]
testtool = sys.argv[2]

fails = 0
skips = 0
for filename in sys.argv[3:]:
    print("Testing %s..." % filename)

    with open(filename, encoding="utf-8") as fp:
        lines = [l.strip("\n") for l in fp.readlines()]
    font = lines[0]
    text = lines[1].replace(r'\r', "\r").replace(r'\n', '\n')
    text = " ".join(f"{b:04X}" for b in text.encode("utf-8"))
    opts = lines[2] and lines[2].split(" ") or []
    expected = "\n".join(lines[3:])
    if "," in font:
        fonts = []
        for f in font.split(","):
            if f.endswith(".ttf"):
                f = os.path.join(srcdir, f)
            fonts.append(f)
        fonts = ",".join(fonts)
        command = [testtool, "--bytes", text] + opts + ["--fonts", fonts]
    else:
        font = os.path.join(srcdir, font)
        command = [testtool, "--bytes", text] + opts + ["--font", font]

    actual, ret = cmd(command)
    expected = expected.strip()
    actual = actual.strip()
    if ret == SKIP_EXIT_STATUS:
        # platform is missing a requirement to run the test, eg. old HarfBuzz
        skips += 1
    elif ret:
        print(f"Error code returned: {ret}")
        fails += 1
    elif actual != expected:
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
