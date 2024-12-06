import argparse
import shutil
import subprocess
import sys
import tempfile
from contextlib import ExitStack
from datetime import datetime
from pathlib import Path
from typing import Iterable, Sequence

import sphinx.cmd.build

THIS_FILE = Path(__file__).resolve()
TOOLS_DIR = THIS_FILE.parent
REPO_DIR = TOOLS_DIR.parent


def run(cmd: Iterable[str | Path], *, cwd: Path | None = None) -> None:
    subprocess.check_call(list(map(str, cmd)), cwd=cwd)


def main(argv: Sequence[str]):
    parser = argparse.ArgumentParser(
        description="""Build the amongoc documentation and optionally commit/push the docs"""
    )
    parser.add_argument(
        "--clean-repo-branch",
        metavar="<branch-name>",
        help="If specified, a clean copy of the repository will be cloned at the given branch"
        " to build the docs rather than using the working copy",
    )
    parser.add_argument(
        "--commit-branch", metavar="<branch-name>", help="Branch to which the result should be committed"
    )
    parser.add_argument(
        "--scratch-dir",
        metavar="<directory>",
        help="Directory where build results will be written",
        type=Path,
        default=REPO_DIR / "_build/docs-build",
    )
    parser.add_argument(
        "--remote",
        metavar="<uri>",
        help="The remote repository URI",
        default="git@github.com:mongodb-labs/mongo-c-driver-async.git",
    )
    parser.add_argument(
        "--skip-remote-clone",
        help="Skip cloning from the remote. Assume the repository is already present.",
        action="store_true",
    )
    parser.add_argument(
        "--delete-prior",
        help="Delete the prior scratch directory if it exists",
        action="store_true",
    )
    parser.add_argument(
        "--push",
        help="Push to the remote branch after building the documentation",
        action="store_true",
    )
    args = parser.parse_args(argv)
    clean_repo_branch: str | None = args.clean_repo_branch
    commit_branch: str | None = args.commit_branch
    scratch_dir: Path = args.scratch_dir
    remote: str = args.remote
    delete_prior: bool = args.delete_prior
    skip_remote_clone: bool = args.skip_remote_clone
    push: bool = args.push

    with ExitStack() as stack:
        if clean_repo_branch is None:
            build_repo_root = REPO_DIR
        else:
            build_repo_root = Path(stack.enter_context(tempfile.TemporaryDirectory(suffix=".amongoc-docs-clone")))
            print(f"Cloning into temporary dir [{build_repo_root}]")
            run(
                [
                    "git",
                    "clone",
                    "--quiet",
                    "--depth=1",
                    f"--branch={clean_repo_branch}",
                    REPO_DIR.as_uri(),
                    build_repo_root,
                ]
            )

        if commit_branch is not None:
            if delete_prior:
                try:
                    shutil.rmtree(scratch_dir)
                except FileNotFoundError:
                    pass
            if next(iter(scratch_dir.iterdir()), None) is not None:
                raise RuntimeError(f"Scratch directory [{scratch_dir}] is not empty")
            # Clone the remote
            if not skip_remote_clone:
                print(f"Cloning existing pages into [{scratch_dir}]")
                run(["git", "clone", "--quiet", "--depth=1", f"--branch={commit_branch}", remote, scratch_dir])
            print("Wiping prior content...")
            run(["git", "-C", scratch_dir, "rm", "--quiet", "-rf", "."])

        print("Executing sphinx-build...")
        sphinx.cmd.build.build_main(
            [
                "-W",
                "-jauto",
                "-qa",
                "-bdirhtml",
                f'--doctree-dir={REPO_DIR/"_build/docs-build.doctree"}',
                str(build_repo_root / "docs"),
                str(scratch_dir),
            ]
        )
        scratch_dir.joinpath(".nojekyll").write_bytes(b"")
        print(f"Build pages are in [{scratch_dir}]")
        if commit_branch is not None:
            scratch_dir.joinpath(".buildinfo").unlink()
            print("Staging...")
            run(["git", "-C", scratch_dir, "add", "."])
            today = datetime.now().isoformat()
            print("Committing...")
            run(["git", "-C", scratch_dir, "commit", "--quiet", "-m", f"Documentation build [{today}]"])
            if push:
                run(["git", "-C", scratch_dir, "push", remote, f"{commit_branch}:{commit_branch}"])


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
