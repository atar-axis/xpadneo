<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# HID Descriptor for XBOX Nano Controller (Bluetooth LE)

Measured on an XBOX Nano Controller connected via Bluetooth LE:

- VID/PID `045E:0B4F`, HID version `0x0609`, BLE firmware 6.09
- OUI field `CC:C1:09` (globally-assigned, unicast)
- 285 bytes, crc16 `0xa56e`

Input report `0x01` carries the sticks and both triggers, report `0x05` carries
the keyboard application. There is a single vendor collection (usage page
`0xFF45`, usage `0x63`, report ID `0x0C`) which xpadneo leaves unhandled.

One detail differs from the older Xbox Wireless Controller (BLE, `045E:0B13`)
descriptor of the same family: the Simulation Controls page is declared **once**
and then shared by both trigger usages, instead of being repeated for each. The
existing fixups look for one page declaration per usage, so they do not match
this shape. The generic `Rx`/`Ry` fixups still apply at offsets 34/36 (the
second stick's `Z`/`Rz`), but `Brake`/`Accelerator` stay on the Simulation page
and are reported as `ABS_BRAKE`/`ABS_GAS`, leaving the `ABS_Z`/`ABS_RZ` axes
that xpadneo always advertises unfilled.

For that fixup the shared page declaration starts at byte 41 (`05 02`), `Brake`
at byte 43 (`09 C5`) and `Accelerator` at byte 58 (`09 C4`) - those are the
indices the guards in `device.c` actually test (`rdesc[41..44]` and
`rdesc[58..59]`). No other usage lies between them, so remapping the page
declaration to Generic Desktop and renaming the two usages affects only the
triggers.

Verified with `evtest` on real hardware after the fixup: `ABS_X`/`ABS_Y` and
`ABS_RX`/`ABS_RY` span the full ±32768 range, `ABS_Z`/`ABS_RZ` span 0..1023 for
both triggers, and neither `ABS_GAS` nor `ABS_BRAKE` is advertised any more.

Unpatched descriptor as dumped by xpadneo with `debug_descriptor=1`:
```
00000000: 05 01 09 05 a1 01 85 01 09 01 a1 00 09 30 09 31 15 00 27 ff ff 00 00 95 02 75 10 81 02 c0 09 01
00000020: a1 00 09 32 09 35 81 02 c0 05 02 09 c5 26 ff 03 95 01 75 0a 81 02 75 06 81 03 09 c4 75 0a 81 02
00000040: 75 06 81 03 05 01 09 39 46 3b 01 65 14 15 01 25 08 75 04 81 02 81 03 05 09 19 01 29 0f 45 00 65
00000060: 00 15 00 25 01 95 0f 75 01 81 02 05 0c 09 b2 95 01 81 02 85 03 05 0f 09 21 a1 02 09 97 75 04 91
00000080: 02 91 03 09 70 25 64 95 04 75 08 91 02 09 50 55 0e 26 ff 00 95 01 91 02 09 a7 09 7c 55 00 95 02
000000a0: 91 02 c0 c0 05 01 09 06 a1 01 85 05 05 07 19 e0 29 e7 25 01 95 08 75 01 81 02 95 01 75 08 81 03
000000c0: 19 01 29 a4 15 01 26 a4 00 95 06 81 00 c0 05 01 09 02 a1 01 85 0e 09 01 a1 00 05 09 19 01 29 03
000000e0: 15 00 25 01 95 03 75 01 81 02 95 01 75 05 81 03 05 01 09 30 09 31 15 80 25 7f 95 02 75 08 81 06
00000100: c0 c0 06 45 ff 09 63 a1 01 85 0c 09 63 15 00 26 ff 00 95 3f 82 02 01 09 63 92 02 01 c0
```

The model has no vibration motors - "The XBOX Nano Controller does not include
haptics" (Xbox Support) - so rumble output cannot be observed on it, even though
the driver declares the output report and enables rumble streaming.

Buttons are numbered non-standard on this model, and the shift is not uniform:
A and B arrive as buttons 1 and 2, but X and Y are 4 and 5, LB and RB are 7 and
8, View and Menu are 11 and 12, Guide is 13 and LS/RS are 14 and 15. Because
hid-input decodes those usages through the global table, every one of them comes
out as the neighbouring button: X reports as Y, Y as LB, LB as View, RB as Menu,
and View/Menu land on unrelated codes while Guide comes out as Share. They are
remapped through the per-device table in `mappings.c` rather than by extending
the global table, because other controllers pad those bits out.

Guide is worth calling out because it moves two usages, from 11 to 13, and lands
in the global table's Share slot: five presses of the Xbox logo produced 57
`KEY_F12` events on the keyboard sub-device, which is how the usage was finally
identified. The configurable Action button below the logo has no input of its own. Pressing
it three times inside a 54-report capture produced two button transitions, both of
them the A presses on either side of it, and `btmon` saw 54 ATT notifications on
the HID input characteristic (`0x0025`) and none on any other handle. It is absent
from the input report, from every other declared report ID and from the vendor GATT
traffic, so it is left unmapped here. An earlier capture that seemed to attribute
usage 13 to it was in fact the logo.

What it is instead: the Xbox Accessories app lists it as **Action**, with three
independently assignable press types (short, long and double) plus an `Enable
Multipress` toggle. All three defaults are console-only - `Take screenshot (XBOX
only)`, `Record what happened (XBOX only)` and `Share recent captures (XBOX only)`
- which is why the button reports nothing on a PC out of the box. It cannot be
found by pressing it on an unconfigured controller: it emits only what it has been
assigned, and it has no usage, report ID or bit of its own.

Measured with short press assigned to **View**, the profile saved and that profile
selected: the button emits `BTN_SELECT`, the same key View itself sends. Four
captures - `Action x3 + View x1` produced four `BTN_SELECT` presses, `View` alone
produced one, `Action x2` with View untouched produced two, and over Bluetooth
`A, X, LB, Action` produced 304/307/310/314. Nothing distinguishes the assignment
from the real button in the report, so there is nothing for the driver to map.
Pressing it also produces no LED change - the logo stays solid white; whether an
on-screen overlay appears on Windows or a console was not tested.

The other two press types fall back to the same assignment on a PC: with short
press on View and long/double left at their console-only defaults, a 1.6 s hold
produced one `BTN_SELECT` and a double press produced two, rather than anything
resembling the capture actions. Assigning only short press is therefore enough to
make the button usable on Linux.

What that means for testing the shift mode: the `0x9000B` events seen before the
remap came from the View button, not from the logo, so the driver appeared to
have a Guide source when it did not. With `disable_shift_mode=1` a sustained
press of View is a single clean press/release pair in separate `SYN_REPORT`
frames (4.03 s and 4.09 s holds measured), so there is no contact-bounce problem;
with shift mode enabled the driver caches and replays the event, which is what
made a held button look like rapid toggling.

## Wired (USB) mode

Over USB the same controller identifies as `045E:0B4E` with `bcdDevice` 6.10,
and it exposes a **single** interface, HID (`bInterfaceClass 0x03`) - no vendor
or GIP interface, unlike the `045E:0B12`/`0B13` pads which present one. xpadneo
claims it on `HID_USB_DEVICE(USB_VENDOR_ID_MICROSOFT, 0x0B4E)`; `hid-generic`
handles it otherwise.

Its report descriptor is 281 bytes, crc16 `0x4cba`. It has to be read over
usbfs with `GET_DESCRIPTOR(REPORT)` on interface 0: with no HID driver bound
there is no `hidraw` node to read it from. It differs from the 285-byte BLE
descriptor in exactly one place - the Consumer-page Mute usage
(`05 0C 09 B2 95 01 81 02`, 8 bytes at BLE offset 107) is replaced by a 4-byte
constant padding item (`95 01 81 03`). Substituting that one item in the BLE
dump reproduces the USB dump byte for byte (281 bytes), so everything else is
identical, including the shared Simulation Controls page that the trigger fixup
targets. The driver's own dump of the wired device agrees: `report descriptor:
length 281 crc16 0x4cba`, and the fixups for `Rx`, `Ry`, `Z` and `Rz` all
apply.

What the transport needs beyond the ID entry is the per-device button map: it is
keyed on the product ID, and USB keeps its own, so the same nine usages are
listed a second time under `0x0B4E`. Verified with a capture of the wired pad:
A reports `BTN_SOUTH` (304), X reports `BTN_X` (307), LB reports `BTN_TL` (310),
View reports `BTN_SELECT` (314) and Guide reports `BTN_MODE` (316) - the same
codes the Bluetooth path produces, so the shifted layout is independent of the
transport.

Two things behave differently on this transport and were fixed or measured
rather than assumed:

- **Output reports must not come from the stack.** `usbhid_output_report()`
  hands the caller's buffer straight to `usb_interrupt_msg()`, which maps it for
  DMA; the rumble call sites pass a local struct, so the wired path tripped
  `transfer buffer is on stack` in `usb_hcd_map_urb_for_dma()` and the transfer
  failed. Reaching this required a non-HOGP device, which only exists once USB
  is supported, so it is latent upstream. Fixed by bouncing the report through
  the heap in `xpadneo_device_output_report()`.
- **HOGP is not involved.** `uses_hogp` is derived from the bus
  (`hdev->bus == BUS_BLUETOOTH`), so a wired device takes the plain
  `hid_hw_output_report()` path; the driver logs `uses_hogp 0` on connect. No
  HOGP code runs for this transport.

Rumble: this controller has **no vibration motors** - "The XBOX Nano Controller
does not include haptics" (Xbox Support) - so no magnitude can ever be observed.
What can be measured is that the driver still streams reports and that the
throttling path applies to the wired transport: playing an effect at 10 ms
intervals, faster than the worker's per-report 20 ms settle, logs `throttled
rumble reprogramming` (`rumble.c`), which is the same behaviour as Bluetooth.

The Action button is silent over USB with the factory mapping: in a 45-second
capture of `/sys/kernel/debug/hid/0003:045E:0B4E.0009/events` (13,785 reports),
two presses of A gave four transitions on `Button.0001`, and three presses of the
Action button gave no transition on any of the fifteen buttons. Assigned to a
PC button it reports that button instead - see above.
