import os
import re

SRC_DIR = "src"

# ANSI escape codes for colors
RED = "\033[91m"
RESET = "\033[0m"

# Matches a function call that includes `error` as an argument
# but excludes lines starting with ERROR_ASSERT, ASSERT, or if
CALL_WITH_ERROR_RE = re.compile(
    r"""
    ^(?!\s*(ERROR_ASSERT|ASSERT|if))  # skip lines starting with ERROR_ASSERT, ASSERT, or if
    .*                                 # anything before
    \(.*\berror\b.*\)                  # parentheses with error inside
    \s*;                               # ending semicolon
    """,
    re.VERBOSE,
)

# Matches ERROR_ASSERT, ASSERT, or if(error ...) as a valid error handling
VALID_ERROR_HANDLING_RE = re.compile(r"^\s*(ERROR_ASSERT|ASSERT|if\s*\(\s*error\b)")

# Matches blank lines or comments
IGNORED_LINE_RE = re.compile(r"^\s*(//.*)?$")


def check_file(path):
    with open(path, "r", encoding="utf-8", errors="ignore") as f:
        lines = f.readlines()

    i = 0
    while i < len(lines):
        line = lines[i]

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
