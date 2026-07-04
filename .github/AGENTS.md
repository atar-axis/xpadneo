<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# DOX Framework

## Purpose

This subtree owns repository automation and GitHub-facing project metadata.


## Ownership

- Issue templates and pull request templates.
- GitHub Actions workflows.
- Repository automation configuration such as stale issue handling.


## Local Contracts

- Keep AI-assistance declarations aligned with `CONTRIBUTING.md`.
- Preserve the human review, human testing, and human sign-off requirement for
  AI-assisted pull requests.
- Treat workflow edits as repository behavior changes because they can affect
  labels, checks, notifications, and issue or pull request state.


## Work Guidance

- Keep template language short and actionable.
- Avoid changing automation side effects in the same commit as unrelated
  documentation or driver changes.
- Inspect the current workflow command lines before changing checkpatch,
  checkout depth, branch, or path filters.


## Verification

- Inspect diffs for template wording changes.
- For workflow edits, check YAML syntax with the available local tooling when a
  parser is installed.
- For checkpatch workflow changes, compare the resulting command with the
  kernel checkpatch invocation used by CI.


## Child DOX Index

No child guides are currently needed.
