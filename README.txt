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

Buildroot Comments
------------------

- While not 'required' for building, it would be smart to enable 'udisks' if you
  intend on using a USB stick to store files.
- It would be smart to enable an SSH server as well.

Setting time on A PI manually
-----------------------------

Example Usage
-------------

Boring Technical Information
----------------------------

- Can run without a display server. Should be capable of hijacking the Pi's
  Kernel renderer, which is handled automagically by SDL3. Code does not specify
  a specific renderer to use.