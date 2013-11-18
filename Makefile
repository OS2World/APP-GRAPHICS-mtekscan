# Makefile for mtekscan - Linux user level device driver for Microtek
# SCSI scanners. 
# Copyright (c) 1996, 1997 Jan Schoenepauck / Fast Forward Productions
# <schoenep@uni-wuppertal.de>

# --------------------------------------------------------------------------
# General settings -
# modify these according to your system setup.


# Make sure the correct path to the sg.h header file is set here,
# if you dont have a link there from /usr/include/scsi. This is
# probably /usr/src/linux/drivers/scsi/ if you have an older
# kernel, or /usr/src/linux/include/scsi/ if your kernel version is
# 1.3.98 or grater.
SG_INCLUDE_PATH = /usr/src/linux/include/scsi/
# SG_INCLUDE_PATH = /usr/src/linux/drivers/scsi/

# Edit these if you want the driver to be installed in a different
# location
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
MANPATH = $(PREFIX)/man
# The -o option specifies the owner of the binary after installation,
# the -g option sets the group.
INSTALL = install -o root -g bin

# Probably you don't have to change these...
CC = gcc
CPP = gcc -E
CCFLAGS = -g -Wall
DEFS =
LDFLAGS = -g -lm
RM = rm -f



################# DON'T CHANGE ANYTHING BELOW THIS LINE ####################

# $Id: Makefile 1.2 1997/09/14 02:51:02 parent Exp $

PROG = mtekscan.exe
OBJS = mtekscan.o mt_error.o options.o scsi.o gammatab.o
SRCS = mtekscan.c mt_error.c options.c scsi.c gammatab.c

all : $(PROG)

$(PROG) : $(OBJS) 
	$(CC) -o $(PROG) $(OBJS) $(LDFLAGS)

mtekscan.o : mtekscan.c config.h global.h mt_defs.h mt_error.h scsi.h \
 options.h gammatab.h
	$(CC) $(CCFLAGS) $(DEFS) -c mtekscan.c -o mtekscan.o

mt_error.o : mt_error.c mt_error.h mt_defs.h
	$(CC) $(CCFLAGS) $(DEFS) -c mt_error.c -o mt_error.o

options.o : options.c options.h global.h config.h mt_defs.h scsi.h gammatab.h
	$(CC) $(CCFLAGS) $(DEFS) -c options.c -o options.o

gammatab.o : gammatab.c global.h config.h options.h
	$(CC) $(CCFLAGS) $(DEFS) -c gammatab.c -o gammatab.o 

scsi.o : scsi.c scsi.h config.h global.h
	$(CC) $(CCFLAGS) -I $(SG_INCLUDE_PATH) -c scsi.c -o scsi.o 


install :
	$(INSTALL) -d -m 755 $(BINDIR)
	$(INSTALL) -d -m 755 $(MANPATH)/man1
	$(INSTALL) -s -m 755 $(PROG) $(BINDIR)
	$(INSTALL) -m 644 mtekscan.1 $(MANPATH)/man1

clean :
	$(RM) *~ $(OBJS)

dist-clean : clean
	$(RM) $(PROG)

