/*
   mt_error.c
   Error reporting for MicroTek scanner driver mtekscan
   
   Copyright (c) 1996, 1997 Jan Schoenepauck / Fast Forward Productions
   <schoenep@uni-wuppertal.de>

   $Id: mt_error.c 1.1 1997/09/13 02:45:10 parent Exp $
*/

#include <stdio.h>

#include "mt_defs.h"
#include "mt_error.h"

extern char *myname;

/* Error messages */

char mt_errmsg[27][80] = {
  "No error",
  "CPU RAM failure", 
  "Scanner system RAM failure",
  "Image RAM failure",
  "DC offset error (black level calibration failure)",
  "Scanning lamp or image sensor circuit failure",
  "Scanning head motor or home position sensor failure",
  "Automatic document feeder paper ejection failure",
  "External power failure",
  "Transparency illuminator lamp failure",
  "Transparency illuminator motor or home position sensor failure",
  "Feeder paper feeding failure",
  "Filter motor or filter position sensor failure",
  "Illegal grain setting",
  "Illegal resolution setting",
  "Illegal scanning frame coordinate",
  "Illegal contrast setting",
  "Illegal paper length setting",
  "Illegal highlight/midtone/shadow adjustment setting",
  "Illegal exposure time (analog brightness adjustment) setting",
  "Color filter specified with automatic document feeder selected",
  "No paper in feeder",
  "Illegal look-up table (gamma adjustment) setting",
  "Illegal offset (digital brightness adjustment) setting",
  "Illegal bits-per-pixel setting",
  "Too many lines of scan data requested",
  "SCSI command error"
};

int last_error = 0;


/*------------------------------------------------------------------------*/
/* void error (int err_code)                                              */
/* Prints the error message for <err_code> and sets last_error to the     */
/* value of <err_code>.                                                   */
/*------------------------------------------------------------------------*/
void error (int err_code) {
  fprintf(stderr, "%s: %s.\n", myname, mt_errmsg[err_code]);
  last_error = err_code;
}


/*------------------------------------------------------------------------*/
/* int mt_check_sense (char *sense)                                       */
/* Analyze the sense data returned by the scanner after a REQUEST SENSE   */
/* command and stored in the array pointed to by <sense>. If an error is  */
/* reported in the sense data, print error message via error().           */
/* Args: <sense>  Pointer to sense data.                                  */
/* Return: GOOD if sense data is not available or contains no error       */
/*         message, CD_ERROR if a command/data error was reported,        */
/*         HW_ERROR if a hardware error was reported, OP_ERROR if an      */
/*         operation error was reported.                                  */
/*------------------------------------------------------------------------*/
int check_sense (unsigned char *sense) {
  switch(sense[0]) {
  case 0x00 :
    return(GOOD);
  case 0x81 :           /* COMMAND/DATA ERROR */
    if (sense[1] & 0x01) error(ERR_SCSICMD);
    if (sense[1] & 0x02) error(ERR_TOOMANY);
    return(CD_ERROR);
  case 0x82 :           /* SCANNER HARDWARE ERROR */
    if (sense[1] & 0x01) error(ERR_CPURAMFAIL);
    if (sense[1] & 0x02) error(ERR_SYSRAMFAIL);
    if (sense[1] & 0x04) error(ERR_IMGRAMFAIL);
    if (sense[1] & 0x10) error(ERR_CALIBRATE);
    if (sense[1] & 0x20) error(ERR_LAMPFAIL);
    if (sense[1] & 0x40) error(ERR_MOTORFAIL);
    if (sense[1] & 0x80) error(ERR_FEEDERFAIL);
    if (sense[2] & 0x01) error(ERR_POWERFAIL);
    if (sense[2] & 0x02) error(ERR_ILAMPFAIL);
    if (sense[2] & 0x04) error(ERR_IMOTORFAIL);
    if (sense[2] & 0x08) error(ERR_PAPERFAIL);
    if (sense[2] & 0x10) error(ERR_FILTERFAIL);
    return(HW_ERROR);                      
  case 0x83 :           /* OPERATION ERROR */
    if (sense[1] & 0x01) error(ERR_ILLGRAIN);
    if (sense[1] & 0x02) error(ERR_ILLRES);
    if (sense[1] & 0x04) error(ERR_ILLCOORD);
    if (sense[1] & 0x10) error(ERR_ILLCNTR);
    if (sense[1] & 0x20) error(ERR_ILLLENGTH);
    if (sense[1] & 0x40) error(ERR_ILLADJUST);
    if (sense[1] & 0x80) error(ERR_ILLEXPOSE);
    if (sense[2] & 0x01) error(ERR_ILLFILTER);
    if (sense[2] & 0x02) error(ERR_NOPAPER);
    if (sense[2] & 0x04) error(ERR_ILLTABLE);
    if (sense[2] & 0x08) error(ERR_ILLOFFSET);
    if (sense[2] & 0x10) error(ERR_ILLBPP);
    return(OP_ERROR);
  default :
    fprintf(stderr, "%s: sense = %02x %02x %02x %02x.\n", myname, 
            sense[0], sense[1], sense[2], sense[3]);
    return(-1);
  }
  return(GOOD);
}

