===================
cat_gif USER MANUAL
===================

An entirely over-engineered pi3b+ image for displaying GIFs of cats. But I 
suppose you can use any ol' gifs. The system will display them full-screen on
whatever display it can find using SDL3's renderer and will pull gifs
from a configured 'new' directory, cache them, and rotate them once at 7AM
every day: landing in the 'used' directory.

Getting Started
---------------

The easiest way to get started is to use pre-built images in the 'releases' on
GitHub.

You can flash the image by using 'dd' on Linux to an SD card:

---
sudo dd if=/path/to/sdcard.img of=/path/to/dev bs=4M conv=fsync status=progress
---

/path/to/your/dev is likely to be of the form: `/dev/sdb` or something similar:
use 'lsblk' find the name. Remember to plug in the SD card first!

Otherwise, a default configuration for buildroot is included in
buildroot/configs/cat_gif_rpi3_64_defconfig, specifically for an RPI3B+. This is
the only officially supported and tested platform for now.

Once the image is running the default users are:

  "root" with password: "changeme"
  "gifuser" with password: "changeme"

It's recommended you use the 'passwd' utility to change the password.

As an example setup, you can create a USB stick with structure:

---
./gifs
+---new/
+---used/
---

And put all your new gifs in new/. If new/ runs out of gifs, the app moves all
the gifs in used/ back to new/ and starts over.

USBs will mount at: '/media/usbX' where X is 0 thru 7.

You can edit the configuration to match the directories you put gifs in with:

---
nano /etc/cat-gif.conf
---

An example configuration for this setup woudl be:

---
/media/usb0/gifs/new
/media/usb0/gifs/used
---

While the app is running (keyboard attached to the Pi):

  Esc          quit the app
  hold 'r' 3s  force a rotation to the next gif

Alternative build methods
-------------------------

The application can be built to run on your host system with:

---
git submodule update --init
cmake -S . -B ./build
cd build && make && cd ..
---

Overview
--------

This application works by using the SDL3 library to display a GIF which rotates
at 7AM local time every day. Keep in mind PIs have no built-in RTC so it needs
network for true time, or you need to set it manually.

Defaults:

- gifs/new............The directory where new gifs will be pulled from
- gifs/used...........The directory where used gifs will go
- fallback.gif........The fallback gif to use on errors with new/ and used/ dirs
- /etc/cat-gif.conf...Default where the program will look for config file

The directories are configured via a line-parsed config file. This config file
can be specified at the command line.

Config file format:
```
# Ignored comment
/path/to/new/dir
/path/to/used/dir
```

It's recommended to edit this with nano, vi, or vim which are all installed on
the pi. Changes are picked up periodically. Note it's intended you store these
on a USB stick that usbmount will mount automagically. See the usbmount config
file at `buildroot/board/overlay/etc/usbmount/usbmount.conf` for the supported
file systems and mount options.

However, since it's just a file path: 'scp', 'rsync' or any other tool is
perfectly useful: it's just a directory with files!

This application should come with a config that is ready-made for pi3b+. Config
options written below are for reference only, it's intended the defconfig
provided in buildroot/configs/cat_gif_rpi3_64_defconfig will supply all Buildroot config you need.

Working directory is............................................/var/lib/cat-gif

Setting time on A PI manually
-----------------------------

You can set the time manually on the pi by using this command:

date -s "2026-10-23 07:00:00"

Example Usage
-------------

cat_gif /path/to/your/config

# Uses default config file at /etc/cat-gif.conf
cat_gif

Boring Technical Information
----------------------------

- Can run without a display server. Should be capable of hijacking the Pi's
  Kernel renderer, which is handled automagically by SDL3. Code does not specify
  a specific renderer to use.
- When buliding with CMake for the host & wanting to test. Make sure to install
  with: `cmake -B build -DCMAKE_INSTALL_PREFIX=/usr` to mirror what buildroot
  will do. Otherwise the paths set in default configuration will be incorrect.

System Information
------------------

- Default root password is: "changeme"
- Default "user/pass" that runs the app is: "gifuser/changeme"
- Make sure to change these as part of your sysadmin or change them in the
  users_table.txt file or the root password using `make menuconfig` in
  buildroot. (BR2_TARGET_GENERIC_ROOT_PASSWD)