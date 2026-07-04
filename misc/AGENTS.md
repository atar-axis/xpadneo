<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# DOX Framework

## Purpose

This subtree owns auxiliary examples, old reference material, generated patch
helpers, and kernel patch notes.


## Ownership

- Example programs and sample udev rules.
- Kernel patch helper scripts and patch outputs.
- Archived or legacy reference files.


## Local Contracts

- Keep examples small, standalone, and clearly tied to the behavior they
  demonstrate.
- Do not update generated patch outputs without also checking the generator or
  source patch context.
- Treat old reference material as historical input unless the task explicitly
  revives it.


## Work Guidance

- Avoid mixing example cleanups with production driver changes.
- Prefer documenting why a patch helper changes over silently replacing output.


## Verification

- Run the smallest available syntax or build check for touched examples or
  helper scripts.


## Child DOX Index

No child guides are currently needed.
