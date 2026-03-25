<!-- SPDX-License-Identifier: GPL-2.0-only -->

# Changes since v0.10 up to v0.10.1

This is a focused maintenance release for the v0.10 series with an emphasis on stability and compatibility.

The main work in v0.10.1 targets rumble reliability. We now advertise rumble support early while deferring actual
rumble traffic until initialization is fully complete, reducing race conditions with userspace stacks that probe
capabilities immediately after hotplug. In addition, rumble locking was corrected to properly preserve IRQ state,
fixing a regression that could lead to hard lockups under force-feedback load.

Beyond rumble, this update improves day-to-day usability and hardware coverage: the optional virtual mouse device can
now be disabled completely, quirk detection was extended for newer GameSir Nova variants, and several
documentation/debugging details were cleaned up.

As a v0.10 maintenance release, this branch intentionally keeps the existing ff-memless-based architecture. The larger
native-rumble rework remains part of the v0.11 development line.

Thanks to everyone who reported regressions, provided traces, and helped validate fixes on real hardware and different
userspace environments.


## Headlines:

  - xpadneo, docs: Replace back ticks syntax for shell substitution
  - xpadneo, mouse: Allow disabling the mouse device completely
  - xpadneo, quirks: Add OUI flag checks for new GameSir Nova
  - xpadneo, rumble: Do not send rumble commands unless ready
  - xpadneo, rumble: Properly save and restore IRQ state during locking

```
Kai Krakow (17):
      xpadneo, licensing: Link the licenses for convenience
      xpadneo, rumble: Remove rumble accumulation
      xpadneo, rumble: Rename private function `xpadneo_rumble_welcome`
      xpadneo, rumble: Introduce helper to calculate throttling delay
      xpadneo, rumble: Migrate to more modern scoped locks
      xpadneo, docs: Replace back ticks syntax for shell substitution
      xpadneo, rumble: Tighten the rumble throttle timing
      xpadneo, docs: Fix style of `disable_shift_mode`
      xpadneo, power: Fix header export signature
      xpadneo, debug: Add new debug mode to improve logging for new devices
      xpadneo, debug: Move HID report debugger to new debug module
      xpadneo, quirks: Add OUI flag checks for new GameSir Nova
      xpadneo, debug: Add Xbox Wireless Controller modern descriptor
      xpadneo, debug: Add Xbox Elite 2 Controller descriptor
      xpadneo, mouse: Allow disabling the mouse device completely
      xpadneo, rumble: Do not send rumble commands unless ready
      xpadneo, rumble: Properly save and restore IRQ state during locking
```


# Changes since v0.9 up to v0.10

*Code name:*
> COUCH POTATO

This is a major release that bundles roughly five years of development since v0.9. During that time, xpadneo received
substantial internal refactoring, broader device support, many compatibility fixes for modern userspace and kernel
changes, and a lot of long-term maintenance work that is not always visible at first glance.

The codename reflects one of the headline additions: xpadneo can now expose controller input as a virtual mouse. This
enables couch-friendly navigation scenarios and rounds out the driver in a practical way beyond pure gamepad usage.
At the same time, this feature is intentionally marked as experimental for now so we can continue to refine its
behavior and ergonomics in v0.11.

Another key focus of this cycle was improving real-world compatibility: updated quirks and heuristics for additional
vendors and controller models, better handling of firmware and profile differences, and more robust interoperability
with SDL/Steam/hidraw environments where mapping conflicts can otherwise become painful.

We also spent significant effort on quality-of-life improvements around installation and packaging, especially in DKMS
and udev handling, plus updated troubleshooting and configuration guidance for current Linux distributions and
Bluetooth setups.

A release of this size is only possible with continued support from users, contributors, testers, package maintainers,
and everyone reporting regressions and sharing detailed logs over the years.

Special thanks to all contributors who helped shape this cycle, including:

  - Florian Dollinger (`@atar-axis`) for the original project foundation and long-standing code base that made this
    evolution possible.
  - Ben Schattinger (`@lights0123`) for Xbox Series X|S support contributions and Share button related integration
    work.
  - Erik Hajnal (`@ehats`) for non-trivial XBE2 mode support work and descriptor/mapping related fixes.
  - Dugan Chen (`@duganchen`) for Elite Series 2 related device support updates.
  - John Mather (`@NextDesign1`) and many others for incremental fixes, reports, and ecosystem feedback that helped
    improve edge-case handling over time.


## IMPORTANT: Updated Licensing

As part of the ongoing refactor, the kernel module code has been relicensed to **GPL-2.0-only**. This change was made
to ensure compatibility with the Linux kernel's licensing requirements and to allow for better integration with the
kernel's codebase. Non-driver content, such as documentation and tooling, remains licensed under **GPL-3.0-or-later**
unless specified otherwise in file headers.

Package maintainers and contributors should take note of this change when distributing or contributing to the project,
ensuring that they comply with the new licensing terms for the relevant parts of the codebase.

As part of this change, we also added AppStream metadata to the repository, which should help with integration into
Linux distribution packaging systems and improve discoverability of the project for users. Thanks to Michael Lloyd
(`@michael-lloyd`) for the contribution of the AppStream metadata.


## The Future

- The virtual mouse mode introduced in this release is still **experimental** and will be improved further during the
  v0.11 cycle, with improvements backported to v0.10 if possible.
- The next release cycle will bring better rumble handling and lay the foundations for profile customization support.
- Hopefully, we will also improve and streamline the documentation further, especially around configuration and
  troubleshooting, to make it easier for users to get the most out of their controllers with xpadneo.


*Thank you for your patience, testing effort, and continued trust in xpadneo.*


## Headlines:

  - core, quirks: Add GameSir T4 Nova Lite support
  - core, quirks: Add GuliKit KK3 MAX quirks
  - core, quirks: Add heuristics to detect GameSir Nova controllers
  - dev: Fix gitignore
  - dkms: Explicitly add version to the install phase
  - dkms: Suggest trusting the git directory if version detection failed
  - docs(#429): Elite S2 Profiles carry over
  - docs: Document Bluetooth LE issues and work-arounds
  - docs: Document workarounds for the Xbox Wireless controller
  - docs: Replace Ko-fi link to attribute donations to current maintainer
  - hid-xpadneo: Allow building with kernel 6.12
  - hid-xpadneo: Do not use Nintendo layouts by default
  - hid-xpadneo: Move share button quirk to `xpadneo_devices` database
  - installer, dkms: Fix trying to install the wrong udev filename
  - xpadneo: Add support for GuliKit KingKong2 PRO controllers
  - xpadneo, core: Add configuration for disabling Xbox logo shift-mode
  - xpadneo, core: Deprecate synthetic rolling axis from triggers
  - xpadneo, core: Fix coding style
  - xpadneo, core: Remove hard requirement of ida_{alloc,free}
  - xpadneo, dkms: Drop deprecated variable CLEAN
  - xpadneo, dkms: Get rid of redundant file installs/removes
  - xpadneo, dkms: Hooks are now effectively a no-op
  - xpadneo, docs: Align all of README.md with current distribution practice
  - xpadneo, docs: Link troubleshooting more prominently
  - xpadneo: Evade SDL-mismapping once again
  - xpadneo: Fix documentation about Share button
  - xpadneo, fixups: Adapt SDL fixup to fix the Xbox button
  - xpadneo, hid: Move paddles to range BTN_TRIGGER_HAPPY5
  - xpadneo, hidraw: Fixup previous commit to properly work with DKMS
  - xpadneo, hidraw: Work around other software messing with our udev rules
  - xpadneo, init: Actually save rumble test values before we replace them
  - xpadneo: Map Share button to F12 by default
  - xpadneo, mouse: Implement mouse support
  - xpadneo, quirks: Add another Microsoft OUI
  - xpadneo, quirks: Another Microsoft OUI
  - xpadneo, quirks: Fix the haptics quirks for third-party controllers
  - xpadneo, quirks: Let another Microsoft OUI bypass heuristics
  - xpadneo, quirks: Prevent applying heuristics for some known vendors
  - xpadneo: Remove deprecated ida_simple_{get,remove}() usage
  - xpadneo: Revert fixups on device removal
  - xpadneo, rumble: Fix rumble throttling for modern firmware
  - xpadneo: Support GameSir T4 Cyclone models
  - xpadneo: Work around invalid mapping in Steam Link

```
Kai Krakow (278):
      xpadneo, udev: Properly re-order modalias
      xpadneo, udev: Add Xbox One X|S PIDs to udev and modalias
      hid-xpadneo: Improve SDL2 work-arounds to fix XBE2 button mappings
      hid-xpadneo: Fix SDL2 button mapping for XBXS
      docs: Fix typo
      hid-xpadneo: Print version during load
      hid-xpadneo: Ignore more files from current kernel tool chain
      hid-xpadneo: Add some benchmark instrumentation
      hid-xpadneo: Fix typo in comment
      hid-xpadneo: Fix logging SDL2 work-around on wrong condition
      hid-xpadneo: Log and record the original descriptor size
      installer: Fix indentation
      Makefile: Create version.h on the fly
      dev: Fix gitignore
      hid-xpadneo: Move share button quirk to `xpadneo_devices` database
      hid-xpadneo: Improve descriptor logging
      hid-xpadneo: Deprecate directional rumble
      src: re-indent with newer indent version
      docs: List incompatible Bluetooth chipsets and settings
      docs: Improve troubleshooting instructions
      docs: Amend list of bugs with known fixes
      docs: Use Xbox Series X|S controller name consistently
      udev: Update rules for upcoming systemd update
      hid-xpadneo: Do not use Nintendo layouts by default
      hid-xpadneo: Split gamepad into subdevices
      hid-xpadneo: Send KEY_MODE for the Xbox button
      hid-xpadneo: Enable detection of the XBE2 keyboard sub device
      xpadneo: Drop kernel compatibility below version 4.18
      docs: Replace Ko-fi link to attribute donations to current maintainer
      hid-xpadneo: Report missing applications just once
      hid-xpadneo: Drop dead module parameter `combined_z_axis`
      docs: Put Ko-fi on a separate line
      docs: Make original driver announcement a quote
      docs: Link xow author
      hid-xpadneo: Auto-detect hardware profiles instead of hard-coding it
      docs: Add missing text about profile support
      hid-xpadneo, udev: Move udev rules up in rules priority
      configure: Remove disable-ff in favor of trigger-rumble-mode
      configure: No options is an error, don't do anything
      configure: Do not require dkms for configuration
      dkms: Install files with proper permission
      installer, dkms: Fix trying to install the wrong udev filename
      hid-xpadneo: Allow modparams for manual re-installation
      docs: Document more of the quirk flags
      hid-xpadneo, Makefile: Make version.h an intermediate target
      hid-xpadneo: Use jiffies converter functions instead of HZ
      hid-xpadneo: Allow adding and removing quirk flags
      hid-xpadneo: Fix indentation
      xpadneo, mouse: Support toggling mouse mode
      dkms: Fix CI
      dkms: Simplify installation
      xpadneo: Drop dynamic version file handling
      docs: Document rumble behavior with SDL_JOYSTICK_HIDAPI
      docs: Fix typo
      docs: Add note about holding the Guide button for too long
      hid-xpadneo: Sync consumer key events and stop processing
      xpadneo: Add current maintainer to module authors
      hid-xpadneo: Move SDL work-arounds to parent device
      hid-xpadneo: Revoke hidraw user access to fix a Proton mapping problem
      docs: Update pairing instruction to mitigate stability issues
      kbuild: Add work-around for matching source and object filename
      xpadneo, core: Add support for synthetic devices
      xpadneo: Fix "consumer control" naming
      xpadneo, consumer: Add synthetic consumer control device on demand
      reporting: Add some space for easy insertion
      reporting: Improve headers
      reporting: Ask for applied firmware updates
      automation: Extend the feedback response deadline
      automation: Remove comments
      reporting: Add model identification
      reporting: Add installed software section
      automation: Move no-response.yml to proper location
      misc, docs: Remove ERTM patches and update docs
      hid-xpadneo: Map instead of disable duplicate button "AC Back"
      docs: Document Bluetooth LE issues and work-arounds
      docs: Remove `Privacy=device` in favor of JustWorks re-pairing
      docs: Mention the xone project which has gone public now
      reporting: Improve formatting of reporting template
      xpadneo, consumer: Fix comment
      docs: List distribution packages
      hid-xpadneo, rumble: Do not lose rumble strength while throttled
      docs: Fix report descriptor syntax errors
      xpadneo: Work around invalid mapping in Steam Link
      xpadneo, hidraw: Also work around SDL2 hidraw mode conflicts
      dkms: Create version instance in DKMS source archives
      dkms: Explicitly add version to the install phase
      dkms, installer: Increase verbosity
      docs: Add note about audio support
      xpadneo, udev: Make udev rule logic more readable
      xpadneo: Add support for XB1S BLE firmware update
      xpadneo: Revert fixups on device removal
      xpadneo: Update devices db for PID 0x0B13
      xpadneo: Add XBE2 firmware 5.13 support
      xpadneo: Add paddles support
      xpadneo, rumble: Fix rumble throttling for modern firmware
      docs: Clarify some points
      xpadneo, core: Warn about old firmware version with stability issues
      dkms: Suggest trusting the git directory if version detection failed
      dkms: Add another status line variant to split module and version
      docs: Update documentation about the XBE2 paddles
      docs: Update the documentation to use the "add quirks" feature
      github: Change checkboxes to proper md format
      github: Reorder checkboxes in bug report
      github: Add problematic software to bug reports
      github: Improve bug reports for identifying the failing layer
      github: Improve feature requests
      xpadneo, fixups: Apply SDL fixups unconditionally
      xpadneo, fixups: Adapt SDL fixup to fix the Xbox button
      docs: Update description for the xone project
      docs: Use capitalization for headlines consequently
      docs: Don't exceed line length
      docs: Fix typo
      docs, installer: Add notes about needed kernel features
      xpadneo, rumble: Remove deprecated directional rumble
      docs: Remove obsolete ERTM notes
      xpadneo: Evade SDL-mismapping once again
      xpadneo, keyboard: Add keyboard support
      xpadneo, consumer: Move buttons to keyboard device
      xpadneo: Map Share button to F12 by default
      xpadneo: Move bit swap helper to include file
      xpadneo, init: Fix rumble testing logic on connect
      xpadneo: Add support for GuliKit KingKong2 PRO controllers
      xpadneo, init: Actually save rumble test values before we replace them
      xpadneo: Upgrade CI to Ubuntu 22.04
      xpadneo: Remove Ubuntu 18.04 from CI
      docs: Fix headline for GuliKit controllers
      Makefile: Use kernel build symlink from `/lib/modules`
      xpadneo: Fix documentation about Share button
      xpadneo, init: Do not report paddles unconditionally
      xpadneo, hid: Move paddles to range BTN_TRIGGER_HAPPY5
      xpadneo, init: Really exclude the keyboard event from the HID bitmap
      xpadneo, init: Remove bogus keys from the keymap
      xpadneo, init: Detect HW profile support on current firmwares
      xpadneo: Support GameSir T4 Cyclone models
      xpadneo, docs: Reintroduce hint about using SDL_JOYSTICK_HIDAPI=0
      xpadneo, core: Sort device quirks into proper order
      xpadneo, quirks: Combine quirks that mean "no haptics support"
      xpadneo, core: Deprecate synthetic rolling axis from triggers
      docs: Improve BT_DONGLES.md
      docs: Remove feature claims that are obsolete or no longer true
      docs: Fix wording and casing
      docs, README: Remove the broken Codacy badge
      xpadneo, quirks: Fix the haptics quirks for third-party controllers
      xpadneo, core: Reorder devices by PID
      xpadneo, core: Remove explicit sync when not needed
      xpadneo, core: Handle sync events in xpadneo
      docs: Add troubleshoot instructions for controllers requesting a PIN
      docs: Add troubleshooting instructions after upgrading firmware
      docs: Add missing blank line
      docs: Add compatibilty status for a Simplecom Bluetooth dongle
      docs, README: Add notes for package maintainers and on module signing
      github: Migrate stale and close automation
      github: Improve feature request template
      github: Do not mark issues stale automatically
      xpadneo, core: Remove code duplication
      xpadneo, hidraw: Work around other software messing with our udev rules
      hid-xpadneo: Allow building with kernel 6.12
      xpadneo, hidraw: Fixup previous commit to properly work with DKMS
      hid-xpadneo, core: Match kernel 6.12 report_fixup signature
      xpadneo, core: Fix coding style
      core, quirks: Add GameSir T4 Nova Lite support
      core, quirks: Add heuristics to detect GameSir Nova controllers
      core, quirks: DRY the rumble tests
      core, tests: Fix the pulse test and print test info
      core, debug: Allow output of decoded rumble packets for debugging
      core, quirks: Provide a way to remove quirks added by heuristics
      xpadneo, debug: Fix indentation and output
      core, quirks: Add GuliKit KK3 MAX quirks
      xpadneo, quirks: Prevent applying heuristics for some known vendors
      xpadneo, quirks: Let another Microsoft OUI bypass heuristics
      xpadneo, installer: Fix some general issues with the scripts
      xpadneo, dkms: Match minimum kernel version with source code
      xpadneo, installer: Add new option parser to support multiple options
      xpadneo, dkms: Add kernel source dir option for easier testing
      xpadneo, installer: Allow forcing replacement of a newer kernel module
      xpadneo, quirks: Another Microsoft OUI
      xpadneo, installer: Exit immediately on error
      xpadneo: Add documentation about testing pull requests
      xpadneo, installer: Fix option parser to not bubble up an error
      xpadneo, dkms: Use "yes" for autoinstall instead of "Y"
      xpadneo: Reorder gitignore
      xpadneo, ci: Add missing dependencies
      xpadneo, ci: Add Ubuntu 24.04
      xpadneo, examples: Build hidraw properly on Ubuntu
      xpadneo, tools: Build hidraw_c test example in CI
      xpadneo, dkms: Remove DKMS work-around and let DKMS handle the source
      xpadneo, installer: Use make to regenerate dkms.conf
      xpadneo, installer: Introduce make-based installer
      xpadneo, installer: Use the new make infrastructure
      xpadneo, dkms: Add make-based installer to CI
      xpadneo, docs: Fix various spelling mistakes and phrasing
      xpadneo, installer: Fix usage of `ETC_PREFIX`
      xpadneo, installer: Re-order config variables to the top of `Makefile`
      xpadneo, installer: Support installing documentation
      xpadneo, docs: Fix indentation, line length and whitespace
      xpadneo, docs: Document the new quirk flags for configuration
      xpadneo, docs: Use dashes instead of stars for list items
      xpadneo, docs: Fix use of bare URLs
      xpadneo, core: Remove left-over xdata indirection
      xpadneo, installer: Delay `set -e` to not silence errors early
      xpadneo, quirks: Another Microsoft OUI
      xpadneo, quirks: Another Microsoft OUI
      xpadneo, ci: Drop Ubuntu 20.04 CI testing
      xpadneo, installer: Get rid of ERTM work-around by requiring kernel 5.12
      xpadneo, installer: Do not fail uninstallation early
      xpadneo, udev: Trigger loading of `uhid` before `bluetooth`
      xpadneo, psy: Fix potential pointer leak during battery registration
      xpadneo, psy: Fix memory leak of device and battery name
      xpadneo, core: Actually check the correct devm pointer in error path
      Makefile: Do not leave a stray intermediate source file behind
      xpadneo, docs: Align CONFIGURATION.md with current implementation
      xpadneo, docs: Align README.md with current distribution practice
      xpadneo, probe: Implement error path for ida_simple_get()
      xpadneo: Remove deprecated ida_simple_{get,remove}() usage
      xpadneo, headers: Drop kernel version check kept for cherry picking
      xpadneo, docs: Document sensor drift and deadzones better
      xpadneo, docs: Link troubleshooting more prominently
      xpadneo, dkms: Fix shellcheck for unused variables
      xpadneo, dkms: Drop deprecated variable CLEAN
      xpadneo, docs: add GPL-2.0 relicensing requirement
      xpadneo, licensing: Add driver relicensing tracking document
      xpadneo, licensing: Record consent from Ben Schattinger
      xpadneo, licensing: Record consent from Erik Hajnal
      xpadneo, licensing: Record consent from Dugan Chen
      xpadneo, licensing: Record consent from yjun
      xpadneo, licensing: Record consent from John Mather
      xpadneo, licensing: Do not chase consent from PiMaker
      xpadneo, dkms: Fix warning about unassigned variables
      xpadneo, dkms: Get rid of redundant file installs/removes
      xpadneo, dkms: Hooks are now effectively a no-op
      xpadneo, docs: Improve bug reporting template
      xpadneo, docs: document safe firmware flashing for Xbox controllers
      xpadneo, docs: improve Bluetooth dongle documentation
      xpadneo, docs: add user-reported Bluetooth dongle compatibility report
      xpadneo, docs: add PCIe Bluetooth adapter compatibility reports
      xpadneo, profiles: Report current profile as absolute axis
      xpadneo, docs: Unify list formatting of README.md
      xpadneo, docs: Align all of README.md with current distribution practice
      xpadneo, core: Remove hard requirement of ida_{alloc,free}
      xpadneo, core: Support explicit paddle button usages
      xpadneo, docs: Fix manufacturer name of a tested USB BLE adapter
      xpadneo, quirks: Add another Microsoft OUI
      xpadneo: Add LSP support for clangd
      xpadneo: Get rid of hid-ids.h
      xpadneo: Add .clangd to fix some more LSP warnings
      xpadneo, core: Do not reset profile switch indicator unless emulated
      xpadneo, core: Honor Guide+Select mouse toggle with hardware profiles
      xpadneo, mouse: Implement mouse support
      xpadneo, core: Remove unused headers
      xpadneo, power: Move psy/battery function out of the legacy core
      xpadneo, quirks: Move quirks handling out of the legacy core
      xpadneo, mouse: Unify mouse driver naming conventions and structure
      xpadneo, mappings: Move usage mappings out of the legacy core
      xpadneo, profiles: Move events and profiles out of the legacy core
      xpadneo, rumble: Move rumble handling outside of the legacy core
      xpadneo, core: Move remaining code out of the legacy core
      xpadneo, synthetic: Move helpers for synthetic drivers out of the core
      xpadneo, devices: Move device helpers out of the legacy core
      xpadneo, drivers: Streamline naming convention
      xpadneo: Rename FF to RUMBLE to better match the functionality
      xpadneo, device: Move kernel backward compatibility to separate header
      xpadneo, compat: Move remaining compatibility functions into compat.h
      xpadneo, core: Move HID_QUIRK_INPUT_PER_APP check to core
      xpadneo, core: Move driver versioning to core
      xpadneo, helpers: Move shared helpers into separate header file
      xpadneo, headers: Move private definitions out of shared headers
      xpadneo: Do not use "extern" on the implementation function signature
      xpadneo: Align private and public symbol prefixes
      xpadneo, core: Improve error paths on initialization failures
      xpadneo, device: Fix potential format mismatch at compile time
      xpadneo, quirks: Improve quirks argument parser
      xpadneo: Refactor subdevices to re-use code and common error paths
      xpadneo, events: Warn once if shift mode is active
      xpadneo, events: Explain why no explicit sync is needed for ABS_PROFILE
      xpadneo, events: Degrade shift mode warning to notice log level
      xpadneo, CI: Add Github workflow to check commits
      xpadneo, installer: Add metainfo as optional install target

Alexander Tsoy (2):
      docs: Suggest alternative command to get controller info
      docs: Add compatibilty status for UGREEN CM390

l3d00m (2):
      xpadneo, docs: Update links to other projects
      xpadneo, docs: Fix typo and add specific hardware support to xone

Aniket (1):
      Remove outdated secure boot docs

Eduard Tolosa (1):
      xpadneo, udev: update rules for systemd v258+

Emil Velikov (1):
      dkms: remove REMAKE_INITRD

Florian Dollinger (1):
      Update README.md

John Mather (1):
      xpadneo, quirks: Add another OUI

Kevin Locke (1):
      Add BUILD_EXCLUSIVE_CONFIG to dkms.conf

Luis Davila (1):
      Replaced Antergos with EndeavorOS

Matthew Scharley (1):
      bluez-tools isn't required on Tumbleweed

Michael-Lloyd (1):
      xpadneo, meta: Add AppStream metainfo with hardware modalias

Moté (1):
      docs: Document workarounds for the Xbox Wireless controller

Scott Bailey (1):
      Doc update for KK3 Max

Willster Johnson (1):
      docs(#429): Elite S2 Profiles carry over

arnxxau (1):
      added ff_connect_notify config option to configure.sh

bouhaa (1):
      xpadneo, core: Add configuration for disabling Xbox logo shift-mode

jbillingredhat (1):
      README.md: Fedora instructions are incorrect

liberodark (1):
      docs: Update BT_DONGLES

mikaka (1):
      Don't disable ERTM if kernel 5.12 or later

ryanrms (1):
      Adding openSUSE to readme.md

thiccaxe (1):
      Update TROUBLESHOOTING.md

yjun (1):
      xpadneo: Fix missing report check
```


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

- Discord community (shared with xow, ask to join your project)
- Code generation assertions
- Start of code-redesign for splitting the driver core
- Start of profile support (not implemented yet)
- Dropped old debug mode
- Better compliance with the Linux Gamepad specification
- Improved driver documentation
- Improved hardware documentation
- Improved hardware support
- Improved quirks handler to more easily handle hardware quirks
- Improved HID handler performance
- Improved dmesg logging
- Improved DKMS installer
- Improved rumble timing and precision
- Improved rumble damping setting
- Improved continuous integration testing
- Added support for new hardware-level device functions
- Added high-precision mode for the thumb sticks
- Added option for disabling dead-zones for better Proton support

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

- hid-xpadneo, quirks: We need to carry a quirk for Linux button mappings
- hid-xpadneo, rumble: Migrate damping to generic attenuation parameter
- installer: Change to base directory first
- xpadneo, deadzones: Implement a high-precision mode without dead zones
- xpadneo, udev: Work around libinput using the controller as touchpad

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

- Thanks to Adam Becker for adding some bug and style fixes.
- Lars-Sören Steck fixed a scripting bug, thank you.
- Manjaro users now see instructions for their favorite distribution,
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

- dmks, installer: Move etc sources one level up
- docs, mapping: Document and explain `joydev` mapping
- hid-xpadneo: Adhere to Linux Gamepad Specification
- hid-xpadneo, battery: Rework detection, parsing and reporting
- hid-xpadneo, compat: Fix compilation on kernel 4.19
- hid_xpadneo, quirks: Add "8BitDo SN30 Pro+"
- hid-xpadneo, rumble: Throttle reprogramming of rumble motors
- installer: Parse DKMS version info correctly
- installer: Use awk in non-GNU mode

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

- fix direction_rumble_test path error (#168)
- hid-xpadneo: Convert mapping to using tables instead of code
- Pretend we are in Windows wireless mode

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
