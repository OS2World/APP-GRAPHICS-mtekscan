



                             mtekscan v0.2


                 Linux Driver for MicroTek SCSI scanners








  Copyright (c) 1996, 1997 by Jan Schoenepauck / Fast Forward Productions








____________________________________________________________________________
Introduction

This driver allows the use of MicroTek ScanMaker SCSI scanners with the
Linux operating system. It is a simple command line program which scans an
image and writes it to stdout (or a specified file) in PBM / PGM / PPM
format. It does not have a flashy graphical interface like WINDOWS TWAIN
drivers or Macintosh scanning plugins (I am working on that, though), but it
does the job. You can use the pbm package to convert the output, e.g.

   mtekscan -f 0 0 2 2 -r 72 -c | ppmquant 256 | ppmtogif > scan.gif

to create a GIF image file, or use a command like

   mtekscan -f 1 0 4.6 2.4 -r 150 -g | xv -

to view the scanned image directly with xv. For an explanation of the
command line options/switches, see the man page (or call mtekscan with 
the -h switch).



----------------------------------------------------------------------------
                               * WARNING *
----------------------------------------------------------------------------


                           THIS IS BETA CODE.

This code comes with ABSOLUTELY NO WARRANTY. I do not take responsibility
if your computer or scanner explodes, or if it crashes your system -- it 
did crash mine several times during the development phase. It runs stable 
now ON *MY* SYSTEM. If you have a different setup, e.g. a different SCSI 
host adapter, a different kernel / low level SCSI driver version, or a 
different scanner,  I cannot say what will happen.
The generic SCSI driver in the kernel which is used to communicate with 
the SCSI hardware is not foolproof. It can cause kernel panics or silent
lockups if it is programmed the wrong way. USE THIS CODE AT YOUR OWN RISK.
To reduce the risk of data loss, it might be a good idea to back up
your hard disk, or at least do a 'sync' before testing the program so that 
the hard disk cache is flushed to disk.

The code was developed and tested on a Pentium 100 running Linux 1.2.13 and
2.0.26 with a ScanMaker E6 connected using the Adaptec AVA-1502E SCSI adapter
that came with the scanner. Although not mentioned in the SCSI-HOWTO, the
AVA-1502E works with the AHA152x low level kernel driver (though I'm not
sure if it is 100% compatible - if it is not, then that might be an 
explanation for some little oddities in the behaviour I experienced).
I have the scanner connected as the only device in the SCSI chain and no 
other SCSI adapters installed. Some people say they get problems with
the generic SCSI driver when there is more than one device in the SCSI 
chain, so if you encounter any problems with SCSI bus lockups or kernel
panics or the like, try the driver with the scanner connected as the
only device if possible.


____________________________________________________________________________
Copying Policy

The code is Copyright (c) 1996, 1997 by Jan Schoenepauck. Permission is
granted  to freely use, copy, and distribute it, as long as no fee is 
charged. You are also free to modify it or use parts of it for other
programs, as long as you name me as the original author. Please notify me if
you make any modifications which might be worth adding in a future release.
If you enclose this code in a commercial distribution, please let me know
and please send me a sample of it.


____________________________________________________________________________
Installation

This is not a kernel level driver. The code works in user space, and
uses the generic SCSI driver in the kernel to communicate with the SCSI
hardware (i.e. the scanner). So, in order to run it, you have to compile
the generic SCSI driver support into your kernel (obviously, you also
need a low-level SCSI driver for your SCSI host adapter, too). You
need to know the device the scanner is mapped to (usually, it will be
one of /dev/sga, /dev/sgb ... /dev/sgh, but on some machines they might
be called /dev/sg0, /dev/sg1 ...). The devices are dynamically mapped to
the SCSI devices at boot time, starting with the lowest used ID. If your
scanner is the only device in the SCSI chain, it will always be /dev/sga,
no matter which ID it is set to. It is a good idea to create a symlink to 
the scanner device (say, make /dev/scanner a link to /dev/sga, in the above 
example), so you won't have to recompile mtekscan if the device mapping 
changes (which will happen if you connect an additional SCSI device with 
a lower ID than the scanner), but can simply change the link instead.
Take a look at the section "SCSI setup" below for some additional
information.

Before compiling the code, take a look at the config.h file. Set
DEFAULT_DEVICE to the correct generic SCSI device (or to the symlink 
if you created one). If you have a ScanMaker E6, you will probably not have 
to change the other settings (though you might want to change the default 
scanning options). But if you want to try the driver with a different 
MicroTek scanner, you might have to change the values for the maximum 
resolution and probably set FIX_FRAMESIZE to 0 (if your scanned images
have the wrong size or resolution, experiment with these two settings).
See section "Using mtekscan with other MicroTek scanners" below.

When you're finished with the config.h file, edit the settings at the top 
of the Makefile (if neccessary). Make sure the path to the generic 
driver's include file sg.h is correct -- it will probably be 
/usr/src/linux/drivers/scsi/ if you have an older kernel, or 
/usr/src/linux/include/scsi/ as of kernel v1.3.98 (assuming that
/usr/src/linux is your kernel source path). For newer kernel versions,
there should exist a link from /usr/include/scsi; if not you should
create one -- this way, you won't have to bother setting the path in the
Makefile. Refer to /usr/src/linux/README if you don't know how to do this.

Finally, run 'make'. If you want to install mtekscan properly for your
machine, run 'make install' as root -- this will copy the executable and the
manpage to the specified locations. Now make sure the scanner device
is read-/writeable, and that the scanner was connected and switched 
on at boot time (it must have been detected during the booting phase). 
Call 'mtekscan -I' to see if the scanner responds. If everything went OK
up to this point, you should be able to scan now. Go ahead, read the
man page to get an idea of the many options and scanner settings mtekscan
offers.


____________________________________________________________________________
Using mtekscan with other MicroTek scanners

This code was written for and tested with a ScanMaker E6, but the newer
MicroTek scanners all have basically the same SCSI programming interface,
so it should work with other models, too.

So far, mtekscan has reportedly been used successfully with a ScanMaker II,
ScanMaker IIXE, ScanMaker IIHR, ScanMaker E3, ScanMaker III, and a 
ScanMaker 35t+ (and the E6, of course).
It also works with the Adara ImageStar I (Adara seems to be related to
MicroTek somw way), the Genius ColorPage-SP2 (which seems to be
compatible with the ScanMaker E3) and the Primax Deskscan Color (which
is basically the same machine as a ScanMaker II).

Of course, only SCSI scanners are supported. It will definitely not work
if the scanner is connected via a different adapter card.

Transparency scanning should work, but I have no transparency illuminator,
so I can't test it. Support for an automatic document feeder is still not
available (yet). I don't have one, either... if you need support for an ADF,
write me. But please see the section "Reporting Bugs" below as to which
information to include before writing me.


____________________________________________________________________________
SCSI setup

It is recommended to set the value of SCSI_BIG_BUF in sg.h to a value
high enough to contain at least one scan line of data (depending on your
maximum scanning resolution) before compiling the generic SCSI kernel driver.
It was set to 32768 in my kernel source, which seems to be O.K. Lower values
result in the scanner having to send it´s data in very small chunks, which
slows the scanning considerably. The file sg.h is probably located in
/usr/src/linux/drivers/scsi if you have an older kernel, or in
/usr/src/linux/include/scsi for newer kernel versions (1.3.98 and up).
There should exist a link from /usr/include/scsi to the corresponding
directory in the kernel source.

It might be a good idea to compile the low-level SCSI kernel driver for your
host adapter and the generic SCSI driver as a module, so you don´t have
to turn your scanner on at boot-time when using an older kernel (I always
forget that...). You just have to unload the driver module (if it is loaded),
turn on your scanner, and load the driver module. Otherwise, you´d have to
turn the scanner on and reboot. Note that newer kernels support another way
of dynamically adding and removing SCSI devices by writing to 
/proc/scsi/scsi, which also allows to change the device mapping. See the
documentation in the kernel source and the SCSI- and SCSI-Programming-HOWTO
for additional information.

Thomas Kuerten <kuerten@informatik.tu-muenchen.de> pointed out that if the
scanner is connected to an Adaptec AHA-2940 card, the Max Transmission Speed
should be set to 5 MB/s and the synchronous negotiation should be disabled
for the SCSI ID of the scanner (in the menu that shows up when you hit
Alt-A at boot time). Maybe other adapters need this to be set, too (if
possible).


____________________________________________________________________________
Reporting bugs

Although mtekscan runs stable now on my system (with a ScanMaker E6), there
might be still some bugs, especially in those parts of the code that I cannot
test myself, i.e. the support for other MicroTek scanners than the E6,
three-pass and transparency scanning. There are some known bugs that I am
currently unable to fix described in the BUGS section of the manpage. Also
refer to the TODO file to see if I am already aware of the bugs/ideas/
improvements that might turn up before mailing me. And finally, I will
put up a WWW page where the latest changes and information will be
made available first at the Fast Forward web site,
http://fb4-1112.uni-muenster.de/ffwd/

If you find any bugs, send me an email. Tell me what scanner model you
use, it's resolution, and which SCSI adapter you use. You should also
include a copy of the scanner information that can be printed by calling  
mtekscan with the -I switch. And if you can scan something, but it looks 
just weird, send me a copy of that too (not too large images, of course - 
a 50x50 pixel image should be enough) together with the commandline switches 
that you used -- it might be helpful when analyzing what went wrong.
I will try to put in bugfixes and improvements, but I CAN'T PROMISE 
ANYTHING. I do not guarantee that the driver works for you now or will
do so in a future release.

Of course, I also appreciate any ideas and suggestions. And even if you
are just happy using this code, I'd be pleased to hear from you.


____________________________________________________________________________
Contacting the Author


Snail mail:  Jan Schoenepauck
             Magdalenenstr. 8
             48143 Muenster
             Germany

EMail:       schoenep@uni-wuppertal.de

Check the Fast Forward WWW site at http://fb4-1112.uni-muenster.de/ffwd/
for the latest information and updates for our projects.


____________________________________________________________________________
Sources

The code was written with the help of the following sources:


    The SCSI-Programming-HOWTO by Heiko Eissfeldt


    The MicroTek SCSI Image Scanner Programmer's Reference
    (thanks to Warren Early at MicroTek for the help)


    Parts of the code were ripped from the muscan driver code 
    by Torsten Eichner


____________________________________________________________________________
Thanks...

...to Warwick Allison, Wolfgang Wander, Itai Nahshon, and everybody
else who tested the code and sent me reports, comments, bugfixes, patches,
or hardware information. All of you have been a great help.
