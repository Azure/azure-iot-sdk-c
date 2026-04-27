---
mode: agent
description: Update the azure-iot-sdk-c submodule chain bottom-up, enforce cross-tree commit-SHA consistency, and open one PR per repository, pausing for manual merge between levels.
---

# Update submodules — azure-iot-sdk-c

You are updating the nested-submodule dependency chain rooted at
`Azure/azure-iot-sdk-c`. Submodules appear in multiple parents and **every
occurrence of the same upstream repo must pin the exact same commit SHA**.
Work strictly bottom-up, one level at a time, one PR per repo, and **stop after
each PR is opened until the user confirms it is merged**.

## Inputs
- `${input:rootPath:Absolute path to the local azure-iot-sdk-c clone}`
- `${input:targetBranch:Branch to track upstream (default: master)}`
- `${input:prBranchPrefix:Branch name prefix for the PRs (default: chore/update-submodules)}`

## Dependency graph (authoritative, bottom-up)

```
L0  azure-macro-utils-c        (no submodules)
    parson                      (no submodules)
    RIoT                        (no submodules)

L1  azure-ctest                 -> macro-utils
    azure-c-testrunnerswitcher  -> macro-utils

L2  umock-c                     -> ctest, testrunnerswitcher, macro-utils

L3  azure-c-shared-utility      -> ctest, testrunnerswitcher, macro-utils, umock-c
    (a.k.a. c-utility)

L4  azure-umqtt-c               -> c-utility, macro-utils, testrunnerswitcher, ctest, umock-c
    azure-uamqp-c               -> c-utility, ctest, umock-c, testrunnerswitcher, macro-utils
    azure-uhttp-c               -> c-utility, macro-utils, testrunnerswitcher, ctest, umock-c
    azure-utpm-c                -> c-utility, macro-utils, testrunnerswitcher, ctest, umock-c

L5  azure-iot-sdk-c (root)      -> c-utility, umqtt, uamqp, parson, uhttp, RIoT,
                                   utpm, macro-utils, umock-c,
                                   testrunnerswitcher, ctest
```

Process levels in order **L0 → L5**. Within a level, repos are independent and
may be processed in parallel, but a level is not "done" until **all PRs at that
level are merged by the user**.

## Hard rules

1. **Never force-push. Never merge a PR yourself. Never bypass branch protection.**
2. **One PR per repo per level.** Do not bundle unrelated bumps.
3. **SHA-consistency invariant:** before opening any PR for a parent repo, every
   submodule that also appears elsewhere in the tree must be pinned to the same
   commit SHA across *all* parents that reference it. If a mismatch is found,
   stop and report — do not "guess" a winner.
4. **No upstream code edits.** If a build/test fails after a bump, report and stop.
5. **No secrets in logs.** Use `gh` CLI (already authenticated) for PR creation.
6. Do not modify `.gitmodules` URLs. Only update the recorded commit pointer.

## Pre-flight checks

Run from `${input:rootPath}`:

```bash
git -C "${rootPath}" status --porcelain   # must be clean
git -C "${rootPath}" submodule update --init --recursive
gh auth status                            # must be logged in
```

If any check fails, stop and report.

## Per-level workflow

For each repo `R` at the current level:

1. **Fetch upstream**
   ```bash
   git -C <R> fetch origin
   git -C <R> log --oneline HEAD..origin/${targetBranch}
   ```
   If no new commits, skip `R`.

2. **Verify R's own submodules are already up-to-date and consistent**
   (they must already be merged from a prior level). Run the consistency
   check below restricted to `R`'s subtree. If inconsistent, stop.

3. **Create branch and bump pointer in every parent that references R**
   - Identify parents from the graph above.
   - In each parent `P`:
     ```bash
     git -C <P> checkout -b ${prBranchPrefix}/<R>-<shortsha>
     git -C <P>/<path-to-R> checkout origin/${targetBranch}
     git -C <P> add <path-to-R>
     git -C <P> commit -m "chore: bump <R> to <shortsha>"
     ```

4. **Build & test each parent** using the parent's documented build command
   (look in `build_all/`, `jenkins/`, or `CMakeLists.txt`). Capture logs to
   `.copilot_tmp_logs/<repo>-<level>.log`. On failure: stop, report, do not push.

5. **Push and open PR per parent**
   ```bash
   git -C <P> push -u origin <branch>
   gh -R Azure/<P-name> pr create \
      --title "chore: bump <R> to <shortsha>" \
      --body  "$(cat <<EOF
   Bumps submodule \`<R>\` to \`<full-sha>\`.

   Upstream commits:
   $(git -C <P>/<path-to-R> log --oneline <oldsha>..<newsha>)

   Validation: <pass/fail summary, link to log>
   Consistency: verified across <list of sibling parents at this level>.
   EOF
   )"
   ```

6. **Notify and pause.** Print every PR URL produced at this level, then say:
   > Level `<N>` PRs opened. Waiting for manual merge of all listed PRs before
   > proceeding to level `<N+1>`. Reply "merged" once done.
   **Do not proceed.**

7. **Resume.** When the user confirms, in every parent run:
   ```bash
   git -C <P> checkout ${targetBranch}
   git -C <P> pull --ff-only
   git -C <P> submodule update --init --recursive
   ```
   Re-run the consistency check (below) before moving up.

## Cross-tree SHA consistency check

Before opening PRs at any level, and after merges before advancing, run this
check across the entire working tree:

```bash
# From rootPath
git submodule foreach --recursive --quiet '
  echo "$(git config --get remote.origin.url)|$(git rev-parse HEAD)|$displaypath"
' | sort
```

Group rows by normalized URL (strip `.git`, lowercase, ignore http/https). For
every URL with >1 row, **all SHAs in the group must be identical**. If not:

- Identify which parent(s) lag behind.
- Report the offenders as a table: `repo | parent | path | sha`.
- **Stop.** Do not auto-resolve.

## Final level (root azure-iot-sdk-c)

After L4 PRs are merged and consistency holds, perform the same flow for the
root repo: create one PR that bumps every direct submodule of the root to its
new tip. Validate with the root's `build_all/linux/build.sh` (or
`build_all/windows/build_client.cmd` on Windows). Open the PR and stop.

## Output contract (what you must report back to the user)

After every level, print:

- Level number and the repos processed.
- For each repo: old SHA → new SHA, # of upstream commits, build/test result,
  PR URL.
- Consistency-check result (PASS/FAIL with table on FAIL).
- Explicit "WAITING FOR MERGE" line and the list of PRs to merge.

Never advance a level without an explicit user "merged" confirmation.
