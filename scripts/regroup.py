from __future__ import annotations

import argparse
import json
import subprocess
import sys
from datetime import datetime
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent
OUTPUT_DIR = SCRIPT_DIR / "output"
PLAN_PATH = OUTPUT_DIR / "plan.json"
BACKUP_DIR = OUTPUT_DIR / "backup"


def die(msg: str) -> None:
    print(f"error: {msg}", file=sys.stderr)
    raise SystemExit(1)


def git(*args: str) -> str:
    proc = subprocess.run(["git", "-C", str(REPO_ROOT), *args], capture_output=True)
    if proc.returncode != 0:
        die(f"git {' '.join(args)} failed:\n{proc.stderr.decode('utf-8', 'replace').strip()}")
    return proc.stdout.decode("utf-8", "replace")


def main() -> None:
    parser = argparse.ArgumentParser(description="Regroup a plan group's junk commits.")
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--to-next", type=int, metavar="ID", help="move junk to the next group")
    action.add_argument("--standalone", type=int, metavar="ID", help="make junk its own commit")
    parser.add_argument("--count", type=int, help="with --to-next: move only the last N junk")
    parser.add_argument("--message", help="commit message (required with --standalone)")
    args = parser.parse_args()

    groups = json.loads(PLAN_PATH.read_text(encoding="utf-8"))
    target_id = args.to_next or args.standalone
    index = next((i for i, g in enumerate(groups) if g["id"] == target_id), None)
    if index is None:
        die(f"group #{target_id} not found")
    group = groups[index]
    junk = list(group["junk"])
    if not junk:
        die(f"group #{target_id} has no junk commits to move")

    if args.to_next is not None:
        if index + 1 >= len(groups):
            die("there is no next group")
        if args.count is not None:
            if not 1 <= args.count <= len(junk):
                die(f"--count must be between 1 and {len(junk)}")
            junk = junk[-args.count:]
        moved = set(junk)
        group["shas"] = [s for s in group["shas"] if s not in moved]
        group["junk"] = [s for s in group["junk"] if s not in moved]
        nxt = groups[index + 1]
        nxt["shas"] = junk + nxt["shas"]
        nxt["junk"] = junk + nxt["junk"]
        nxt["leading"] = True
        touched = [group, nxt]
        summary = f"moved {len(junk)} junk commit(s) from #{target_id} to #{nxt['id']}"
    else:
        if not args.message:
            die("--standalone requires --message")
        moved = set(junk)
        group["shas"] = [s for s in group["shas"] if s not in moved]
        group["junk"] = []
        fresh = {"base": junk[0], "shas": junk, "junk": [], "leading": False,
                 "message": args.message}
        groups.insert(index + 1, fresh)
        touched = [group, fresh]
        summary = f"split {len(junk)} junk commit(s) out of #{target_id} into a new commit"

    # Hints were computed for the old grouping; drop them so nobody trusts stale data.
    for item in touched:
        item.pop("hint", None)
        item["review"] = True
    for number, item in enumerate(groups, start=1):
        item["id"] = number

    expected = git("rev-list", "--reverse", "HEAD").split()
    if [s for g in groups for s in g["shas"]] != expected:
        die("regroup would break history coverage/order; plan.json left untouched")

    BACKUP_DIR.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    (BACKUP_DIR / f"plan.{stamp}.json").write_bytes(PLAN_PATH.read_bytes())
    PLAN_PATH.write_text(json.dumps(groups, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")

    print(summary)
    print(f"plan now has {len(groups)} commits (was {len(groups) - (1 if args.standalone else 0)})")
    print("note: review.md is now stale; re-run gen_plan.py only if you accept losing edits")


if __name__ == "__main__":
    main()