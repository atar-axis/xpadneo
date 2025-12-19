# Driver relicensing / rewrite tracking (kernel module scope)

*This is a preparatory step and does not affect current releases.*

> Scope: Only code that is linked into the kernel module (hid-xpadneo/src/*).
> Goal: End state is GPL-2.0-only for all kernel driver code.
> Non-driver code (docs/examples/tools) may remain GPL-3.0-or-later.


## Purpose of this PR

This PR documents and prepares the ongoing effort to make the xpadneo kernel driver code compatible with the Linux
kernel’s GPL-2.0-only licensing requirements.

The goal is to eventually have all code that is linked into the kernel module (`hid-xpadneo/src/*`) licensed under
GPL-2.0-only, while allowing non-driver code (documentation, examples, tooling) to remain under GPL-3.0-or-later.


## What this PR does

- It introduces a tracking table that categorizes existing driver contributions by author, scope, and complexity.
- Based on that classification, we distinguish between:
  - **non-trivial driver logic**, where explicit relicensing consent is preferred, and
  - **trivial or low-effort changes**, which will be re-implemented during the ongoing refactoring instead of chasing
    consent.
- This PR does **not** change the license of existing code yet. It documents the current state and the intended
  migration path.


## About contributor tagging

Only contributors whose changes include non-trivial driver logic are explicitly tagged and asked for consent. This is
intentional.

Minor fixes (e.g. one-line bugfixes, formatting, small table entries, simple parameter gates) are planned to be
re-implemented as part of the refactor, as doing so is typically simpler and less intrusive than retroactively
collecting formal approvals.

If you contributed driver code and believe your contribution has been misclassified here, please feel free to comment —
corrections are welcome.


## Scope clarification

- This effort applies **only** to code that is linked into the kernel module.
- Documentation, descriptors, examples, and tooling are explicitly out of scope for GPL-2.0-only and may remain under
  GPL-3.0-or-later.


## Next steps

- Collect explicit consent from selected contributors where applicable.
- Continue modularization and clean-room re-implementation of driver components.
- Track replaced or removed legacy code in this table as the refactor progresses.

This approach is intended to be transparent, fair to contributors, and aligned with kernel licensing expectations.


## Request for confirmation

If you’re okay with it, please confirm here that your driver contributions may be relicensed under GPL-2.0-only as part
of the ongoing refactor.

This request applies only to contributions that are linked into the kernel module itself.

Simply post a reply with:

> I’m fine with relicensing my driver contributions under GPL-2.0-only.

If you prefer not to consent, a short reply stating that is perfectly fine and helps us plan the rewrite accordingly.


## Tracking table

| Contributor (git author) | Likely GitHub handle | Driver commits / areas | Size (adds/dels in driver) | Classification | Action | Status | Notes / rewrite plan |
|---|---|---:|---:|---|---|---|---|
| Kai Krakow | @kakra | maintainer / main driver | 3030 / 2749 | core | keep | n/a | main rewrite/refactor driver into GPL-2.0-only files |
| atar-axis / Florian Dollinger | @atar-axis | core driver (historical) | 1782 / 556 + 268 / 45 | core | keep | ✅ (per your note) | unify identities in tracking; already OK to relicense own code |
| Ben Schattinger | @lights0123 | Series X/S support; Share button quirks + HID bit fixups | 16 / 2 (driver) | non-trivial logic | keep | ✅ | Adds quirk flag, usage map for Share (KEY_RECORD), raw_event bit re-mapping, adds PID 0x0B13. |
| Erik Hajnal | @ehats | XBE2 unknown mode: mapping/fixups, ignore keyboard, reportsize 55 | 42 / 11 (driver) | non-trivial logic | keep | ✅ | Adds many USAGE_IGN entries, ignores HID_UP_KEYBOARD, extends raw_event reportsize handling, tweaks PID handling. |
| Dugan Chen | @duganchen | Elite Series 2 wireless: adds BT PID + comments + disabled (#if 0) hack | 21 / 2 (driver) | trivial-ish / data plumbing | keep | ✅ | Mostly PID table entry (0x0B05), comment expansion, and #if 0 stub. |
| bouhaa | @bouhaa | disable_shift_mode module param gating BTN_XBOX shift behavior | 8 / 1 (driver) | small feature gate | rewrite/replace (easy) | ⏳ | Adds module_param + condition `!param_disable_shift_mode` around existing branch. Re-implement cleanly in new modular file if needed. |
| Adam Becker | @adam-becker | small bugfixes + cleanup + param gate + rename | 29 / 24 (driver) | trivial to small-moderate | rewrite/replace | ⏳ | Battery mode case fix; early return cleanup; MODULE_PARM_DESC formatting; add `ff_connect_notify` param + gate welcome rumble; rename mapping function. All easy to re-implement during refactor without chasing consent. |
| John Mather | @NextDesign1 | OUI quirk entry (single line) | 1 / 0 (driver) | trivial / factual data | rewrite/replace | ⏳ | Treat as replaceable while rebuilding quirks/device tables in new files |
| PiMaker | @pi | build/debug print fix (single line) | 1 / 1 (driver) | trivial | rewrite/replace | ⏳ | fold into refactor; do not chase consent |
| nassek | @nassek | missing variable name (single line) | 1 / 1 (driver) | trivial | rewrite/replace | ⏳ | fold into refactor; do not chase consent |
| yjun | @yjun123 | missing report check (2-line change) | 2 / 2 (driver) | trivial | keep | ✅ | commit 93c76df |


## Status legend

- ✅ consent obtained (comment link recorded)
- ❌ consent declined
- ⏳ pending (handle lookup / ask / review)
- 🔁 rewrite planned
- 🧹 removed

*Thanks to all contributors for helping keep xpadneo maintainable long-term.*
