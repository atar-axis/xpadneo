## Working with Secure Boot

Secure Boot is a verification mechanism used when your computer loads your operating system. The boot  process of a
Linux distribution usually goes like this:
[UEFI](https://help.ubuntu.com/community/UEFI) -> [UEFI Shim loader](https://www.rodsbooks.com/efi-bootloaders/secureboot.html#shim) -> your distribution.
Now back to our two options: disabling Secure Boot or signing xpadneo with your own key.


### Disabling Secure Boot

In order for Secure Boot to be active, it must be enabled both at the UEFI level and at the shim level. Which means you
should be able to disable Secure Boot:

* either in your firmware options. You can usually access them by pressing F1, F2 or F12 justa after turning on the
  computer.
* or in the shim. Run `sudo mokutil --disable-validation`, and choose a password. Reboot your computer. You will be
  asked if you really want to disable Secure Boot. Press `Yes` and enter the password. You can now safely forget the
  password.

After choosing either of these two options, Secure Boot should be disabled. You may therefore try to connect your Xbox
One gamepad.


### Signing xpadneo with your own keys

If you do not wish to disable Secure Boot, you will need to get the Shim to trust xpadneo. The process goes this way:

* generate a cryptographic key pair: a private key and a public key
* sign the module with your private key
* get the shim to trust your public key

Xpadneo will need to be re-signed every time it is updated, and every time the kernel is updated. To simplify this
process, we provide scripts which will help you generate your keys and automatically sign xpadneo when needed.

```console
cd xpadneo
# Xpadneo will be signed each time it is installed, you should therefore uninstall it now if needed!
sudo ./uninstall.sh

# Copy a directory containing three helper scripts in the /root directory
sudo cp -r misc/module-signing /root

# Generate the keys and ask the shim to trust them.
# They will be saved as /root/module-signing/MOK.der (public key) and /root/module-signing/MOK.priv (private key)
# You will be asked to choose a password. This password will only be needed one time, just after the reboot.
sudo /root/module-signing/one-time-setup

# READ THIS BEFORE REBOOTING:
# Just after rebooting, you will be prompted to press a key to enter the Shim. Press any key, then select the
# `Enroll key` option. You will get a chance to review the key that you are about to trust. To accept the key, select
# Yes then enter the password you chose previously.
# /!\ WARNING: The keyboard layout used to type the password will be QWERTY, no matter what keyboard you use.

# Choose `Continue` to boot into your operating system.
# Your public key is now trusted by the shim.

# Ask DKMS to sign Xpadneo when needed.
echo "POST_BUILD=../../../../../../root/module-signing/dkms-sign-module" | sudo tee "/etc/dkms/hid_xpadneo"

# Go back to the xpadneo folder and install the module. It will be signed automatically.
cd xpadneo
sudo ./install.sh
```

Notes:

* if in the future you wish to remove your public key from the shim, run

   ```console
   sudo mokutil --delete "/root/module-signing/MOK.der"
   ```

  Reboot, and follow the procedure.
* using theses scripts, your private key will be kept unencrypted in the `/root` directory. This may pose a security
  risk. If you wish to password protect your private keys, use
  [these scripts, written by dop3j0e](https://gist.github.com/dop3j0e/2a9e2dddca982c4f679552fc1ebb18df). Be warned
  that you will be asked for your password every time the kernel gets an update.
