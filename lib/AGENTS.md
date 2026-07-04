<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# DOX Framework

## Purpose

This subtree owns shared shell helper code used by the root installer entry
points.


## Ownership

- Installer and configuration helper functions.
- Shared option parsing and shell utilities.


## Local Contracts

- Keep shell behavior compatible with the root scripts that source these files.
- Avoid surprising privileged actions; root or sudo requirements must remain
  visible at the root script level.
- Preserve existing command-line options unless the task explicitly changes the
  installer interface.


## Work Guidance

- Keep helper changes separate from unrelated documentation or driver changes.
- Check every root script that sources a changed helper.


## Verification

- Run `bash -n` on touched shell files and affected root scripts.


## Child DOX Index

No child guides are currently needed.
