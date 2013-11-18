
This is a port of the Linux program "mtekscan" v0.2 by Jan Schoenepauck.
Please read the "README" that he provided.  The essential item that makes this
port possible is the "ASPI Router" driver by Daniel Dorau.  I've included the
distribution ("aspir101.zip"), which you'll need to install.  (Note that the 
ASPI driver is not under the GNU License.)  You'll also need "emxrt" installed.

I've also included "tkscan" by Hang-Bae Kim, a tcl/tk front end that supports
"mtekscan".  It will need some adapting to work with this version.

The binary "mteke3.exe" supports the E3 scanner.  The binary "mteke6.exe",
ported and tested by Behram Parekh, supports the E6.  The file "config.h.E6" 
has the defines that he used to compile it.

The program is used in exactly the same way as the Linux version, except for
two differences:

1.  You MUST give your scanner's SCSI ID # (such as 6) as the first item on
	the command line.

2.  Always specify an output file using the "-o filename.ppm" option.  Other-
	wise, mtekscan will dump loads of junk to your screen.

Example (for a scanner with ID=6):
   mtekscan -f 0 0 4.6 2.4 -r 150 -g -o image.ppm


The source is set up to generate an E3 version (config.h == confige3.h).  To
create an E6 version, copy confige6.h to config.h, run "make", and then rename
the result (mtekscan.exe) to mteke6.exe.

DISCLAIMER:

This is free, only slightly tested software.  Read the "README" from the
original author.  Be sure to specify the correct SCSI ID of your scanner.  I
don't know what will happen if you specify the wrong ID; you might end up
writing garbage to a disk drive.

-- Jeff Freedman (jsf@hevanet.com)

HISTORY:

mtek004:
	Added E6 executable from Behram Parekh (bparekh@ibm.net)

mtek003:
	Fixed buffer-size problem which caused errors on larger scans.

mtek002:
	First release.
