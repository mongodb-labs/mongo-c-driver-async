import argparse
import itertools
from pathlib import Path
import re
import sys

parser = argparse.ArgumentParser(
    description="Fixup #include directives to use <...> for absolute includes"
)
parser.add_argument(
    "--check",
    action="store_true",
    help="Only check the #include formats, do not change anything",
)
args = parser.parse_args()
only_check: bool = args.check


pats = [
    "**/*.c",
    "**/*.h",
    "**/*.cpp",
    "**/*.hpp",
]

dirs = ["src/", "include/"]

dirs = map(Path, dirs)
source_files = itertools.chain.from_iterable(
    itertools.chain.from_iterable((p.glob(pat) for pat in pats) for p in dirs)
)

INCLUDE_RE = re.compile(r'(\s*#include\s+)"(\w.*)"(.*)$')


def subst(fpath: Path):
    "Create a regex substitution function that prints a message for the file when a substitution is made"

    def f(mat: re.Match[str]) -> str:
        # See groups in INCLUDE_RE
        newl = f"{mat[1]}<{mat[2]}>{mat[3]}"
        print(f"{fpath}: update #include directive: {mat[0]!r} → {newl!r}")
        return newl

    return f


err = False
for fpath in source_files:
    old_lines = fpath.read_text()
    new_lines = "\n".join(
        INCLUDE_RE.sub(subst(fpath), ln) for ln in old_lines.split("\n")
    )
    if new_lines == old_lines:
        # Nothing to do
        continue
    if only_check:
        print(f"File [{fpath}] contains improper #include directives", file=sys.stderr)
        err = True
    else:
        fpath.write_text(new_lines, newline="\n")
if err:
    sys.exit(1)
