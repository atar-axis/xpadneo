---
name: Bug report
about: Report a bug

---

## Version of xpadneo
<!-- Please let us know the version number of xpadneo, either the one shown
     during install (preferred) or the one you downloaded (releases). -->

## Controller Model
<!-- Please identify your controller model. -->

- [ ] Xbox One S controller
- [ ] Xbox Elite 2 controller
- [ ] Xbox Series X|S controller
- [ ] Other:

### Connection mode

- [ ] Bluetooth connection
- [ ] USB cable (not yet supported)
- [ ] Xbox Dongle connection (not yet supported)

## Installed Software
<!-- Some software may interfere with functionality or detection, or may
     introduce unwanted side-effects. Please check which software you're
     running. -->

- [ ] Anti-Micro (may affect button mappings)
- [ ] Steam Input (enabled by default via Steam Desktop client)
- [ ] Steam Link (usually via Raspberry Pi or other micro computers)
- [ ] netstick (shares input devices via network similar to Steam Link)
- [ ] xboxdrv (user-space gamepad driver)
- [ ] xow (alternative driver using the Xbox dongle)

## Severity / Impact
<!-- Give us some impression of the importance of this bug report. You can
     easily check these after submitting the bug report. -->

- [ ] I've read the docs and the bug reporting instructions
- [ ] I've applied the latest firmware update to the controller
- [ ] It does not work at all
- [ ] It used to work in a previous version
- [ ] It mostly works but sometimes it doesn't
- [ ] I found a work-around
- [ ] I probably didn't figure it all out but it's too early to give up
- [ ] I don't know how to ... <!-- describe below -->
- [ ] It's too complicated
- [ ] Fantastic work but ... <!-- describe below -->
- [ ] I can code and I want to help

## Describe the Bug
<!-- A clear and concise description of what the bug is. -->


## Steps to Reproduce
<!-- Steps to reproduce the behavior: -->


## Expected Behavior
<!-- A clear and concise description of what you expected to happen. -->


## Screenshots / GIFs / Videos
<!-- If applicable, add screenshots or screen recordings to help explain
     your problem. -->


## System Information
<!-- Please add at least the following outputs: -->

<!-- Paste the output below the line prepended with # -->
```console
# uname -a

```

<!-- Paste the output below the line prepended with # -->
```console
# xxd -c20 -g1 /sys/module/hid_xpadneo/drivers/hid:xpadneo/0005:045E:*/report_descriptor | tee >(cksum)

```

## Controller and Bluetooth Information
<!-- Also follow these steps to create addition information
     about your Bluetooth dongle and connection: -->

<!-- First, disconnect the controller. -->

<!-- Run `sudo btmon | tee xpadneo-btmon.txt` and connect the controller. -->

<!-- Run `dmesg | egrep -i 'hid|input|xpadneo' | tee xpadneo-dmesg.txt`. -->

<!-- Run `lsusb` and pick the device number of your dongle. -->

<!-- Run `lsusb -v -s## | tee xpadneo-lsusb.txt` where `##` is the device
     number picked in the previous step -->

<!-- Attach the resulting files, do not bundle the files into a single
     archive. If some files are too big, gzip them individually. Drag
     and drop the files below. -->


## Additional Context
<!-- Add any other context about the problem here. -->
