# Establish - The Installer

Establish will download a list of operating system images
from which you can choose, download and the install directly
onto a disk for booting.

Establish allows you to write to a local file but also the
program detects for local disks suitable for writing to.
This auto-detection works quite well on FreeBSD, DragonFly
BSD, OpenBSD and Linux.

Once the write to the disk is complete the SHA256 checksum will
be displayed for your convenience.

On some operating systems disk buffering is required and thus 
implemented.

Included is a small program that will search for distributions,
and find the latest image for various architectures. This program
requires Golang compiler/interpreter.

