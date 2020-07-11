# Changes since v0.7 up to v0.8

*Quote of the day:*
> HID me baby, one more time!

Much hard work of several weeks has gone into this release, so there's a big
reason to say thanks to all contributors, be it testing, suggestions, bug
reporting, or - of course - programming.

Many thanks @medusalix for finally figuring out the underlying problems of
the disconnect behavior of the controller during rumble events: After some
internal discussion, we managed to find a race condition in the controller
firmware and worked around it by throttling the rumble reprogramming to
intervals of at least 10ms as this is what the controllers seems to accept
as the minimal interval without freaking out. As a "thank you", I contributed
a logically identical patch to the [xow](https://github.com/medusalix/xow)
project which has not been merged yet, tho. Check it out - it supports the
native dongle that comes with the controller!

Also, let's all say "thank you" to @ehats for contributing the Xbox Elite 2
series controller support. He started from zero to mastering the HID
internals in just a few days with just a little help from me. I'm sure, a
vast amount of effort, a steep learning curve, and a lot of endurance and
probably also one beer or another went into this work. Leave him some kudos
for this awesome contribution, and expect more to follow. His work builds
the base for adding profile support and customization into the next xpadneo
version. He qualified for the quote of the day above.

A handful patches have been contributed which fix some smaller bugs:

  * Thanks to Adam Becker for adding some bug and style fixes.
  * Lars-Sören Steck fixed a scripting bug, thank you.
  * Manjaro users now see instructions for their favorite distribution,
    thanks to @snpefk.

Let's not forget our testers, who have patiently been with us during the
development of this milestone: We appreciate your work, patience, and
contributions.

Many of the bug fixes have been backported to the v0.7 branch which has now
received its end of life. If you're having problems with the new features,
please report that and use the v0.7 branch until that specific bug is fixed.


## Breaking Changes

This version removed some module parameters with only partial replacement.
One of those if `disable_ff` which can no longer be used. Instead, there's
a new parameter `trigger_rumble_mode` which can disable trigger rumble
only. The next update will include a new parameter to set rumble attenuation
to 100% which translates to no rumble at all.


## Headlines:

  * dmks, installer: Move etc sources one level up
  * docs, mapping: Document and explain `joydev` mapping
  * hid-xpadneo: Adhere to Linux Gamepad Specification
  * hid-xpadneo, battery: Rework detection, parsing and reporting
  * hid-xpadneo, compat: Fix compilation on kernel 4.19
  * hid_xpadneo, quirks: Add "8BitDo SN30 Pro+"
  * hid-xpadneo, rumble: Throttle reprogramming of rumble motors
  * installer: Parse DKMS version info correctly
  * installer: Use awk in non-GNU mode

```
Kai Krakow (70):
      configure: `fake_dev_version` no longer exists
      configure: `trigger_rumble_damping` should no longer default to 4
      configure: Choose a single or default config file only
      configure: Create missing directories and files on demand
      configure: Silence superfluous error messages
      dkms, install: Do not apply inline version patching
      dkms, installer: Fix early exit if `git-rev-parse` does not work
      dkms, installer: Fix version parsing
      dkms, udev: Eliminate bash scripting from udev rules
      dkms, uninstall: Fix early exit if `modprobe -r` does not work
      dkms, update: Suggest a download URL actually matching the version
      dkms: Use cmdline format which requires less escaping
      dmks, installer: Move etc sources one level up
      docs, chipsets: Update chipset compatibility reports
      docs, cleanup: Fix typos
      docs, cleanup: Fix white-space and new-lines
      docs, cleanup: Fix whitespace
      docs, cleanup: Remove outdated debug documentation
      docs, cleanup: Visual tweaks
      docs, formatting: Migrate to proper headlines
      docs, mapping: Document and explain `joydev` mapping
      docs: Document 8BitDo SN30 Pro+ controller
      docs: Improve the debugging instructions
      docs: Mention other projects
      docs: Update documentation with rumble modes
      docs: xpadneo is no longer the only driver
      hid-xpadneo, battery: Protect concurrent access with spin locks
      hid-xpadneo, battery: Rework detection, parsing and reporting
      hid-xpadneo, cleanup: Reorder includes
      hid-xpadneo, cleanup: Simplify mapping code
      hid-xpadneo, comments: Fix wording
      hid-xpadneo, compat: Fix compilation on kernel 4.19
      hid-xpadneo, events: Do not send Xbox logo event for turn-off
      hid-xpadneo, formatting: Kernel allows 100 chars line length now
      hid-xpadneo, Makefile: Accept `rmmod` failing
      hid-xpadneo, patching: Only show version patching if changed
      hid-xpadneo, rumble: Protect concurrent access with spin locks
      hid-xpadneo, rumble: Throttle reprogramming of rumble motors
      hid-xpadneo: Adhere to Linux Gamepad Specification
      hid-xpadneo: parameter `combined_z_axis` cannot be changed at runtime
      hid-xpadneo: Separate private and foreign includes
      hid_xpadneo, cleanup: Use symbols instead of numbers in mapping phase
      hid_xpadneo, probe: Fix newline in error path
      hid_xpadneo, quirks: Add "8BitDo SN30 Pro+"
      hid_xpadneo, quirks: Migrate 8BitDo quirks to OUI match
      hid_xpadneo, rumble: Optimize motor reprogramming
      hid_xpadneo, style: Fix indentation
      hid_xpadneo: Add quirk modes for misbehaving controllers
      hid_xpadneo: New trigger rumble modes
      hidxpad-neo, cleanup: Fix a comment style
      installer, cleanup: Remove superfluous whitespace
      installer: Add support for disabling ERTM override
      installer: Compare latest version correctly
      installer: Fix non-git version lookup
      installer: Get latest version from the releases page
      installer: Parse DKMS version info correctly
      installer: Print some version information in update instructions
      installer: Trim whitespace from online version numbers
      installer: Try to fetch the current version from git tags
      installer: Use awk in non-GNU mode
      meta: Ignore vscode metadata completely
      meta: Sort gitignore
      misc: Add hidraw test program
      Update bug_report.md
      Update bug_report.md
      Update bug_report.md
      Update bug_report.md
      Update bug_report.md
      Update README.md
      xpadneo, event: Remove symbols

Adam Becker (6):
      Fix bug where 2 would map to unknown PS type instead of battery.
      Fixing formatting issues
      Don't use common return point if no resources are freed
      Formatting parameter sections
      Add parameter section for toggling the FF connect notification
      Rename 'xpadneo_mapping' to match other names

Erik Hajnal (1):
      Add support for XBE2 (Unknown mode)

Lars-Sören Steck (1):
      Fix error "[: unexpected operator"

snpefk (1):
      Add install instruction for Manjaro
```

# Changes since v0.6 up to v0.7

Major code overhaul and redesign. It optimizes the controller
communication path, adds support for more hardware and uses
HID table fixups now instead of trying to code for every
broken mapping variant.

## Headlines:

  * fix direction_rumble_test path error (#168)
  * hid-xpadneo: Convert mapping to using tables instead of code
  * Pretend we are in Windows wireless mode

```
Kai Krakow (21):
      Add support von non-DKMS build
      Follow the kernel code style better
      hid-xpadneo: Prevent accidental fall-through
      hid-xpadneo: Scale rumble magnitudes correctly
      hid-xpadneo, cleanup: Remove setting default axis values
      hid-xpadneo, cleanup: Outsource welcome rumble
      hid-xpadneo, cleanup: Cleanup `xpadneo_initBatt()` a little
      hid-xpadneo: Fix usage of report ID in `xpadneo_raw_event()` hook
      hid-xpadneo: Use work queue for rumble effects
      hid-xpadneo: Use work queue for battery events
      docs: Add some controller documentation
      Pretend we are in Windows wireless mode
      Revert "shift axis values to the left"
      hid-xpadneo: Convert mapping to using tables instead of code
      hid-xpadneo: Clean up
      hid-xpadneo: Do not repeat rumble packets
      hid-xpadneo: Rework the directional rumble model
      hid-xpadneo, cleanup: Clean up combined z-axis feature
      hid-xpadneo: Pretend different firmware for PID 0x02E0
      hid-xpadneo: Autodetect battery presence after connect
      hid-xpadneo, cleanup: Remove most of the remaining debug cruft

Dugan Chen (1):
      Add Support For Xbox Elite Series 2 Wireless

Florian Dollinger (1):
      Create no-response.yml

Srauni (1):
      fix direction_rumble_test path error (#168)
```
