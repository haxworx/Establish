# Establish - The Installer

This program will help you install the latest operating system
images to your local device.

You can modify and create your own list from your server or use
the stock list.

I will try to keep that up-to-date.

The program will download, check and write in one action.

Once the write to the disk is complete the SHA256 checksum will
be displayed for your convenience.

# Ayup Selector

You can use ayup to download files over HTTP/s. Selector will
pull the list of image URLs for ease-of-use.

These command-line tools (used together) do the same as Establish.
Neither uses ecore_con. Maybe they should???

FIXME (Linux):

* Need to discard devices that are crucial to the operating system.
  Basically don't mess up! It *should* do what you ask it to do
  for now!

FIXME (FreeBSD):

* Why does it need to use the fallback code???

TODO:

* Maybe a good idea to move the HTTP/SSL stuff out of the GUI
  binary and call that??? 

