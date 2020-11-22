# Changes since v0.8 up to v0.9

*Quote of the day:*
> Good News, Everyone!

Much hard work of several weeks has gone into this release, so there's a big
reason to say thanks to all contributors, be it testing, suggestions, bug
reporting, or - of course - programming.

First of all, Michael Schmitt donated an Xbox Elite 2 controller, so we now
have full support for basic functionality of this controller, with support for
the advanced features coming in the v0.10 version of the driver. This builds
upon contributions done in the previous version of the driver.

Additionally, Ben Schattinger contributed support for the new Xbox Series X|S
controllers that were released by Microsoft with their new console generation.
Support for the new share button is still not finalized but it will generate a
key code. This new controller uses BLE (Bluetooth low energy) to connect to PC,
so you need a Bluetooth dongle that supports that features (most should do).

Both of you, Michael and Ben, qualified for the quote of the day. You guys
rock.

Also, some donations were provided by several people via Ko-fi. Thanks for
that, it's really appreciated. You guys are awesome!

Let's not forget all the people who contributed to the project patiently
providing feedback, bug reports, and testing, or even just a "thank you": This
release is our "thank you" back to the community.

Finally, we've got a logo now.

Here the gist of what the new version delivers:

  * Discord community (shared with xow, ask to join your project)
  * Code generation assertions
  * Start of code-redesign for splitting the driver core
  * Start of profile support (not implemented yet)
  * Dropped old debug mode
  * Better compliance with the Linux Gamepad specification
  * Improved driver documentation
  * Improved hardware documentation
  * Improved hardware support
  * Improved quirks handler to more easily handle hardware quirks
  * Improved HID handler performance
  * Improved dmesg logging
  * Improved DKMS installer
  * Improved rumble timing and precision
  * Improved rumble damping setting
  * Improved continuous integration testing
  * Added support for new hardware-level device functions
  * Added high-precision mode for the thumb sticks
  * Added option for disabling dead-zones for better Proton support

Many of the bug fixes have been backported to the v0.8 branch which has now
received its end of life. If you're having problems with the new features,
please report that and use the v0.8 branch until that specific bug is fixed.


## Known Problems

The Bluetooth stability of the Xbox controllers is still a known issue which we
probably can't fix in the driver. Some work-arounds are in place but that's not
a 100% guarantee. Some of these problems can even be observed when using the
controller with Windows. All the problems seem to mostly focus around using
rumble, so you may need to disable rumble.

The other class of problems is connecting/pairing the controller properly. This
seems to be a Linux kernel issue or a Bluez daemon issue, or a combination of
both. Usually, Windows doesn't show any problems here. Apparently, we cannot
fix this in the driver. You may want to report this to the kernel or Bluez
developers.

Always use the controller updated to its latest firmware before trying other
steps: Microsoft fixed some connection problems, and also HID implementation
details in the past with firmware updates.


## Breaking Changes

This release fixes button mappings for 8BitDo controllers to actually match
their names on the controller instead of matching positions with original Xbox
controllers. The X,Y and A,B buttons will be swapped compared to previous
versions.

This change is controversial but let me tell you that we are going to work on
changes that let you easily choose and switch behavior in v0.10.

The module parameter `debug_level` has been dropped. Some other parameters
have been dropped in favor of better replacements. Please reconfigure your
parameters and settings.


## The Future

The new controllers provide some new functionality at the hardware level which
we are going to try to emulate at the driver level for older generation
controllers. The prominent example for this is mapping profiles support: Until
we figured out how this is implemented in hardware for the Xbox Elite 2
controller, we won't implement any emulation support in the driver. This is
because we want to end up with an emulation that is 100% compatible with the
hardware implementation, and not end up with some customization functions that
needs emulation for the Xbox Elite 2 controller.

We are also going to split the driver into components to easily add support for
USB or GIP dongle connections, with USB probably coming first because the GIP
dongle still has licensing issues for using its firmware.


## Headlines:

  * hid-xpadneo, quirks: We need to carry a quirk for Linux button mappings
  * hid-xpadneo, rumble: Migrate damping to generic attenuation parameter
  * installer: Change to base directory first
  * xpadneo, deadzones: Implement a high-precision mode without dead zones
  * xpadneo, udev: Work around libinput using the controller as touchpad

```
Kai Krakow (78):
      hid-xpadneo: Drop `debug_level`
      hid-xpadneo, quirks: Pass quirks from driver data
      udev: Expose all xpadneo input devices as user-readable
      hid-xpadneo, profiles: Prepares profile switching for customization
      Update bug_report.md
      docs, news: Document breaking changes.
      hid-xpadneo, rumble: Migrate damping to generic attenuation parameter
      xpadneo, udev: Work around libinput using the controller as touchpad
      hid-xpadneo: Tell the user which controller connected
      hid-xpadneo, quirks: Fix a typo
      hid-xpadneo, quirks: Expand flags to 32 bit
      hid-xpadneo, quirks: We need to carry a quirk for Linux button mappings
      xpadneo, quirks: Add Nintendo mappings quirk for 8BitDo controllers
      docs: Fix a syntax problem in SDL link
      xpadneo, deadzones: Use smaller dead zone and fuzz for precision
      xpadneo, deadzones: Implement a high-precision mode without dead zones
      docs: Document another storage location for SDL gamepad mappings
      docs: Document SDL HIDAPI breakage
      hid-xpadneo, init: Ignore HID_CONNECT_FF
      hid-xpadneo, quirks: Convert to proper bit values
      hid-xpadneo: Update copyright
      configure, cleanup: Remove the tedious whitespace
      xpadneo, cleanup: Fix missing newline at end of file
      hid-xpadneo, cleanup: Remove some more comments
      hid-xpadneo: Replace `combined_z_axis` with additional axis
      hid-xpadneo: Stop spamming the HID layer with repeated reports
      Revert "hid-xpadneo, quirks: Convert to proper bit values"
      hid-xpadneo, timing: Use clamp() instead min()/max()
      hid-xpadneo, rumble: Tighten the rumble timing
      hid-xpadneo: Fix potentially loosing input packets for XBE2 controllers
      docs: Add Repology badge
      docs: Mention MissionControl sibling project
      docs: Cleanup some whitespace
      docs: Add xpadneo logo
      docs: Add Discord badge
      installer: Drop VERSION tag file from master branch
      installer: Fix white space
      installer, dkms: Prevent showing readlink errors
      installer: Add verbose mode
      installer: Exit on unexpected errors
      installer, dkms: Skip ERTM if setting is not writable
      tests: Add verbose mode to Azure Pipeline
      tests: Also test uninstallation in Azure Pipeline
      tests: Dump make.log to stdout on verbose DKMS error
      tests: Run Azure Pipeline on multiple Ubuntu LTS versions
      hid-xpadneo: Ignore trigger scale switches
      hid-xpadneo, profiles: Log to kernel starting with lower-case
      hid-xpadneo, rumble: Remove useless use of max()
      hid-xpadneo: Optimize delay_work clamping
      hid-xpadneo, rumble: Use proper integer rounding in calculations
      hid-xpadneo, rumble: Limit command duration
      hid-xpadneo: Fix kernel coding standards
      docs: Remove bogus blank line
      docs: Do not misuse back-ticks
      docs: Document broken packet format of XBE2 v1
      hid-xpadneo: Handle XBE2 v2 packet format
      hid-xpadneo: Add XBE2 trigger scale setting
      docs: Fix collaboration referral
      docs: Fix a typo
      hid-xpadneo: Reserve another bit for the new XBXS share button
      hid-xpadneo: Document XBXS modes and PIDs
      docs: Document XBXS controller support in the README
      docs: Restructure text about profile support
      docs: Add BLE note for the XBXS controller
      docs: Move profile switching section
      hid-xpadneo: Make assertions of hardware buffer sizes
      hid-xpadneo: Make rumble motor bits a full enum type
      installer: Move version information to separate include file
      installer: Remove excessive blank lines
      installer: "INSTALLED" is an array
      configure: Use braces around variables
      installer: Change to base directory first
      hid-xpadneo: Alias the Share button
      hid-xpadneo: Fix a comment about rumble timing
      hid-xpadneo: Improve PID documentation
      docs: Document Bluetooth low energy requirements
      installer: Also fix the updater
      hid-xpadneo: Move headers to separate file

Ben Schattinger (2):
      docs: Document Xbox Series X/S controller
      hid-xpadneo: Add Xbox Series X / S controller support
```


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
