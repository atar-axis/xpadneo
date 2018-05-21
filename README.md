This is the DKMS version of my hid-xpadneo driver. Take a look in the master branch for further information.

## Prerequisites
To install it, make sure you have installed
* dkms
* linux-headers

## Installation
Download the `hid-xpadneo-<version>` folder into your local _/usr/src/_ directory.
Run the following commands

1. sudo dkms add hid-xpadneo -v <version>
2. sudo dkms install hid-xpadneo -v <version>
