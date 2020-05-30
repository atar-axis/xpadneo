---
name: Bug report
about: Report a bug

---

## Describe the bug
<!-- A clear and concise description of what the bug is. -->

## Steps to Reproduce
<!-- Steps to reproduce the behavior: -->

## Expected behavior
<!-- A clear and concise description of what you expected to happen. -->

## Screenshots/Gifs
<!-- If applicable, add screenshots or animated gifs to help explain your problem. -->

## System information
<!-- Please add at least the following outputs: -->

<!-- Paste the output below the line prepended with # -->
```console
# uname -a

```

<!-- Paste the output below the line prepended with # -->
```console
# xxd -c20 -g1 /sys/module/hid_xpadneo/drivers/hid:xpadneo/0005:045E:*/report_descriptor | tee >(cksum)

```

## Controller and Bluetooth information
<!-- Also follow these steps to create addition information
     about your Bluetooth dongle and connection: -->

<!-- First, disconnect the controller. -->

<!-- Run `sudo btmon | tee xpadneo-btmon.txt` and connect the controller. -->

<!-- Run `dmesg | egrep -i 'hid|input|xpadneo' | tee xpadneo-dmesg.txt`. -->

<!-- Run `lsusb` and pick the device number of your dongle. -->

<!-- Run `lsusb -v -s## | tee xpadneo-lsusb.txt` where `##` is the device
     number picked in the previous step -->

<!-- Attach the resulting files, do not bundle the files into a single
	 archive. If some files are too bug, gzip them individually. -->

## Additional context
<!-- Add any other context about the problem here. -->
