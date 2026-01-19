import os
import re

SRC_DIR = "src"

RED = "\033[91m"
RESET = "\033[0m"

# Pragmas
DISABLE_RE = re.compile(r"//\s*find-error\s+off")
ENABLE_RE = re.compile(r"//\s*find-error\s+on")

CALL_WITH_ERROR_RE = re.compile(
    r"""
    ^(?!\s*(ERROR_ASSERT|ASSERT|if))
    .*
    \(.*\berror\b.*\)
    \s*;
    """,
    re.VERBOSE,
)

VALID_ERROR_HANDLING_RE = re.compile(r"^\s*(ERROR_ASSERT|ASSERT|if\s*\(\s*error\b)")

IGNORED_LINE_RE = re.compile(r"^\s*(//.*)?$")


def check_file(path):
    with open(path, "r", encoding="utf-8", errors="ignore") as f:
        lines = f.readlines()

    enabled = True
    i = 0

    while i < len(lines):
        line = lines[i]

        # Handle pragmas
        if DISABLE_RE.search(line):
            enabled = False
            i += 1
            continue

        if ENABLE_RE.search(line):
            enabled = True
            i += 1
            continue

        if not enabled:
            i += 1
            continue

        if CALL_WITH_ERROR_RE.match(line):
            j = i + 1

            # Skip blank lines and comments
            while j < len(lines) and IGNORED_LINE_RE.match(lines[j]):
                j += 1

            if j >= len(lines) or not VALID_ERROR_HANDLING_RE.match(lines[j]):
                print(f"[{RED}ERROR{RESET}] {path}:{i + 1} missing error handling")

        i += 1


def main():
    for root, _, files in os.walk(SRC_DIR):
        for name in files:
            if name.endswith(".cpp"):
                check_file(os.path.join(root, name))


if __name__ == "__main__":
    main()
