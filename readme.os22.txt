
This is a port of the Linux program "mtekscan" v0.2 by Jan Schoenepauck.
Please read the "README" that he provided.  The essential item that makes this
port possible is the "ASPI Router" driver by Daniel Dorau.  I've included the
distribution ("aspir101.zip"), which you'll need to install.  (Note that the 
ASPI driver is not under the GNU License.)

I've also included "tkscan" by Hang-Bae Kim, a tcl/tk front end that supports
"mtekscan".

The binary "mtekscan.exe" supports the E3 scanner.  If you look at the author's
documentation, you'll see that it's easy to modify the "config.h" file to get
E6 support.  To build a new version, you'll need a recent EMX (gcc for OS/2)
compiler and a copy of GNU make.  Just type "make" to build it.  If someone
tries this and tests it, I could include that binary in here as well.

The program is used in exactly the same way as the Linux version, except for
two differences:

1.  You MUST give your scanner's SCSI ID # (such as 6) as the first item on
	the command line.

2.  Always specify an output file using the "-o filename.ppm" option.  Other-
	wise, mtekscan will dump loads of junk to your screen.

Example (for a scanner with ID=6):
   mtekscan 6 -f 0 0 4.6 2.4 -r 150 -g -o image.ppm


DISCLAIMER:

This is free, only slightly tested software.  Read the "README" from the
original author.  Be sure to specify the correct SCSI ID of your scanner.  I
don't know what will happen if you specify the wrong ID; you might end up
writing garbage to a disk drive.

-- Jeff Freedman (jsf@hevanet.com)
