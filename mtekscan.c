/*
   mtekscan.c

   Linux driver for MicroTek SCSI scanners (and compatibles).

   Copyright (c) 1996,1997 Jan Schoenepauck / Fast Forward Productions
   <schoenep@uni-wuppertal.de>

   For latest information, check:
   http://fb4-1112.uni-muenster.de/ffwd/

   $Id: mtekscan.c 1.5 1997/09/16 04:43:59 parent Exp parent $
*/


#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#ifdef __EMX__
#include <ctype.h>
#endif
#include "config.h"
#include "global.h"
#include "options.h"
#include "scsi.h"
#include "gammatab.h"
#include "mt_defs.h"
#include "mt_error.h"

char *myname;

int use_expanded_resolution;
int fast_color_prescan;
int resolution_setting_unit;
int multi_bit_scan;
int one_pass_color_scan;

unsigned char contrast;
unsigned char exposure;
FILE *output_file;
unsigned char *imagebuffer;
unsigned char *xferbuffer;
unsigned int ibufsize;
int image_width, image_height;

int scanning = 0;


/* Macros */

#define ERROR_RETURN(string...) { fprintf(stderr, "%s: ", myname); \
fprintf(stderr, ##string);  scsi_close_device(); return(RET_FAIL); }

#define EXIT_CLOSE(code) { scsi_close_device(); exit(code); }
#define SWAP(a,b) { int t; (t) = (a);  (a) = (b);  (b) = (t); }

#define DEBUG_MSG(string) { fprintf(stderr, "%s: %s\n", myname, string); }

/*------------------------------------------------------------------------*/
/* int convert_unit (float value)                                         */
/* Converts between measurement units. The input value unit is defined    */
/* by src_unit and is converted into the unit defined by set_unit.        */
/* Args: value  - the size to be converted                                */
/* Return: The converted value in int format.                             */
/*------------------------------------------------------------------------*/
int convert_unit (float value) {
  switch (set_unit) {
  case EIGHTHINCH :
    switch (src_unit) {
    case CM :
      return((int)((value / 2.54) * 8));
    case MM :
      return((int)((value / 25.4) * 8));
    case EIGHTHINCH :
      return((int)value);
    default :
      return((int)(value * 8));
    }
    break;
  case PIXEL :
    switch (src_unit) {
    case CM :
    case MM :
    case EIGHTHINCH :
    default :
    }
  }
  return(0);
}


/*------------------------------------------------------------------------*/
/* unsigned char get_resolution_byte (int desired_level)                  */
/* Computes the resolution code needed by the scanner from the specified  */
/* resolution value <desired_level>                                       */
/* Args: <desired_level>  Resolution value.                               */
/* Return: Resolution value or 0 if desired resolution is too high.       */
/*------------------------------------------------------------------------*/
unsigned char get_resolution_byte (int desired_level) {
  float max_in_range;
  float temp;
  int min_val;
  
/* some special cases */
  if ((MAX_BASE_RESOLUTION == 600) && (mt_res_5percent)) {
    if (desired_level == 400) return(0x17);
    if (desired_level == 200) return(0x1d);
  }
  if ((MAX_BASE_RESOLUTION == 300) && (mt_res_5percent)) {
    if (desired_level == 200) return(0x17);
    if (desired_level == 100) return(0x1d);
  }
  if (desired_level > MAX_BASE_RESOLUTION) {
#if (IGNORE_XRES_FLAG)
    if (desired_level > MAX_EXPANDED_RESOLUTION) {
#else
    if ((!mt_ExpandedResolution) || 
       (desired_level > MAX_EXPANDED_RESOLUTION)) {
#endif
      fprintf(stderr, "%s: Selected resolution exceeds maximum.\n", myname);
      return(0);
    }
    max_in_range = MAX_EXPANDED_RESOLUTION;
    use_expanded_resolution = 1;
  }
  else {
    max_in_range = MAX_BASE_RESOLUTION;
    use_expanded_resolution = 0;
  }
  if ((mt_res_1percent) && (!FORCE_5_PERCENT_STEPS)) {
    resolution_setting_unit = 1;
    return((unsigned char)(((float)desired_level * 100) / max_in_range));
  }
  else {
    resolution_setting_unit = 5;
    min_val = ((MAX_BASE_RESOLUTION == 400)||(MAX_BASE_RESOLUTION == 800)) ?
              0x20 : 0x10;
    temp = (max_in_range - (float)desired_level) / (max_in_range * 0.05);
    if ((unsigned char)temp > 0x0f) {
      fprintf(stderr, "%s: Resolution must be at least %d dpi.\n",
              myname, (int)(max_in_range*0.25));
      return(0);
    }
    return(min_val + (unsigned char)temp);
  }
}


/*------------------------------------------------------------------------*/
int wait_unit_ready (void) {
  scsi_6byte_cmd TEST_UNIT_READY = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  int retry = 0;

#if (DEBUG)
  DEBUG_MSG("wait_unit_ready() called.");
#endif

  while (scsi_handle_cmd(TEST_UNIT_READY, 6, NULL, 0, NULL, 0)
         != RET_SUCCESS) {
    retry++;
    if (retry == 5) {
      fprintf(stderr, "%s: Timeout waiting for scanner.\n", myname);
      return(RET_FAIL);
    }
    if (verbose)
      fprintf(stderr, "%s: Waiting for scanner to get ready (retry %d).\n",
              myname, retry);
    sleep(3);
  }

#if (DEBUG)
  DEBUG_MSG("wait_unit_ready() done.");
#endif
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
int selftest (void) {
  scsi_6byte_cmd SEND_DIAGNOSTIC = { 0x1d, 0x04, 0x00, 0x00, 0x00, 0x00 };
  scsi_6byte_cmd RECEIVE_RESULTS = { 0x1c, 0x00, 0x00, 0x00, 0x01, 0x00 };
  unsigned char diag_result;

  fprintf(stderr, "%s: Performing scanner selftest...\n", myname);

  if (scsi_handle_cmd(SEND_DIAGNOSTIC, 6, NULL, 0, NULL, 0) != RET_SUCCESS) {
    check_sense(scsi_sensebuffer);
    ERROR_RETURN("Error executing SEND DIAGNOSTIC command.\n");
  }

  sleep(5);
  wait_unit_ready();

  if (scsi_handle_cmd(RECEIVE_RESULTS, 6, NULL, 0, &diag_result, 1)
      != RET_SUCCESS) {
    check_sense(scsi_sensebuffer);
    ERROR_RETURN("Error executing RECEIVE DIAGNOSTIC RESULTS command.\n");
  }

  if (diag_result == 0x00)
    fprintf(stderr, "%s: Selftest result code 0x%02x (no failures).\n",
            myname, diag_result);
  else {
    fprintf(stderr, "%s: Selftest result code 0x%02x.\n", myname, diag_result);
    if (diag_result & 0x01)
      fprintf(stderr, "    CPU RAM failure\n");
    if (diag_result & 0x02)
      fprintf(stderr, "    Scanner system RAM failure\n");
    if (diag_result & 0x04)
      fprintf(stderr, "    Image RAM failure (image buffer or "
                      "ping-pong buffer)\n");
    if (diag_result & 0x10)
      fprintf(stderr, "    DC offset error (black level "
                      "calibration failure)\n");
    if (diag_result & 0x20)
      fprintf(stderr, "    Scanning lamp or image sensor circuit failure\n");
    if (diag_result & 0x40)
      fprintf(stderr, "    Scanning head motor or home position "
                      "sensor failure\n");
    if (diag_result & 0x80)
      fprintf(stderr, "    Automatic document feeder paper "
                      "ejection failure\n");
  }
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
int scanning_frame (float xf1, float yf1, float xf2, float yf2) {
  scsi_6byte_cmd SCANNING_FRAME = { 0x04, 0x00, 0x00, 0x00, 0x09, 0x00 };
  unsigned char frame_data[9];
  int x1, y1, x2, y2;
  float ymax = doc_max_y;

#if (DEBUG)
  DEBUG_MSG("scanning_frame() called.");
#endif

  if ((paper_length >= 0) && (paper_length < doc_max_y)) ymax = paper_length;
  if ((xf1 >= doc_max_x)||(xf2 >= doc_max_x)||(yf1 >= ymax)||(yf2 >= ymax))
    ERROR_RETURN("Scanning frame coordinate exceeds max. document size.\n");
#if (FIX_FRAMESIZE)
  xf1 /= 2;
  yf1 /= 2;
  xf2 /= 2;
  yf2 /= 2;
#endif
  x1 = convert_unit(xf1);
  y1 = convert_unit(yf1);
  x2 = convert_unit(xf2);
  y2 = convert_unit(yf2);
  if (x1 == x2)
    ERROR_RETURN("Scanning frame has zero width.\n");
  if (y1 == y2)
    ERROR_RETURN("Scanning frame has zero height.\n");
  if (x2 < x1) SWAP(x1, x2);
  if (y2 < y1) SWAP(y1, y2);
  frame_data[0] = 0;
  if (scan_type == HALFTONE) frame_data[0] |= 0x01;
  if (set_unit == PIXEL)    frame_data[0] |= 0x08;
  frame_data[1] = (unsigned char)(x1 & 0xff);
  frame_data[2] = (unsigned char)((x1 & 0xff00) >> 8);
  frame_data[3] = (unsigned char)(y1 & 0xff);
  frame_data[4] = (unsigned char)((y1 & 0xff00) >> 8);
  frame_data[5] = (unsigned char)(x2 & 0xff);
  frame_data[6] = (unsigned char)((x2 & 0xff00) >> 8);
  frame_data[7] = (unsigned char)(y2 & 0xff);
  frame_data[8] = (unsigned char)((y2 & 0xff00) >> 8);
  if (scsi_handle_cmd(SCANNING_FRAME, 6, frame_data, 9, NULL, 0)
      != RET_SUCCESS) {
    check_sense(scsi_sensebuffer);
    ERROR_RETURN("Error executing SCANNING FRAME command.\n");
  }

#if (DEBUG)
  DEBUG_MSG("scanning_frame() done.");
#endif
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
int mode_select (void) {
  scsi_6byte_cmd MODE_SELECT = { 0x15, 0x00, 0x00, 0x00, 0x00, 0x00 };
  unsigned char mode_data[11];
  int paperlen;
  int tlength = 0x0a;

#if (DEBUG)
  DEBUG_MSG("mode_select() called.");
#endif

  if (mt_MidtoneAdjustment) tlength = 0x0b;
  MODE_SELECT[4] = (unsigned char)(tlength & 0xff);
  if (paper_length < 0) paper_length = doc_max_y;
  paperlen = convert_unit(paper_length);
  mode_data[0] = 0x81;
  mode_data[1] = get_resolution_byte(scan_resolution);
  if (mode_data[1] == 0x00) return(RET_FAIL);
  if (resolution_setting_unit == 1) mode_data[0] |= 0x02;
  if (set_unit == PIXEL)            mode_data[0] |= 0x08;
  mode_data[2]  = exposure;
  mode_data[3]  = contrast;
  mode_data[4]  = halftone_pattern;
  mode_data[5]  = scan_velocity;
  mode_data[6]  = shadow_adjust;
  mode_data[7]  = highlight_adjust;
  mode_data[8]  = (unsigned char)(paperlen & 0xff);
  mode_data[9]  = (unsigned char)((paperlen & 0xff00) >> 8);
  mode_data[10] = midtone_adjust;
  if (scsi_handle_cmd(MODE_SELECT, 6, mode_data, tlength, NULL, 0)
      != RET_SUCCESS) {
    check_sense(scsi_sensebuffer);
    ERROR_RETURN("Error executing MODE SELECT command.\n");
  }

#if (DEBUG)
  DEBUG_MSG("mode_select() done.");
#endif
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
int mode_select_1 (void) {
  scsi_6byte_cmd MODE_SELECT_1 = { 0x16, 0x00, 0x00, 0x00, 0x0a, 0x00 };
  unsigned char mode_data[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

#if (DEBUG)
  DEBUG_MSG("mode_select_1() called.");
#endif

  mode_data[1] = (unsigned char)brightness_adjust_r;
  if (!allow_calibration) mode_data[3] |= 0x02;
  if (scsi_handle_cmd(MODE_SELECT_1, 6, mode_data, 10, NULL, 0)
      != RET_SUCCESS) {
    check_sense(scsi_sensebuffer);
    ERROR_RETURN("Error executing MODE SELECT 1 command.\n");
  }

#if (DEBUG)
  DEBUG_MSG("mode_select_1() done.");
#endif
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
int mode_sense (void) {
  scsi_6byte_cmd MODE_SENSE = { 0x1a, 0x00, 0x00, 0x00, 0x00, 0x00 };
  unsigned char mode_data[20];
  int tlength = 0x13;

#if (DEBUG)
  DEBUG_MSG("mode_sense() called.")
#endif

  if (!mt_OnePass) {
    if (mt_MidtoneAdjustment) tlength = 0x0b;
    else tlength = 0x0a;
  }
  MODE_SENSE[4] = (unsigned char)(tlength & 0xff);
  if (scsi_handle_cmd(MODE_SENSE, 6, NULL, 0, mode_data, tlength) 
      != RET_SUCCESS) {
    check_sense(scsi_sensebuffer);
    ERROR_RETURN("Error executing MODE SENSE command.\n");
  }
#if (SHOW_MODE_SENSE)
  {
    int i;
    fprintf(stderr, "%s: MODE SENSE data:\n", myname);
    for (i=0; i<tlength; i++) fprintf(stderr, "%02x ", mode_data[i]);
    fprintf(stderr, "\n");
  }
#endif
#if (DEBUG)
  DEBUG_MSG("mode_sense() done.");
#endif
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
int mode_sense_1 (void) {
  scsi_6byte_cmd MODE_SENSE_1 = { 0x19, 0x00, 0x00, 0x00, 0x1e, 0x00 };
  unsigned char mode_data[30] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

#if (DEBUG)
  DEBUG_MSG("mode_sense_1() called.");
#endif

  mode_data[1] = (unsigned char)brightness_adjust_r;
  mode_data[2] = (unsigned char)brightness_adjust_g;
  mode_data[3] = (unsigned char)brightness_adjust_b;
  if (scsi_handle_cmd(MODE_SENSE_1, 6, mode_data, 30, NULL, 0)
      != RET_SUCCESS) {
    check_sense(scsi_sensebuffer);
    ERROR_RETURN("Error executing MODE SENSE 1 command.\n");
  }

#if (DEBUG)
  DEBUG_MSG("mode_sense_1() done.");
#endif
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
int accessory_and_backtracking (void) {
  scsi_6byte_cmd ACCESSORY = { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00 };

#if (DEBUG)
  DEBUG_MSG("accessory_and_backtracking() called.");
#endif

  if (use_adf)            ACCESSORY[4] |= 0x41;
  if (prescan)            ACCESSORY[4] |= 0x18;
  if (transparency)       ACCESSORY[4] |= 0x24;
  if (allow_backtracking) ACCESSORY[4] |= 0x81;
  if (scsi_handle_cmd(ACCESSORY, 6, NULL, 0, NULL, 0) != RET_SUCCESS) {
    check_sense(scsi_sensebuffer);
    ERROR_RETURN("Error executing ACCESSORY & BACKTRACKING command.\n");
  }

#if (DEBUG)
  DEBUG_MSG("accessory_and_backtracking() done.");
#endif
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
int start_scan (char filter) {
  scsi_6byte_cmd START_SCAN = { 0x1b, 0x00, 0x00, 0x00, 0x01, 0x00 };

#if (DEBUG)
  DEBUG_MSG("start_scan() called.");
#endif

  if (use_expanded_resolution) START_SCAN[4] |= 0x80;
  if (multi_bit_scan)          START_SCAN[4] |= 0x40;
  if (one_pass_color_scan)     START_SCAN[4] |= 0x20;
  if (reverse_colors)          START_SCAN[4] |= 0x04;
  if (fast_color_prescan)      START_SCAN[4] |= 0x02;
  switch(filter) {
  case RED :
    START_SCAN[4] |= 0x08;  break;
  case GREEN :
    START_SCAN[4] |= 0x10;  break;
  case BLUE :
    START_SCAN[4] |= 0x18;  break;
  case CLEAR :
  default :
  }
  if (scsi_handle_cmd(START_SCAN, 6, NULL, 0, NULL, 0) != RET_SUCCESS) {
    check_sense(scsi_sensebuffer);
    ERROR_RETURN("Error executing START SCAN command.\n");
  }

#if (DEBUG)
  DEBUG_MSG("start_scan() done.");
#endif
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
int stop_scan (void) {
  scsi_6byte_cmd STOP_SCAN = { 0x1b, 0x00, 0x00, 0x00, 0x00, 0x00 };
  int retry = 0;
  int result, error;

#if (DEBUG)
  DEBUG_MSG("stop_scan() called.");
#endif

  do {
    result = scsi_handle_cmd(STOP_SCAN, 6, NULL, 0, NULL, 0);
    if (result != RET_SUCCESS) {
      error = check_sense(scsi_sensebuffer);
      if ((error == HW_ERROR) || (error == OP_ERROR)) return(-1);
      retry++;
      if (verbose)
        fprintf(stderr, "%s: Waiting for scan head to return (retry %d).\n",
                myname, retry);
      sleep(WAIT_RETURN_SLEEP_TIME);
    }
  } while ((result != RET_SUCCESS) && (retry < MAX_WAIT_RETURN_RETRY));
  if (result < 0) 
    ERROR_RETURN("Error executing STOP SCAN command (timeout).\n");

#if (DEBUG)
  DEBUG_MSG("stop_scan() done.");
#endif
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
int get_scan_status (int *busy, int *lw, int *rmg) {
  scsi_6byte_cmd GET_SCAN_STATUS = { 0x0f, 0x00, 0x00, 0x00, 0x06, 0x00 };
  unsigned char status_reply[6];
  int retry = 0;

#if (DEBUG)
  DEBUG_MSG("get_scan_status() called.");
#endif

  do {
    if (scsi_handle_cmd(GET_SCAN_STATUS, 6, NULL, 0, status_reply, 6)
        != RET_SUCCESS) {
      check_sense(scsi_sensebuffer);
      ERROR_RETURN("Error executing GET SCAN STATUS command.\n");
    }
    *busy = (int)(status_reply[0] != 0);
    *lw   = (int)(status_reply[1] 
            +       256 * status_reply[2]);
    *rmg  = (int)(status_reply[3] 
            +       256 * status_reply[4] 
            + 256 * 256 * status_reply[5]);
    if (*busy) {
      retry++;
      fprintf(stderr, "%s: Scanner is busy, retrying (%d)...\n",
              myname, retry);
      sleep(BUSY_SLEEP_TIME);
    }
  } while ((*busy) && (retry < MAX_BUSY_RETRY));
  if (*busy)
    ERROR_RETURN("Maximum retry reached - aborting.\n");

#if (DEBUG)
  DEBUG_MSG("get_scan_status() done.");
#endif
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
int read_scanned_data (int nlines, unsigned char *buffer, int bsize) {
  scsi_6byte_cmd READ_SCANNED_DATA = { 0x08, 0x00, 0x00, 0x00, 0x00, 0x00 };

#if (DEBUG)
  DEBUG_MSG("read_scanned_data() called.");
#endif

  READ_SCANNED_DATA[2] = (unsigned char)((nlines & 0xff0000) >> 16);
  READ_SCANNED_DATA[3] = (unsigned char)((nlines & 0xff00) >> 8);
  READ_SCANNED_DATA[4] = (unsigned char)(nlines & 0xff);
  if (scsi_handle_cmd(READ_SCANNED_DATA, 6, NULL, 0, buffer, bsize)
      != RET_SUCCESS) {
    check_sense(scsi_sensebuffer);
    ERROR_RETURN("Error executing READ SCANNED DATA command for %d lines.\n",
                 nlines);
  }

#if (DEBUG)
  DEBUG_MSG("read_scanned_data() done.");
#endif
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
/* int lut_download (char filter, char *gtable, int gt_entries)           */
/* Issue the LOOK-UP-TABLE DOWNLOAD command to the scanner to download    */
/* the gamma table pointed to by <gtable>. <filter> determines the color  */
/* to which the table applies (RED, GREEN, BLUE, or CLEAR; values other   */
/* than CLEAR may only be specified for one-pass color scanners). The     */
/* number of entries is determined by <entries> (256, 1024, 4096 or       */
/* 65536). The entry width is automatically assumed to be one byte if the */
/* number of entries is 256 and two bytes otherwise.                      */
/*------------------------------------------------------------------------*/
int lut_download (char filter, char *gtable, int entries) {
  scsi_10byte_cmd LUT_DOWNLOAD = { 0x55, 0x00, 0x27, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00 };
  int transfer_length;

#if (DEBUG)
  DEBUG_MSG("lut_download() called.");
#endif

  transfer_length = entries * ((entries > 256) ? 2 : 1);
  LUT_DOWNLOAD[7] = (unsigned char)((transfer_length & 0xff00) >> 8);
  LUT_DOWNLOAD[8] = (unsigned char)(transfer_length & 0xff);
  if (gt_entries > 256) LUT_DOWNLOAD[9] |= 0x01;
  switch (filter) {
  case RED :
    LUT_DOWNLOAD[9] |= 0x40;
    break;
  case GREEN :
    LUT_DOWNLOAD[9] |= 0x80;
    break;
  case BLUE :
    LUT_DOWNLOAD[9] |= 0xc0;
    break;
  case CLEAR :
  default :
  }
  if (scsi_handle_cmd(LUT_DOWNLOAD, 10, gtable, transfer_length, NULL, 0)
      != RET_SUCCESS) {
    check_sense(scsi_sensebuffer);
    ERROR_RETURN("Error sending LOOK-UP-TABLE DOWNLOAD command.\n");
  }

#if (DEBUG)
  DEBUG_MSG("lut_download() done.");
#endif
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
/* void abort_scan (int s)                                                */
/* When scanning starts, this is made the signal handler for SIGINT, so   */
/* scanning is correctly terminated with a STOP SCAN command. Also called */
/* (with s = -1) if errors occur after the START SCAN command.            */
/* Args: <s>  signal number or -1 if called directly                      */
/*------------------------------------------------------------------------*/
void abort_scan (int s) {
  free(imagebuffer);
  free(xferbuffer);
  stop_scan();
  if (s == -1)
    fprintf(stderr, "\n%s: Abort scan called.\n", myname);
  else if (verbose) 
    fprintf(stderr, "\n%s: Scan aborted (caught signal %d).\n", myname, s);
  EXIT_CLOSE(RET_FAIL);
}


/*------------------------------------------------------------------------*/
/* int scan (void)                                                        */
/* Main scanning function. The 3-pass code is based on the patch I got    */
/* from Warwick Allison <warwick@cs.uq.edu.au>.                           */
/* Return: RET_SUCCESS if successful, RET_FAIL if not.                    */
/*------------------------------------------------------------------------*/
int scan (void) {
  int scanner_busy, linewidth, remaining;
  int bytes_per_line, header_bytes_per_line;
  int total_lines, lines_read[3];
  int nlines;
  int planes, plane;
  int passes, pass;
  int i, j;
  unsigned char *bptr;

  xferbuffer = (unsigned char *)malloc(scsi_memsize);
  if (xferbuffer == NULL) {
    perror("malloc");
    ERROR_RETURN("Cannot allocate %d bytes for transfer buffer.\n",
                 scsi_memsize);
  }

  if (wait_unit_ready() != RET_SUCCESS)
    ERROR_RETURN("TEST UNIT READY failed.\n");

  lines_read[0] = lines_read[1] = lines_read[2] = 0;
  contrast = (contrast_adjust / 7) + 7;
  exposure = (exposure_time_adjust / 3) + 7;
#if (USE_FCP)
  if (prescan && mt_FastColorPrescan) fast_color_prescan = 1;
  else fast_color_prescan = 0;
#else
  fast_color_prescan = 0;
#endif
  if ((scan_type == COLOR) || (scan_type == GRAYSCALE)) multi_bit_scan = 1;
  else multi_bit_scan = 0;
#if (FORCE_3PASS)
  mt_OnePass = 0;
#endif
  if (scan_type == COLOR) {
    if (mt_OnePass) {
      one_pass_color_scan = 1;
      passes = 1;
    }
    else {
      one_pass_color_scan = 0;
      passes = 3;
    }
  }
  else {
    one_pass_color_scan = 0;
    passes = 1;
  }

  if (mt_MODE_SELECT_1) {
    if (mode_select_1() != RET_SUCCESS) return(RET_FAIL);
    if (mode_sense_1()  != RET_SUCCESS) return(RET_FAIL);
  }

  if (mt_SWslct)
    if (accessory_and_backtracking() != RET_SUCCESS) return(RET_FAIL);

  if (mode_sense() != RET_SUCCESS) return(RET_FAIL);

  if (verbose) {
    fprintf(stderr, "%s: Scan settings\n", myname);
    fprintf(stderr, "  frame: %2.2f, %2.2f to %2.2f, %2.2f %s, %d dpi, "
            "paper length %2.2f %s\n",
            scan_frame_x1, scan_frame_y1,
            scan_frame_x2, scan_frame_y2, unit_string[src_unit],
            scan_resolution,
            (paper_length<0) ? doc_max_y : paper_length, unit_string[src_unit]);
    fprintf(stderr, "  exposure time %d%%, contrast %d%%\n",
            (exposure - 7) * 3, (contrast - 7) * 7);
    fprintf(stderr, "  brightness adjustment: red 0x%02x, green 0x%02x, "
            "blue 0x%02x\n",
            (unsigned char)brightness_adjust_r,
            (unsigned char)brightness_adjust_g,
            (unsigned char)brightness_adjust_b);
    fprintf(stderr, "  black: 0x%02x  gray: 0x%02x  white: 0x%02x\n",
            shadow_adjust, midtone_adjust, highlight_adjust);
    if (gammatable_filename != NULL)
      fprintf(stderr, "  gamma: <%s>\n", gammatable_filename);
    else
      fprintf(stderr, "  gamma: red %.2f, green %.2f, blue %.2f\n",
              red_gamma, green_gamma, blue_gamma);
    if (scan_type == HALFTONE) 
      fprintf(stderr, "  halftone pattern 0x%02x\n", halftone_pattern);
    fprintf(stderr, "  writing to %s\n", output_filename ? output_filename :
            "stdout");
  }

  for (pass = 0; pass < passes; pass++) {
    if (scanning_frame(scan_frame_x1, scan_frame_y1, 
                       scan_frame_x2, scan_frame_y2) != RET_SUCCESS)
       return(RET_FAIL);

    if (mt_MaxLookupTableSize) {          /* download gamma table */
      if (verbose)
        fprintf(stderr,"%s: Downloading gamma table...\n", myname);
      if (one_pass_color_scan) {
        lut_download(RED, gt_r, gt_entries);
        lut_download(GREEN, gt_g, gt_entries);
        lut_download(BLUE, gt_b, gt_entries);
      }
      else {
        switch (pass) {
        case 1 :
          lut_download(CLEAR, gt_g, gt_entries);
          break;
        case 2 :
          lut_download(CLEAR, gt_b, gt_entries);
          break;
        case 0 :
        default :
          lut_download(CLEAR, gt_r, gt_entries);
          break;
        }
      }
    }
    if (mode_select() != RET_SUCCESS) return(RET_FAIL);
    if (mode_sense()  != RET_SUCCESS) return(RET_FAIL);

    if ((verbose) && (allow_calibration)) {
      if (passes == 1) fprintf(stderr, "%s: Calibrating scanner...\n", myname);
      else fprintf(stderr, "%s: Calibrating %s...\n", myname, 
                   (pass == 0) ? "red" : ((pass == 1) ? "green" : "blue"));
    }

    signal(SIGINT, abort_scan);
    signal(SIGSEGV, abort_scan);
   
    if (passes == 3) {
      if (start_scan((pass == 0) ? RED : ((pass == 1) ? GREEN : BLUE))
          != RET_SUCCESS)
        return(RET_FAIL);
    }
    else if (start_scan(CLEAR) != RET_SUCCESS) return(RET_FAIL);

    if (get_scan_status(&scanner_busy, &linewidth, &remaining) < 0)
      abort_scan(-1);

/* Some strange errors can only be detected by this check */
    if (linewidth <= 0) {
      fprintf(stderr, "%s: Scanner returned suspicious line width value %d.\n",
              myname, linewidth);
      abort_scan(-1);
    }

    switch (scan_type) {
    case LINEART :
    case HALFTONE :
      bytes_per_line = linewidth;
      header_bytes_per_line = 0;
      image_width = bytes_per_line * 8;
      planes = 1;
      break;
    case GRAYSCALE :
      if (bits_per_color < 8) {
        bytes_per_line = linewidth;
        image_width = linewidth * (8 / bits_per_color);
      }
      else {
        bytes_per_line = ((bits_per_color + 7) / 8) * linewidth;
        image_width = linewidth;
      }
      header_bytes_per_line = 0;
      planes = 1;
      break;
    case COLOR :
      switch (mt_ColorDataSequencing) {
      case 0x00 :
        header_bytes_per_line = 0;
        bytes_per_line = ((bits_per_color + 7) / 8) * linewidth;
        image_width = linewidth;
        planes = 1;
        break;
      case 0x03 :
        header_bytes_per_line = 6;
        bytes_per_line = ((bits_per_color + 7) / 8) * 3 * (linewidth - 2);
        image_width = linewidth - 2;
        planes = 3;
        break;
      default :
        header_bytes_per_line = 0;
        bytes_per_line = ((bits_per_color + 7) / 8) * 3 * linewidth;
        image_width = linewidth;
        planes = 3;
      }
      break;
    default :
      fprintf(stderr, "%s: Unknown scan type (%d).\n", myname, scan_type);
      abort_scan(-1);
      return(RET_FAIL);
    }

#if (FORCE_3PASS)
    header_bytes_per_line = 0;
    bytes_per_line = ((bits_per_color + 7) / 8) * linewidth;
    image_width = linewidth;
    planes = 1;
#endif

    image_height = remaining;
    if (verbose) {
      fprintf(stderr, "%s: %s %s image, %dx%d pixel\n",
              myname, prescan ? "Previewing" : "Scanning",
              (scan_type == COLOR) ? ((passes == 1) ? "color" : 
              ((pass == 0) ? "red" : ((pass == 1) ? "green" : "blue"))) :
              ((scan_type == GRAYSCALE) ? "grayscale" : 
              ((scan_type == HALFTONE) ? "halftone" : "line art")),
              image_width, image_height);
    }

    if (pass == 0) {
      ibufsize = bytes_per_line * remaining * passes;
      imagebuffer = (unsigned char *)malloc(ibufsize);
      if (imagebuffer == NULL) {
        perror("malloc");
        fprintf(stderr, "%s: Cannot allocate %d bytes for image buffer.\n",
                myname, ibufsize);
        abort_scan(-1);
      }
    }
    total_lines = remaining;
    if (verbose) {
      fprintf(stderr, "%s: Remaining lines: %04d", myname, remaining);
      fflush(stderr);
    }

    while (lines_read[0]+lines_read[1]+lines_read[2] < 
           total_lines * planes * (pass + 1)) {
      if (get_scan_status(&scanner_busy, &linewidth, &remaining) < 0)
        abort_scan(-1);
      nlines = remaining;
      if (nlines * (bytes_per_line + header_bytes_per_line) > scsi_memsize) {
        nlines = scsi_memsize / (bytes_per_line + header_bytes_per_line);
/* Error check suggested by Itai Nahshon <nahshon@almaden.ibm.com> */
        if (nlines < 1) {
          fprintf(stderr, "%s: SCSI buffer can not hold single line "
                          "of scanned data.\n", myname);
          abort_scan(-1);
        }
      }
      if (read_scanned_data(nlines, xferbuffer, 
          (bytes_per_line + header_bytes_per_line) * nlines) < 0)
        abort_scan(-1);
      if (verbose) {
        fprintf(stderr, "\b\b\b\b%04d", remaining-nlines);
        fflush(stderr);
      }

      if ((scan_type == COLOR) && (one_pass_color_scan)) {
        bptr = xferbuffer;
        for (i = 0; i < nlines; i++) {
          for (j = 0; j < planes; j++) {
            if (header_bytes_per_line) {
              bptr++;
              plane = (bptr[0] == 'R') ? 0 : ((bptr[0] == 'G') ? 1 : 2);
              bptr++;
            }
            else plane = j;
            memcpy(imagebuffer + (plane * image_width * image_height) + 
                   (lines_read[plane] * bytes_per_line / planes), bptr, 
                   bytes_per_line / 3);
            bptr += bytes_per_line / 3;
            lines_read[plane]++;
          }
        }
      }
      else if ((fast_color_prescan) && (pass == 1)) {
        for (i = 0; i < nlines; i++) {
          lines_read[0]++;
          memcpy(imagebuffer + (2 * bytes_per_line * image_height) -
                 (lines_read[0] * bytes_per_line), xferbuffer +
                 (i * bytes_per_line), bytes_per_line);
        }
      }
      else {
        memcpy(imagebuffer + (lines_read[0] * bytes_per_line), xferbuffer, 
               nlines * bytes_per_line);
        lines_read[0] += nlines;
      }
    } /* end of read loop */
    if (verbose) fprintf(stderr, "\n");
    if (stop_scan() != RET_SUCCESS) return(RET_FAIL);
  } /* end of pass loop */
  signal(SIGINT, SIG_DFL);
  signal(SIGSEGV, SIG_DFL);
  free(xferbuffer);
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
/* int open_output_file (void)                                            */
/* Tries to open the output file specified by <output_filename> and sets  */
/* <output_file> to the file pointer. If <output_filename> is NULL,       */
/* <output_file> is set to stdout.                                        */
/* Return: 0 if successful, -1 if file could not be opened.               */
/*------------------------------------------------------------------------*/
int open_output_file (void) {
  if (output_filename == NULL) {
    output_file = stdout;
    return(RET_SUCCESS);
  }
#ifndef __EMX__
#define WRTMODE "w"
#else	/* OS/2 */
#define WRTMODE "wb"
#endif
  if ((output_file = fopen(output_filename, WRTMODE)) == NULL) {
    perror(myname);
    return(RET_FAIL);
  }
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
/* void write_image (int raw)                                             */
/* Writes the image file to <output_file> in pbm/pgm/ppm format (depen-   */
/* ding on the value of <scan_type>).                                     */
/* Includes some changes submitted by Wolfgang Wander <wwc@lars.desy.de>  */
/* to speed up writing of data.                                           */
/* Args: <raw>  If this is nonzero, the image is written in pbm/pgm/ppm   */
/*              raw format (smaller, faster, better) if possible.         */
/*              Otherwise the longer, but more generic standard ASCII     */
/*              format is written.                                        */
/* Note: Raw format cannot be used if the maximum value for a color       */
/*       component is larger than 255, i.e. in 10 or 12 bpp images.       */
/*------------------------------------------------------------------------*/
void write_image (int raw) {
  int i, j;
  unsigned char mask;
  char wbuf[3];
  char magic_number[3] = { 'P', '0', 0 };

  switch (scan_type) {
  case LINEART :
  case HALFTONE :
    magic_number[1] = (raw ? '4' : '1');  break;
  case GRAYSCALE :
    magic_number[1] = (raw ? '5' : '2');  break;
  case COLOR :
    magic_number[1] = (raw ? '6' : '3');  break;
  }
  fprintf(output_file, "%s\n# Created with mtekscan v.%s\n%d %d\n",
          magic_number, VERSION,  image_width, image_height);
  if ((scan_type == GRAYSCALE) || (scan_type == COLOR))
    fprintf(output_file, "255\n");
  if (scan_type == GRAYSCALE) {
    for (i = 0; i < image_width * image_height; i++) {
      if (raw)
        fputc(imagebuffer[i], output_file);
      else
        fprintf(output_file, "%d%s", imagebuffer[i], (i%16==15) ? "\n" : " ");
    }
  }
  else if (scan_type == COLOR) {
    for (i = 0; i < image_width * image_height; i++) {
      if (raw) {
        for (j = 0; j < 3; j++) 
          wbuf[j] = imagebuffer[i + image_width * image_height * j];
        fwrite(wbuf, 3, 1, output_file);
      }
      else {
        fprintf(output_file, "%d %d %d%s", imagebuffer[i], 
                imagebuffer[i + image_width * image_height],
                imagebuffer[i + image_width * image_height * 2],
                (i % 6 == 5) ? "\n" : " ");
      }
    }
  }
  else {
    if (raw)
      for (i = 0; i < ibufsize; i++)
        fputc(imagebuffer[i], output_file);
    else {
      mask = 0x80;
      j = 0;
      for (i = 0; i < image_width * image_height; i++) {
        fprintf(output_file, "%d%s", (imagebuffer[j] & mask) ? 1 : 0,
                (i % 30 == 29) ? "\n" : " ");
        mask = mask >> 1;
        if (mask == 0x00) {
          mask = 0x80;
          j++;
        }
      }
    }
  }
  fprintf(output_file, "\n");
}



/**************************************************************************/

/**********************           M A I N           ***********************/

/**************************************************************************/
int main (int argc, char **argv) {
  int result;

  myname = strrchr(argv[0], '/');
  if (myname == NULL) myname = argv[0];
  else myname++;

  scsi_debug_level = SCSI_DEBUGLEVEL;
#ifndef __EMX__
  scsi_device = (char *)malloc(strlen(DEFAULT_DEVICE)+1);
  strcpy(scsi_device, DEFAULT_DEVICE);
#else	/* OS/2 */
  if (argc > 1 && isdigit(argv[1][0])) {	// Get SCSI ID of scanner.
     scsi_device = (char *)malloc(strlen(argv[1]) + 1);
     strcpy(scsi_device, argv[1]);
  } else {
     usage();
     exit(RET_FAIL);
  }
#endif
  if (scsi_open_device() == RET_FAIL)
    ERROR_RETURN("Unable to open SCSI device.\n");

#ifndef __EMX__
  result = checkopt(argc, argv);
#else	/* OS/2 */
  result = checkopt(argc - 1, argv + 1);	// 1st opt. is scsi ID.
#endif
  if (result != RET_SUCCESS) {
    if (-result >= argc)
      ERROR_RETURN("Missing or invalid option(s).\n  Use %s -h for a list "
                   "of valid options.\n", myname);
    ERROR_RETURN("Illegal option '%s'.\n  Use %s -h for a list "
                 "of valid options.\n", argv[-result], myname);
  }

  if (open_output_file() != RET_SUCCESS)
    ERROR_RETURN("Unable to open output file '%s'.\n", output_filename);
  if (get_scanner_info() != RET_SUCCESS)
    ERROR_RETURN("Unable to get scanner info.\n");

  if (perform_selftest) {
    if (wait_unit_ready() != RET_SUCCESS) EXIT_CLOSE(RET_FAIL);
    if (selftest() != RET_SUCCESS) EXIT_CLOSE(RET_FAIL);
    EXIT_CLOSE(RET_SUCCESS);
  }

  if (gammatable_filename != NULL) {
    if (verbose)
      fprintf(stderr, "%s: Loading gamma tables from %s...\n", myname,
              gammatable_filename);
    if (read_gamma_tables(gammatable_filename) != RET_SUCCESS) {
      fprintf(stderr, "%s: Failed reading gamma tables from %s.\n",
              myname, gammatable_filename);
      EXIT_CLOSE(RET_FAIL);
    }
  }
  else {
    init_gamma_tables(mt_MaxLookupTableSize);
    create_gamma_table(gt_r, red_gamma);
    create_gamma_table(gt_g, green_gamma);
    create_gamma_table(gt_b, blue_gamma);
  }

#if (INVERT_USING_GAMMA)
  if (reverse_colors) {
    invert_gamma_table(gt_r);
    invert_gamma_table(gt_g);
    invert_gamma_table(gt_b);
    reverse_colors = 0;
  }
#endif

#if (WRITE_GAMMA)
  write_gamma_tables("mtekscan.gt");
#endif

  if (test_options() != RET_SUCCESS) EXIT_CLOSE(RET_FAIL);
  if (scan() != RET_SUCCESS) EXIT_CLOSE(RET_FAIL);
  free_gamma_tables();
  write_image(WRITE_RAW);
  free(imagebuffer);
  scsi_close_device();
  return(RET_SUCCESS);
}
