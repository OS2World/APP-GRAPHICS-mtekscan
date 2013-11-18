/*
   config.h
   Global #defines for mtekscan.

   Copyright (c) 1996, 1997 Jan Schoenepauck / Fast Forward Productions
   <schoenep@uni-wuppertal.de>

   $Id: config.h 1.1 1997/09/13 02:45:10 parent Exp $
*/

/* Adjust the following settings according to your scanner and system     */
/* configuration. The following list contains some settings for specific  */
/* scanner models as they have been reported to me. If your scanner model */
/* is not listed here, or you get unexpected scanning results, you might  */
/* try to experiment with these. Please read the description supplied for */
/* each setting carefully.                                                */
/*                                                                        */
/* ScanMaker E6:  No changes needed (probably).                           */
/*                                                                        */
/* ScanMaker E3:  FIX_FRAMESIZE must be set to 0; VENDOR_STRING must be   */
/*                "        " (8 spaces) or empty, MAX_BASE_RESOLUTION is  */
/*                300.                                                    */
/*                                                                        */
/* ScanMaker II:  FIX_FRAMESIZE must be set to 0; MAX_WAIT_RETURN_RETRY   */
/*                and/or WAIT_RETURN_SLEEP_TIME values must be raised if  */
/*                you get 'Error executing STOP SCAN command (timeout)'   */
/*                messages. MAX_BASE_RESOLUTION must be set to 300,       */
/*                MAX_EXPANDED_RESOLUTION to 600.                         */
/*                You could also try to set USE_FCP to 1 and mail me if   */
/*                it works.                                               */
/*                                                                        */
/* ScanMaker 35t: FIX_FRAMESIZE must be set to 0; MAX_BASE_RESOLUTION is  */
/*                1950, MAX_EXPANDED_RESOLUTION 3900.                     */
/*                                                                        */
/* Non-MicroTek scanners: At least change VENDOR_STRING to reflect the ID */
/*                reported by your scanner (or leave empty).              */


/* Scanner device name.                                                   */
/* The default device name to be used, e.g. /dev/sga (or use /dev/scanner */
/* and make it a link pointing to the device your scanner is mapped to at */
/* boot time).                                                            */
#define DEFAULT_DEVICE "/dev/scanner"

/* Maximum resolution(s) supported by scanner.                            */
/* MAX_EXPANDED_RESOLUTION is ignored if the scanner does not support an  */
/* expanded resolution range and IGNORE_XRES is not set (see below).      */
/* On the other hand, if the scanner does support an expanded resolution  */
/* but the value is set to zero here, it will not be used. As a rule of   */
/* thumb, if your scanner reports that it supports expanded resolution    */
/* (check the output of 'mtekscan -I') and it has a physical resolution   */
/* of, say, 300 dpi horizontally by 600 dpi vertically, you'll probably   */
/* have to set MAX_BASE_RESOLUTION to the horizontal resolution value     */
/* (300) and MAX_EXPANDED_RESOLUTION to the vertical resolution value     */
/* (600).                                                                 */
/* These values have to be set here, as there is no way to request this   */
/* information from the scanner.                                          */
/*	For the E3:	*/
#define MAX_BASE_RESOLUTION 300
#define MAX_EXPANDED_RESOLUTION 600
/*	For the E6:	*/
/* #define MAX_BASE_RESOLUTION 600	*/
/* #define MAX_EXPANDED_RESOLUTION 1200	*/

/* Expanded resolution support check override flag.                       */
/* The ScanMaker E6 supports an expanded resolution even though the       */
/* scanner reports that it does not. If this option is set to 1, the      */
/* scanner´s opinion is ignored. It is probably safe to leave this un-    */
/* changed even if your scanner really does not have an expanded          */
/* resolution range.                                                      */
#define IGNORE_XRES_FLAG 1

/* E6 frame size correction flag.                                         */
/* The ScanMaker E6 behaves a little strange: It always scans an area     */
/* twice as large as the one specified (e.g. 2x2" if 1x1" was specified)  */
/* Maybe I've made a mistake somewhere, but until I find out, this        */
/* #define will fix it.                                                   */
/* So far, the E6 is the only scanner known to need this fix.             */
/*	For the E3:	*/
#define FIX_FRAMESIZE 0
/*	For the E6:	*/
/* #define FIX_FRAMESIZE 0	*/

/* SCSI command handling fix flag.                                        */
/* Setting this to 1 enables a different behaviour in the communication   */
/* with the generic SCSI driver. I need this one set for the AVA-1502E    */
/* host adaptor card, but if you have a different card and experience     */
/* SCSI bus lock-ups, kernel panics, system crashes or the like, try      */
/* setting it to 0. Apparently this is not neccessary anymore for newer   */
/* kernel versions.                                                       */
#define SCSI_REPLYLEN_FIX 0

/* Scanner vendor ID.                                                     */
/* The string against which the VendorID reported by the scanner is       */
/* checked, for security. According to the docs, all MicroTek scanners    */
/* should reply with MICROTEK, but at least the E3 doesn't (reports       */
/* 8 spaces). And of course all non-MicroTek scanners will most probably  */
/* need a different setting here. If leave this empty (or comment it out) */
/* the check is not made (the device type code is still checked, though). */
/*	For the E3:	*/
#define VENDOR_STRING "        "
/*	For the E6:	*/
/* #define VENDOR_STRING "MICROTEK" */

/* Busy retry/timeout.                                                    */
/* Maximum number of retries when checking if scanner is busy, and time   */
/* to sleep between retries (in seconds). There is probably no need to    */
/* change these.                                                          */
#define MAX_BUSY_RETRY  5
#define BUSY_SLEEP_TIME 3

/* Head positioning delay control.                                        */
/* Maximum number of retries when waiting for the scan head to return to  */
/* starting position between scanning passes, and time to sleep between   */
/* them (in seconds). Some scanner models don't accept a STOP SCAN        */
/* command until the scan head has returned to starting position (The     */
/* ScanMaker II is known to be among these -- I guess it's a feature of   */
/* 3-pass scanners only). Raise these values if mtekscan aborts with a    */
/* 'Error executing STOP SCAN command (timeout)' message while the head   */
/* is still on its way back to home position.                             */
#define MAX_WAIT_RETURN_RETRY  10
#define WAIT_RETURN_SLEEP_TIME 2

/* Fast Color Prescan flag - EXPERIMENTAL.                                */
/* Some 3-pass models (e.g. the ScanMaker II, according to it´s inquiry   */
/* output) support a ´Fast Color Prescan´, i.e. scanning the green color  */
/* plane while the scanning head returns to the starting position.        */
/* Setting this value to 1 will enable this mode when the -p (prescan)    */
/* option is specified on the command line, but it is still untested --   */
/* I have no idea if it will work and no means to test it.                */
#define USE_FCP 0

/* Output file format selection.                                          */
/* If this is set to 1, images are written in pbm/pgm/ppm raw format,     */
/* resulting in much smaller files. It is probably not necessary to set   */
/* it to 0, except for debugging perhaps.                                 */
#define WRITE_RAW 1

/* Software-based negative scanning.                                      */
/* Normally, specifying the -n switch selects the scanner´s negative      */
/* scanning option. If the scanner does not support negative scanning,    */
/* or negative scanning does not work this way (as it has been reported   */
/* for the ScanMaker 35t+), you can set this to 1 to force mtekscan to    */
/* invert the image by using an inverted gamma table.                     */
#define INVERT_USING_GAMMA 1

/* Default scanning options.                                              */
/* You can change these defaults if you wish to use a particular          */
/* setting most of the time.                                              */
#define DEFAULT_RESOLUTION 150
/* A negative paper length value tells mtekscan to use the maximum        */
/* document length supported by the scanner as the paper length setting.  */
#define DEFAULT_PAPER_LENGTH -1
/* Scan type can be LINEART, HALFTONE, GRAYSCALE or COLOR                 */
#define DEFAULT_SCAN_TYPE GRAYSCALE
/* The default scanning frame size. The top left corner is at the         */
/* origin (0,0) by default. 8.2" x 11.7" = DIN A4                         */
#define DEFAULT_WIDTH  8.2
#define DEFAULT_HEIGHT 11.7
/* The halftone pattern must be in the range 0..11                        */
#define DEFAULT_HALFTONE_PATTERN 0



/*------------------------------------------------------------------------*/
/*         Don't change anything beyond this point unless you             */
/*                    really know what you're doing                       */
/*------------------------------------------------------------------------*/

/* The following #defines are for debugging only.                         */
#define FORCE_5_PERCENT_STEPS 0
#define FORCE_3PASS 0
#define SHOW_MODE_SENSE 0
#define WRITE_GAMMA 0
#define SCSI_DEBUGLEVEL 0
#define DEBUG 0

/* Version number, obviously. Don't change.                               */
#define VERSION "0.2"
