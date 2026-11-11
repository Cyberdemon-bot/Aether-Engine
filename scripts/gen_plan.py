from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from collections import Counter
from datetime import datetime
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent
OUTPUT_DIR = SCRIPT_DIR / "output"
BACKUP_DIR = OUTPUT_DIR / "backup"

DEFAULT_JUNK_REGEX = r"^(\.+|nothing|try a bit|update app|wip|tmp)$"
LONG_RUN = 5  # runs this long deserve a manual look regardless of file overlap

_EMPTY_TREE: str | None = None


# --------------------------------------------------------------------------- #
# git helpers (always executed inside REPO_ROOT)
# --------------------------------------------------------------------------- #
def die(msg: str) -> None:
    print(f"error: {msg}", file=sys.stderr)
    raise SystemExit(1)


def warn(msg: str) -> None:
    print(f"warning: {msg}", file=sys.stderr)


def git(*args: str) -> str:
    proc = subprocess.run(["git", "-C", str(REPO_ROOT), *args], capture_output=True)
    if proc.returncode != 0:
        stderr = proc.stderr.decode("utf-8", "replace").strip()
        die(f"git {' '.join(args)} failed:\n{stderr}")
    return proc.stdout.decode("utf-8", "replace")


def git_lines(*args: str) -> list[str]:
    return [line for line in git(*args).splitlines() if line]


def git_names(*args: str) -> set[str]:
    """Read a NUL-separated path list (safe for spaces / unicode in filenames)."""
    return {name for name in git(*args).split("\0") if name}


def empty_tree() -> str:
    global _EMPTY_TREE
    if _EMPTY_TREE is None:
        proc = subprocess.run(["git", "-C", str(REPO_ROOT), "mktree"],
                              input=b"", capture_output=True, check=True)
        _EMPTY_TREE = proc.stdout.decode().strip()
    return _EMPTY_TREE


def parent_of(sha: str) -> str | None:
    parts = git("rev-list", "--parents", "-n", "1", sha).split()
    return parts[1] if len(parts) > 1 else None


def commit_files(sha: str) -> set[str]:
    """Files touched by one commit; --root also covers the very first commit."""
    return git_names("diff-tree", "--root", "--no-commit-id", "--name-only", "-r", "-z", sha)


# --------------------------------------------------------------------------- #
# preflight / loading
# --------------------------------------------------------------------------- #
def preflight(ref: str) -> None:
    if not (REPO_ROOT / ".git").exists():
        die(f"{REPO_ROOT} is not a git repository (expected <repo-root>/.git)")
    top = Path(git("rev-parse", "--show-toplevel").strip()).resolve()
    if top != REPO_ROOT:
        die(f"git toplevel is {top}, but this script sits in {SCRIPT_DIR}. "
            f"Place the script at <repo-root>/scripts/gen_plan.py")
    git("rev-parse", "--verify", f"{ref}^{{commit}}")
    if git_lines("rev-list", "--merges", ref):
        die("history contains merge commits; this planner supports linear history only")
    if git_lines("status", "--porcelain"):
        warn("working tree is dirty; the plan is built from committed history only")


def load_commits(ref: str) -> list[dict]:
    raw = git("log", "--reverse", "--format=%H%x00%at%x00%aI%x00%s", ref)
    commits = []
    for line in raw.splitlines():
        if not line:
            continue
        sha, ts, date, subject = line.split("\0", 3)
        commits.append({"sha": sha, "ts": int(ts), "date": date, "subject": subject})
    if not commits:
        die(f"no commits found for ref '{ref}'")
    return commits


# --------------------------------------------------------------------------- #
# grouping
# --------------------------------------------------------------------------- #
def build_groups(commits: list[dict], junk_re: re.Pattern) -> list[dict]:
    groups: list[dict] = []
    pending: list[dict] = []  # junk found before the first meaningful commit

    for commit in commits:
        if junk_re.match(commit["subject"].strip()):
            commit["role"] = "junk"
            if groups:
                groups[-1]["shas"].append(commit["sha"])
                groups[-1]["junk"].append(commit["sha"])
            else:
                pending.append(commit)
        else:
            commit["role"] = "base"
            groups.append({
                "base": commit["sha"],
                "shas": [c["sha"] for c in pending] + [commit["sha"]],
                "junk": [c["sha"] for c in pending],
                "leading": bool(pending),
                "message": commit["subject"],
            })
            pending = []

    if pending:  # junk at the very end of history
        if not groups:
            die("every commit is junk; there is nothing to group into")
        groups[-1]["shas"] += [c["sha"] for c in pending]
        groups[-1]["junk"] += [c["sha"] for c in pending]
    return groups


def suggest(n_junk: int, overlap_prev: int, overlap_next: int) -> str:
    if n_junk >= LONG_RUN:
        return "inspect manually: long run, consider standalone or split"
    if overlap_next > overlap_prev and overlap_prev == 0:
        return "consider merging into NEXT commit"
    if overlap_next >= 2 * max(overlap_prev, 1):
        return "consider merging into NEXT commit"
    return "merge into previous (default)"


def annotate(groups: list[dict], by_sha: dict, threshold: int, max_files: int) -> None:
    for index, group in enumerate(groups):
        group["id"] = index + 1
        group["review"] = len(group["junk"]) >= threshold
        if not group["junk"]:
            continue
        if group["leading"]:
            group["review"] = True
            group["hint"] = {"dots": len(group["junk"]),
                             "suggestion": "verify manually: junk before first meaningful commit"}
            continue

        base = group["base"]
        first_junk, last_junk = group["junk"][0], group["junk"][-1]
        prev = parent_of(first_junk) or empty_tree()
        nxt = groups[index + 1]["base"] if index + 1 < len(groups) else None

        junk_files = git_names("diff", "--name-only", "-z", prev, last_junk)
        overlap_prev = len(junk_files & commit_files(base))
        overlap_next = len(junk_files & commit_files(nxt)) if nxt else 0
        gap_prev = (by_sha[first_junk]["ts"] - by_sha[base]["ts"]) / 60.0
        gap_next = ((by_sha[nxt]["ts"] - by_sha[last_junk]["ts"]) / 60.0) if nxt else None

        group["hint"] = {
            "dots": len(group["junk"]),
            "prev": base[:7],
            "prev_subject": by_sha[base]["subject"],
            "next": nxt[:7] if nxt else None,
            "next_subject": by_sha[nxt]["subject"] if nxt else None,
            "overlap_prev": overlap_prev,
            "overlap_next": overlap_next,
            "gap_prev_minutes": round(gap_prev, 1),
            "gap_next_minutes": round(gap_next, 1) if gap_next is not None else None,
            "stat": git("diff", "--shortstat", prev, last_junk).strip(),
            "files_total": len(junk_files),
            "files": sorted(junk_files)[:max_files],
            "range": f"{base[:7]}..{last_junk[:7]}",
            "suggestion": suggest(len(group["junk"]), overlap_prev, overlap_next),
        }


def validate(groups: list[dict], commits: list[dict]) -> None:
    flat = [sha for group in groups for sha in group["shas"]]
    if flat != [c["sha"] for c in commits]:
        die("internal error: groups do not cover the history exactly once, in order")


# --------------------------------------------------------------------------- #
# writers
# --------------------------------------------------------------------------- #
def backup_previous(plan_path: Path) -> Path | None:
    if not plan_path.exists():
        return None
    BACKUP_DIR.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    dest = BACKUP_DIR / f"{plan_path.stem}.{stamp}{plan_path.suffix}"
    dest.write_bytes(plan_path.read_bytes())
    return dest


def carry_over_messages(groups: list[dict], plan_path: Path) -> int:
    """Keep hand-written messages for groups whose membership did not change."""
    if not plan_path.exists():
        return 0
    try:
        old = json.loads(plan_path.read_text(encoding="utf-8"))
        old_messages = {tuple(g["shas"]): g.get("message", "") for g in old}
    except (json.JSONDecodeError, OSError, TypeError, KeyError):
        warn("previous plan.json is unreadable; starting from scratch")
        return 0
    kept = 0
    for group in groups:
        message = old_messages.get(tuple(group["shas"]))
        if message and message != group["message"]:
            group["message"] = message
            group["message_edited"] = True
            kept += 1
    return kept


def write_plan(path: Path, groups: list[dict]) -> None:
    path.write_text(json.dumps(groups, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")


def write_inventory(path: Path, groups: list[dict], by_sha: dict) -> None:
    rows = ["group_id\trole\tsha\tdate\tsubject"]
    for group in groups:
        junk = set(group["junk"])
        for sha in group["shas"]:
            commit = by_sha[sha]
            subject = commit["subject"].replace("\t", " ")
            role = "junk" if sha in junk else "base"
            rows.append(f"{group['id']}\t{role}\t{sha}\t{commit['date']}\t{subject}")
    path.write_text("\n".join(rows) + "\n", encoding="utf-8")


def write_review(path: Path, groups: list[dict], commits: list[dict], args) -> None:
    runs = Counter(len(g["junk"]) for g in groups if g["junk"])
    n_junk = sum(len(g["junk"]) for g in groups)
    flagged = [g for g in groups if g["review"]]
    auto = [g for g in groups if g["junk"] and not g["review"]]
    dist = ", ".join(f"{size}x{count}" for size, count in sorted(runs.items()))

    out = [
        "# Commit squash plan - review report",
        "",
        f"- generated: {datetime.now().isoformat(timespec='seconds')}",
        f"- repo: `{REPO_ROOT}`",
        f"- ref: `{args.ref}`",
        f"- junk regex: `{args.junk_regex}`",
        "",
        "## Summary",
        "",
        f"- commits in history: **{len(commits)}**",
        f"- junk commits: **{n_junk}** in **{sum(runs.values())}** runs",
        f"- run length distribution: {dist}",
        f"- planned commits: **{len(groups)}**",
        f"- groups needing review (>= {args.review_threshold} junk): **{len(flagged)}**",
        "",
        "## How to use",
        "",
        "1. Edit `plan.json`: rewrite `message` for every group.",
        "2. To attach junk to the NEXT commit instead: move those SHAs to the front of",
        "   the next group's `shas` list (keep global order intact).",
        "3. To keep a run as its own commit: give it its own group object.",
        "4. Never drop a commit - only regroup. Then run the rebuild step and check",
        "   that `git diff <backup-ref> <new-ref>` is empty.",
        "",
        "## Groups to review",
        "",
    ]

    for group in sorted(flagged, key=lambda g: (-len(g["junk"]), g["id"])):
        hint = group["hint"]
        out.append(f"### #{group['id']} - {group['message']}  ({hint['dots']} junk)")
        out.append("")
        out.append(f"- suggestion: **{hint['suggestion']}**")
        if "range" in hint:
            out.append(f"- prev `{hint['prev']}` \"{hint['prev_subject']}\" | "
                       f"file overlap {hint['overlap_prev']} | gap {hint['gap_prev_minutes']} min")
            out.append(f"- next `{hint['next']}` \"{hint['next_subject']}\" | "
                       f"file overlap {hint['overlap_next']} | gap {hint['gap_next_minutes']} min")
            out.append(f"- {hint['stat'] or 'no net change'}")
            out.append(f"- files ({hint['files_total']}): {', '.join(hint['files'])}")
            out.append("")
            out.append("```bash")
            out.append(f"git log --stat {hint['range']}")
            out.append(f"git diff {hint['range']}")
            out.append("```")
        out.append("")

    out += ["## Auto-merged (below review threshold)", "",
            "| group | base message | junk | overlap prev/next |",
            "|---|---|---|---|"]
    for group in auto:
        hint = group["hint"]
        out.append(f"| #{group['id']} | {group['message']} | {hint['dots']} | "
                   f"{hint['overlap_prev']}/{hint['overlap_next']} |")
    path.write_text("\n".join(out) + "\n", encoding="utf-8")


# --------------------------------------------------------------------------- #
# entry point
# --------------------------------------------------------------------------- #
def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate a commit-squash plan.")
    parser.add_argument("--ref", default="HEAD", help="ref to plan (default: HEAD)")
    parser.add_argument("--junk-regex", default=DEFAULT_JUNK_REGEX,
                        help="case-insensitive regex matching disposable commit subjects")
    parser.add_argument("--review-threshold", type=int, default=2,
                        help="flag groups with at least this many junk commits (default: 2)")
    parser.add_argument("--max-files", type=int, default=12,
                        help="max file names listed per group in review.md (default: 12)")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    try:
        junk_re = re.compile(args.junk_regex, re.IGNORECASE)
    except re.error as exc:
        die(f"invalid --junk-regex: {exc}")

    preflight(args.ref)
    commits = load_commits(args.ref)
    by_sha = {c["sha"]: c for c in commits}
    groups = build_groups(commits, junk_re)
    annotate(groups, by_sha, args.review_threshold, args.max_files)
    validate(groups, commits)

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    plan_path = OUTPUT_DIR / "plan.json"
    kept = carry_over_messages(groups, plan_path)
    saved = backup_previous(plan_path)

    write_plan(plan_path, groups)
    write_inventory(OUTPUT_DIR / "inventory.tsv", groups, by_sha)
    write_review(OUTPUT_DIR / "review.md", groups, commits, args)

    n_junk = sum(len(g["junk"]) for g in groups)
    n_review = sum(g["review"] for g in groups)
    print(f"repo      : {REPO_ROOT}")
    print(f"ref       : {args.ref}")
    print(f"history   : {len(commits)} commits, {n_junk} junk")
    print(f"plan      : {len(groups)} commits, {n_review} groups need review")
    if kept:
        print(f"carried over {kept} hand-written message(s) from the previous plan")
    if saved:
        print(f"previous plan backed up to {saved.relative_to(REPO_ROOT)}")
    for name in ("plan.json", "review.md", "inventory.tsv"):
        print(f"wrote     : {(OUTPUT_DIR / name).relative_to(REPO_ROOT)}")


if __name__ == "__main__":
    main()