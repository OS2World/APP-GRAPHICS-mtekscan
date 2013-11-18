/*
   mt_defs.h
   Some handy #defines for mtekscan.
   
   Copyright (c) 1996, 1997 Jan Schoenepauck / Fast Forward Productions
   <schoenep@uni-wuppertal.de>

   $Id: mt_defs.h 1.1 1997/09/13 02:45:10 parent Exp $
*/

#ifndef _MT_DEFS_H
#define _MT_DEFS_H

/* Scan types */
#define LINEART    0
#define HALFTONE   1
#define GRAYSCALE  2
#define COLOR      4

/* Size units */
#define INCH       0
#define MM         1
#define CM         2
#define EIGHTHINCH 3
#define POINT      4
#define PIXEL      5

/* Filters / planes */
#define CLEAR   ('C')
#define RED     ('R')
#define GREEN   ('G')
#define BLUE    ('B')

/* Document type/location definitions (for mt_TypLoc) */
#define DOC_ON_FLATBED 0x00
#define DOC_IN_FEEDER  0x01
#define TRANSPARENCY   0x10

/* Scanner errors */
#define GOOD             0

#define CD_ERROR        -1
#define HW_ERROR        -2
#define OP_ERROR        -3

#define ERR_CPURAMFAIL   1
#define ERR_SYSRAMFAIL   2
#define ERR_IMGRAMFAIL   3
#define ERR_CALIBRATE    4
#define ERR_LAMPFAIL     5
#define ERR_MOTORFAIL    6
#define ERR_FEEDERFAIL   7
#define ERR_POWERFAIL    8
#define ERR_ILAMPFAIL    9
#define ERR_IMOTORFAIL  10
#define ERR_PAPERFAIL   11
#define ERR_FILTERFAIL  12
#define ERR_ILLGRAIN    13
#define ERR_ILLRES      14
#define ERR_ILLCOORD    15
#define ERR_ILLCNTR     16
#define ERR_ILLLENGTH   17
#define ERR_ILLADJUST   18
#define ERR_ILLEXPOSE   19
#define ERR_ILLFILTER   20
#define ERR_NOPAPER     21
#define ERR_ILLTABLE    22
#define ERR_ILLOFFSET   23
#define ERR_ILLBPP      24
#define ERR_TOOMANY     25
#define ERR_SCSICMD     26

#endif
