#!/usr/bin/env python3
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

"""Report top-level submodules pinned behind their upstream branch. Never fails the build.

In Azure Pipelines (TF_BUILD set) each outdated dependency is a warning, the job ends as
SucceededWithIssues and a markdown summary is attached to the run. GITHUB_TOKEN, if set, is used
for the GitHub compare API (commits behind, pinned commit date).
"""
import json
import os
import re
import subprocess
import sys
import tempfile
import urllib.request

IN_PIPELINE = bool(os.environ.get("TF_BUILD"))


def git(*args, cwd=None):
    return subprocess.run(["git", *args], cwd=cwd, check=True, capture_output=True, text=True).stdout


def submodules(root):
    """[(path, url, branch or None)] from .gitmodules."""
    entries = {}
    out = git("config", "-f", ".gitmodules", "--get-regexp", r"^submodule\..*\.(path|url|branch)$", cwd=root)
    for line in out.splitlines():
        key, value = line.split(" ", 1)
        name, field = key[len("submodule."):].rsplit(".", 1)
        entries.setdefault(name, {})[field] = value.strip()
    return [(e["path"], e["url"], e.get("branch")) for e in entries.values() if "path" in e and "url" in e]


def pinned_sha(root, path):
    out = git("ls-tree", "HEAD", path, cwd=root).split()
    return out[2] if len(out) >= 3 and out[1] == "commit" else None


def upstream_tip(url, branch):
    """(branch, sha) of `branch`, or of the remote's default branch."""
    if branch:
        out = git("ls-remote", url, f"refs/heads/{branch}")
        return branch, (out.split()[0] if out else None)
    out = git("ls-remote", "--symref", url, "HEAD")
    m = re.search(r"^ref: refs/heads/(\S+)\s+HEAD", out, re.M)
    sha = next((l.split()[0] for l in out.splitlines() if l.endswith("\tHEAD") and not l.startswith("ref:")), None)
    return (m.group(1) if m else "HEAD"), sha


def github_repo(url):
    m = re.match(r"https://github\.com/([^/]+/[^/]+?)(?:\.git)?/?$", url, re.I)
    return m.group(1) if m else None


def compare(repo, base, head):
    """(commits behind, pinned commit date) from the GitHub compare API, or (None, None)."""
    req = urllib.request.Request(f"https://api.github.com/repos/{repo}/compare/{base}...{head}",
                                 headers={"Accept": "application/vnd.github+json"})
    token = os.environ.get("GITHUB_TOKEN")
    if token:
        req.add_header("Authorization", f"Bearer {token}")
    try:
        with urllib.request.urlopen(req, timeout=30) as r:
            data = json.load(r)
        date = (data.get("merge_base_commit") or {}).get("commit", {}).get("committer", {}).get("date", "")
        return data.get("ahead_by"), date[:10] or None
    except Exception:
        return None, None


def warn(msg):
    print(f"##vso[task.logissue type=warning]{msg}" if IN_PIPELINE else f"WARNING: {msg}", flush=True)


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else "."
    rows, outdated = [], 0
    for path, url, branch in sorted(submodules(root)):
        pinned = pinned_sha(root, path)
        try:
            ref, tip = upstream_tip(url, branch)
        except subprocess.CalledProcessError as e:
            warn(f"{path}: could not read {url}: {e.stderr.strip()}")
            rows.append((path, url, "?", pinned, None, None, None, "unreachable"))
            continue
        if not pinned or not tip:
            warn(f"{path}: pinned or upstream commit not found ({url})")
            rows.append((path, url, ref, pinned, tip, None, None, "unknown"))
            continue
        if pinned == tip:
            rows.append((path, url, ref, pinned, tip, 0, None, "up to date"))
            continue
        repo = github_repo(url)
        behind, date = compare(repo, pinned, tip) if repo else (None, None)
        if behind == 0:
            rows.append((path, url, ref, pinned, tip, 0, date, "ahead of upstream"))
            continue
        outdated += 1
        detail = f"{behind} commits behind" if behind is not None else "behind"
        since = f", pinned commit from {date}" if date else ""
        warn(f"{path} is {detail} {repo or url}@{ref} (pinned {pinned[:9]}{since}, latest {tip[:9]})")
        rows.append((path, url, ref, pinned, tip, behind, date, "OUTDATED"))

    lines = ["# Dependency updates", ""]
    lines.append(f"**{outdated} of {len(rows)} dependencies are behind upstream.** Time to update them!"
                 if outdated else f"All {len(rows)} dependencies are up to date.")
    lines += ["", "| Submodule | Upstream | Pinned | Latest | Behind | Pinned date | Status |", "|---|---|---|---|---|---|---|"]
    for path, url, ref, pinned, tip, behind, date, status in rows:
        lines.append(f"| {path} | {github_repo(url) or url}@{ref} | {(pinned or '-')[:9]} | {(tip or '-')[:9]} | "
                     f"{'-' if behind is None else behind} | {date or '-'} | {status} |")
    summary = "\n".join(lines) + "\n"
    print(summary)

    if IN_PIPELINE:
        fd, name = tempfile.mkstemp(suffix=".md", prefix="dependency-updates-")
        with os.fdopen(fd, "w", encoding="utf-8") as f:
            f.write(summary)
        print(f"##vso[task.uploadsummary]{name}")
        if outdated:
            print("##vso[task.complete result=SucceededWithIssues;]Outdated dependencies", flush=True)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as e:  # never fail the build
        warn(f"dependency update check failed: {e}")
        sys.exit(0)
