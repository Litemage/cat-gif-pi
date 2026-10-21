===================
cat_gif USER MANUAL
===================

This application works by using the SDL3 library to display a GIF which rotates
at 7AM local time every day. Keep in mind PIs have no built-in RTC so it needs
network for true time, or you need to set it manually.

Default directories:

- gifs/new.......The directory where new gifs will be pulled from
- gifs/used......The directory where used gifs will go
- fallback.gif...The fallback gif to use on errors with new/ and used/ dirs

You may pass your own directories as arguments to the CLI (see "Example Usage").
It is intended that 'gifs' is stored on a USB stick that auto-mounts: this
makes it easy to load gifs on/off.

However, since it's just a file path: 'scp', 'rsync' or any other tool is
perfectly useful: it's just a directory with files!

This application should come with a config that is ready-made for pi3b+. Config
options written below are for reference only, it's intended the defconfig
provided in <TODO> will supply all Buildroot config you need.

Working directory is............................................/var/lib/cat_gif

Quick Start
-----------

TODO: Copy/pasteable commands for zero to `make` in buildroot.

Buildroot Comments
------------------

- While not 'required' for building, it would be smart to enable 'udisks' if you
  intend on using a USB stick to store files.
- It would be smart to enable an SSH server as well.
- On a pi3b, it is recommended to enable: BR2_PACKAGE_MESA3D_GALLIUM_DRIVER_VC4,
  BR2_PACKAGE_MESA3D_OPENGL_ES, BR2_PACKAGE_SDL3_OPENGLES for a SDL render.
- You'll need to override the config.txt by setting:
  BR2_PACKAGE_RPI_FIRMWARE_CONFIG_FILE to: $(BR2_EXTERNAL_CATGIF_PATH)/board/config.txt

Setting time on A PI manually
-----------------------------

Example Usage
-------------

Boring Technical Information
----------------------------

- Can run without a display server. Should be capable of hijacking the Pi's
  Kernel renderer, which is handled automagically by SDL3. Code does not specify
  a specific renderer to use.
- When buliding with CMake for the host & wanting to test. Make sure to install
  with: `cmake -B build -DCMAKE_INSTALL_PREFIX=/usr` to mirror what buildroot
  will do. Otherwise the paths set in default configuration will be incorrect.
- To get video dependencies auto loaded (VC4), make sure to turn on `eudev`

System Information
------------------

- Default root password is: "changeme"
- Default "user/pass" that runs the app is: "gifuser/changeme"
- Make sure to change these as part of your sysadmin or change them in the
  users_table.txt file or the root password using `make menuconfig` in
  buildroot. (BR2_TARGET_GENERIC_ROOT_PASSWD)