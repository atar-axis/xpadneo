<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# DOX Framework

- DOX is the agent-guide hierarchy installed here.
- Agents must follow DOX instructions across edits.


## Core Contract

- Agent guides are binding work contracts for their subtrees.
- Work products, source materials, instructions, records, assets, and durable
  docs must stay understandable from the nearest applicable guide plus every
  parent guide above it.
- Follow `CONTRIBUTING.md` for licensing, release staging, AI assistance, and
  sign-off requirements.
- AI-assisted work must remain human-owned: a human reviews, tests, signs off,
  and takes responsibility for submitted commits.
- AI agents must not add `Co-authored-by` trailers for themselves or other AI
  tools.
- AI-assisted pull requests must declare AI assistance in the pull request
  template.


## Read Before Editing

1. Read the root guide.
2. Identify every file or folder you expect to touch.
3. Walk from the repository root to each target path.
4. Read every guide found along each route.
5. If a parent guide lists a child guide whose scope contains the path, read
   that child and continue from there.
6. Use the nearest guide as the local contract and parent docs for repo-wide
   rules.
7. If docs conflict, the closer doc controls local work details, but no child
   doc may weaken DOX.

Do not rely on memory. Re-read the applicable DOX chain in the current session
before editing.

Also read `CONTRIBUTING.md` before contribution, branch, release, backport, or
AI-attribution decisions.


## Update After Editing

Every meaningful change requires a DOX pass before the task is done.

Update the closest owning guide when a change affects:

- purpose, scope, ownership, or responsibilities
- durable structure, contracts, workflows, or operating rules
- required inputs, outputs, permissions, constraints, side effects, or artifacts
- user preferences about behavior, communication, process, organization, or
  quality
- guide creation, deletion, move, rename, or index contents

Update parent docs when parent-level structure, ownership, workflow, or child
index changes. Update child docs when parent changes alter local rules. Remove
stale or contradictory text immediately. Small edits that do not change behavior
or contracts may leave docs unchanged, but the DOX pass still must happen.


## Hierarchy

- The root guide is the DOX rail for project-wide instructions, global
  preferences, durable workflow rules, and the top-level Child DOX Index.
- Child guides own domain-specific instructions and their own Child DOX Index.
- Each parent explains what its direct children cover and what stays owned by
  the parent.
- The closer a doc is to the work, the more specific and practical it must be.


## Child Doc Shape

- Create a child guide when a folder becomes a durable boundary with its own
  purpose, rules, responsibilities, workflow, materials, or quality standards.
- Work Guidance must reflect the current standards of the project or user
  instructions. If there are no specific standards yet, leave it empty.
- Verification must reflect an existing check. If no verification framework
  exists yet, leave it empty and update it when one exists.

Default section order:

- Purpose
- Ownership
- Local Contracts
- Work Guidance
- Verification
- Child DOX Index


## Style

- Keep docs concise, current, and operational.
- Document stable contracts, not diary entries.
- Put broad rules in parent docs and concrete details in child docs.
- Prefer direct bullets with explicit names.
- Do not duplicate rules across many files unless each scope needs a local
  version.
- Delete stale notes instead of explaining history.
- Trim obvious statements, repeated rules, misplaced detail, and warnings for
  risks that no longer exist.


## Project Work Guidance

- Keep commits small and separable.
- Do not mix safe fixes with behavior changes, default flips, or broad driver
  rewrites.
- Keep compatibility-sensitive changes conservative on `master` and stable
  branches.
- Put risky or behavior-changing work into focused long-lived pull requests for
  a later milestone.
- When working from issues or pull requests, inspect linked reports and prior
  review comments before deciding what is actionable.
- Prefer extracting safe, independently testable patches from large pull
  requests instead of rebasing or merging broad experimental series wholesale.
- Use available review, GitHub, CI, or workflow skills when they help preserve
  review-thread context, linked issue context, or test status.
- Do not push, merge, close issues, resolve review threads, or post GitHub
  comments unless explicitly asked.


## Bug Fixing Guidance

- Start from the observed failure, linked reports, and relevant logs.
- Identify whether the fix is a bug fix, safe feature, behavior change, or
  architecture change before choosing its target branch or milestone.
- Document hardware-specific findings when they rely on descriptors, OUIs,
  firmware versions, or controller-specific quirks.


## Verification

- Run the smallest relevant local check for the touched files.
- For driver changes, prefer `make -C hid-xpadneo modules` when the local kernel
  build environment is available.
- For shell-only changes, run `bash -n` on edited scripts.
- Report any verification that could not be run.


## Closeout

1. Re-check changed paths against the DOX chain.
2. Update nearest owning docs and any affected parents or children.
3. Refresh every affected Child DOX Index.
4. Remove stale or contradictory text.
5. Run existing verification when relevant.
6. Report any docs intentionally left unchanged and why.


## User Preferences

When the user requests a durable behavior change, record it here or in the
relevant child guide.


## Child DOX Index

- `.github/` - repository automation, issue templates, pull request templates,
  and CI workflows.
- `docs/` - user-facing documentation, descriptors, heuristics, licensing
  notes, and captured reports.
- `hid-xpadneo/` - DKMS package metadata, installable module assets, udev and
  modprobe integration, and the kernel module source tree.
- `lib/` - shared shell helper code used by the root installer scripts.
- `misc/` - auxiliary examples, old reference material, and kernel patch
  helpers.

The root keeps ownership of top-level project metadata, release notes,
repository-wide contribution policy, root installer entry points, and cache or
tooling folders that are not listed above.
