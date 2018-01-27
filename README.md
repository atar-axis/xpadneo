Linux Driver for Xbox One S Wireless Gamepad
---

This is a driver for the Xbox One S Wireless Gamepad which I created for a student research project at fortiss GmbH.

At the moment of development there was no driver available (and there still is none at January 2018) which supports force feedback (Rumble) one the wireless version (over Bluetooth).

This one does, the buildsystem also fixes a bug I found in L2CAP (which forced us to disable ertm before) and adds the new driver to the hid-core (this automatically loads the new driver when the controller is attached).
Alternatively, it offers a way to load the driver via UDev whenever you are not able to recompile hid-core (e.g. if it is not a module - on Raspbian).


Installation
---

Please install go-task first: https://github.com/go-task/task

- build driver(s) for your local system
  ```
  task
  alternatively: task default
  alternatively: task local
  ```

- build driver(s) for raspberry pi 3 (linux-raspberrypi-kernel_1.20171029-1)
  ```
  task raspi3
  ```
