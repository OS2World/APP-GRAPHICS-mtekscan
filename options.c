/*
   options.c
   Commandline option parsing, scanner checking, option validation etc.

   Copyright (c) 1996, 1997 Jan Schoenepauck / Fast Forward Productions
   <schoenep@uni-wuppertal.de>

   $Id: options.c 1.3 1997/09/16 04:02:59 parent Exp $
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "global.h"
#include "options.h"
#include "gammatab.h"
#include "mt_defs.h"
#include "scsi.h"

extern char *myname;

char unit_string[6][10] = { "inch", "mm", "cm", "1/8 inch", "point", "pixel" };

/* User selectable scan options */

int perform_selftest           = 0;
int scan_resolution            = DEFAULT_RESOLUTION;
int scan_type                  = DEFAULT_SCAN_TYPE;
int reverse_colors             = 0;
int prescan                    = 0;
int use_adf                    = 0;
int allow_backtracking         = 1;
int allow_calibration          = 1;
int transparency               = 0;
int bits_per_color             = 8;
unsigned char scan_velocity    = 0x01;
unsigned char halftone_pattern = DEFAULT_HALFTONE_PATTERN;
int exposure_time_adjust       = 0;
int contrast_adjust            = 0;
char brightness_adjust_r       = 0;
char brightness_adjust_g       = 0;
char brightness_adjust_b       = 0;
unsigned char shadow_adjust    = 0x00;
unsigned char highlight_adjust = 0xff;
unsigned char midtone_adjust   = 0x80;
float red_gamma                = 1.0;
float green_gamma              = 1.0;
float blue_gamma               = 1.0;

float scan_frame_x1            = 0.0;
float scan_frame_y1            = 0.0;
float scan_frame_x2            = DEFAULT_WIDTH;
float scan_frame_y2            = DEFAULT_HEIGHT;

float paper_length             = DEFAULT_PAPER_LENGTH;

int src_unit                   = INCH;
int set_unit                   = EIGHTHINCH;

int verbose                    = 0;

char *output_filename          = NULL;
char *gammatable_filename      = NULL;

/* Options supported by the scanner, set by get_scanner_info() */

float doc_max_x;
float doc_max_y;
int cntr_vals;
int min_cntr, max_cntr;
int exp_vals;
int min_exp, max_exp;
int size_unit;

int mt_DeviceType;
int mt_SCSI_firmware_ver_major, mt_SCSI_firmware_ver_minor;
int mt_scanner_firmware_ver_major, mt_scanner_firmware_ver_minor;
int mt_response_data_format;
int mt_res_1percent, mt_res_5percent;
int mt_LineArt, mt_Hlftone, mt_MultBit, mt_Color;
int mt_Trnsmsv, mt_OnePass, mt_Negtv;
int mt_Builtin_Patterns, mt_DnLoad;
int mt_SnsMvmt, mt_PprMvmt, mt_AutoFdr, mt_HfCompr, mt_RdCompr;
int mt_18Set, mt_PxlSet;
int mt_DocSizeCode;
int mt_ContrastSettings, mt_ExposureTimeSettings;
int mt_ModelCode;
int mt_FWsupp, mt_SWslct, mt_FdrInst, mt_OutRdy, mt_TypLoc, mt_TxpInst;
int mt_ExpandedResolution;
int mt_ShadowHighlightAdjustment, mt_MidtoneAdjustment;
int mt_MaxLookupTableSize, mt_GammaValueMax, mt_GammaEntryLength;
int mt_FastColorPrescan, mt_DataTransferFmtSelect, mt_ColorDataSequencing;
int mt_3pass, mt_MODE_SELECT_1;
int mt_4bpp_supp, mt_10bpp_supp, mt_12bpp_supp, mt_16bpp_supp;
int mt_BrightnessControl;
int mt_DisableLinearizationTable, mt_DisableRecalibration;

char mt_VendorID[9];
char mt_ModelName[17];
char mt_RevisionNo[5];


/* Error reporting macros */

#define ERROR_RETURN(string...) { fprintf(stderr, "%s: ", myname); \
fprintf(stderr, ##string); return(RET_FAIL); }

#define WARNING(string...) { if (verbose) { fprintf(stderr, "%s: ", myname); \
fprintf(stderr, ##string); } }


/*------------------------------------------------------------------------*/
/* int get_scanner_info (void)                                            */
/* Performs an INQUIRY command to the SCSI device (which must be a        */
/* MICROTEK scanner - that should be tested elsewhere), and analyzes the  */
/* data returned by the target.                                           */
/* Return: RET_SUCCESS if successful, RET_FAIL if INQUIRY command failed. */
/* Note: this is different from the generic SCSI inquiry function in      */
/*       scsi.c, as it only reads the Microtek scanner specific data.     */
/*------------------------------------------------------------------------*/
int get_scanner_info (void) {
  unsigned char inquiry[96];    /* buffer for INQUIRY data */
  scsi_6byte_cmd INQUIRY = { 0x12, 0x00, 0x00, 0x00, 96, 0x00 };

  if (scsi_handle_cmd(INQUIRY, 6, NULL, 0, inquiry, 96)
      != RET_SUCCESS) 
    return(RET_FAIL);

  strncpy(mt_VendorID, &inquiry[8], 8);
  strncpy(mt_ModelName, &inquiry[16], 16);
  strncpy(mt_RevisionNo, &inquiry[32], 4);
  mt_VendorID[8]   = 0;
  mt_ModelName[16] = 0;
  mt_RevisionNo[5] = 0;

  mt_DeviceType                 = (int)(inquiry[0] & 0x1f);
  mt_SCSI_firmware_ver_major    = (int)((inquiry[1] & 0xf0) >> 4);
  mt_SCSI_firmware_ver_minor    = (int)(inquiry[1] & 0x0f);
  mt_scanner_firmware_ver_major = (int)((inquiry[2] & 0xf0) >> 4);
  mt_scanner_firmware_ver_minor = (int)(inquiry[2] & 0x0f);
  mt_response_data_format       = (int)inquiry[3];
  mt_res_1percent               = (inquiry[56] & 0x01) > 0;
  mt_res_5percent               = (inquiry[56] & 0x02) > 0;
  mt_LineArt                    = (inquiry[57] & 0x01) > 0;
  mt_Hlftone                    = (inquiry[57] & 0x02) > 0;
  mt_MultBit                    = (inquiry[57] & 0x04) > 0;
  mt_Color                      = (inquiry[57] & 0x08) > 0;
  mt_Trnsmsv                    = (inquiry[57] & 0x20) > 0;
  mt_OnePass                    = (inquiry[57] & 0x40) > 0;
  mt_Negtv                      = (inquiry[57] & 0x80) > 0;
  mt_Builtin_Patterns           = (int)(inquiry[58] & 0x7f);
  mt_DnLoad                     = (inquiry[58] & 0x80) > 0;
  mt_SnsMvmt                    = (inquiry[59] & 0x01) > 0;
  mt_PprMvmt                    = (inquiry[59] & 0x02) > 0;
  mt_AutoFdr                    = (inquiry[59] & 0x04) > 0;
  mt_HfCompr                    = (inquiry[59] & 0x10) > 0;
  mt_RdCompr                    = (inquiry[59] & 0x20) > 0;
  mt_18Set                      = (inquiry[59] & 0x40) > 0;
  mt_PxlSet                     = (inquiry[59] & 0x80) > 0;
  mt_DocSizeCode                = (int)inquiry[60];
  size_unit = INCH;
  switch (mt_DocSizeCode) {
  case 0x00 :
    doc_max_x =  8.5;  doc_max_y = 14.0;   break;
  case 0x01 :
    doc_max_x =  8.5;  doc_max_y = 11.0;   break;
  case 0x02 :
    doc_max_x =  8.5;  doc_max_y = 11.69;  break;
  case 0x03 :
    doc_max_x =  8.5;  doc_max_y = 13.0;   break;
  case 0x04 :
    doc_max_x =  8.0;  doc_max_y = 10.0;   break;
  case 0x05 :
    doc_max_x =  8.3;  doc_max_y = 14.0;   break;
  case 0x80 :
/* Slide format, size is mm */
    size_unit = MM;
    doc_max_x = 35.0;  doc_max_y = 35.0;   break;
  case 0x81 :
    doc_max_x =  5.0;  doc_max_y =  5.0;   break;
  case 0x82 :
/* Slide format, size is mm */
    size_unit = MM;
    doc_max_x = 36.0;  doc_max_y = 36.0;   break;
  default :
/* Undefined document format code */
    doc_max_x =  0.0;  doc_max_y =  0.0;
  }
  mt_ContrastSettings           = (int)((inquiry[61] & 0xf0) >> 4);
  mt_ExposureTimeSettings       = (int)(inquiry[61] & 0x0f);
  mt_ModelCode                  = (int)inquiry[62];
  mt_FWsupp                     = (inquiry[63] & 0x01) > 0;
  mt_SWslct                     = (inquiry[63] & 0x02) > 0;
  mt_FdrInst                    = (inquiry[63] & 0x04) > 0;
  mt_OutRdy                     = (inquiry[63] & 0x08) > 0;
  mt_TypLoc                     = (int)((inquiry[63] & 0x30) >> 4);
  mt_TxpInst                    = (inquiry[63] & 0x40) > 0;
  mt_ExpandedResolution         = (inquiry[64] == 0x01);
  mt_ShadowHighlightAdjustment  = (inquiry[65] & 0x01) > 0;
  mt_MidtoneAdjustment          = (inquiry[65] & 0x02) > 0;
  mt_MaxLookupTableSize         = (inquiry[66] & 0x01) ? 256 : 0;
  if (mt_MaxLookupTableSize) {
    if (inquiry[66] & 0x02) mt_MaxLookupTableSize = 1024;
    if (inquiry[66] & 0x04) mt_MaxLookupTableSize = 4096;
    if (inquiry[66] & 0x08) mt_MaxLookupTableSize = 65536;
  }
  switch (inquiry[66] >> 5) {
  case 0x00 :
    mt_GammaValueMax =   255;  mt_GammaEntryLength = 1;  break;
  case 0x01 :
    mt_GammaValueMax =  1023;  mt_GammaEntryLength = 2;  break;
  case 0x02 :
    mt_GammaValueMax =  4095;  mt_GammaEntryLength = 2;  break;
  case 0x03 :
    mt_GammaValueMax = 65535;  mt_GammaEntryLength = 2;  break;
  default :
    mt_GammaValueMax =     0;  mt_GammaEntryLength = 0;
  }
  mt_FastColorPrescan           = (inquiry[67] & 0x01) > 0;
  mt_DataTransferFmtSelect      = (inquiry[68] & 0x01) > 0;
  mt_ColorDataSequencing        = (int)(inquiry[69] & 0x7f);
  mt_3pass                      = (inquiry[69] & 0x80) > 0;
  mt_MODE_SELECT_1              = (inquiry[71] & 0x01) > 0;
  cntr_vals                     = (int)inquiry[72];
  min_cntr = -42;
  max_cntr =  49;
  if (cntr_vals) max_cntr = cntr_vals * 7 - 49;
  exp_vals                      = (int)inquiry[73];
  min_exp  = -18;
  max_exp  =  21;
  if (exp_vals)  max_exp  = exp_vals * 3 - 21;
  mt_4bpp_supp                  = (inquiry[74] & 0x01) > 0;
  mt_10bpp_supp                 = (inquiry[74] & 0x02) > 0;
  mt_12bpp_supp                 = (inquiry[74] & 0x04) > 0;
  mt_16bpp_supp                 = (inquiry[74] & 0x08) > 0;
  mt_BrightnessControl          = (inquiry[75] & 0x01) > 0;
  mt_DisableLinearizationTable  = (inquiry[75] & 0x02) > 0;
  mt_DisableRecalibration       = (inquiry[75] & 0x04) > 0;
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
/* int print_scanner_info (void)                                          */
/* Performs a get_scanner_info() call and prints the results in human     */
/* readable (long) format to stdout.                                      */
/* Return: RET_FAIL if get_scanner_info() call failed, RET_SUCCESS if     */
/*         successful.                                                    */
/*------------------------------------------------------------------------*/
int print_scanner_info (void) {
  if (get_scanner_info() != RET_SUCCESS) return(RET_FAIL);
  printf("Vendor/model: %s %s\nFirmware revision no. %s, device type %02x.\n",
         mt_VendorID, mt_ModelName, mt_RevisionNo, mt_DeviceType);
  printf("SCSI firmware version: %1d.%1d, scanner firmware version: %1d.%1d\n",
         mt_SCSI_firmware_ver_major,    mt_SCSI_firmware_ver_minor,
         mt_scanner_firmware_ver_minor, mt_scanner_firmware_ver_minor);
  printf("Response data format: 0x%02x\n", mt_response_data_format);
  printf("Resolution selection stepsize: %s%s\n", 
         mt_res_1percent ? "1% " : "", mt_res_5percent ? "5% " : "");
  printf("Supported scanning modes: %s%s%s%s%s%s\n",
         mt_LineArt ? "Lineart " : "",        mt_Hlftone ? "Halftone " : "",
         mt_MultBit ? "Multi-Bit " : "",      mt_Color ? "Color " : "",
         mt_Trnsmsv ? "Transparencies " : "", mt_Negtv ? "Negative" : "");
  printf("One-pass color scanning%s supported\n", mt_OnePass ? "" : " not");
  printf("%d built-in halftone patterns, pattern downloading%s supported\n",
         mt_Builtin_Patterns, mt_DnLoad ? "" : " not");
  printf("Scanner is%s%s type%s\n", 
         mt_SnsMvmt ? " flatbed" : "", mt_PprMvmt ? " edge feed" : "", 
         mt_AutoFdr ? ", ADF is supported" : "");
  printf("Huffman data compression: %s, Read data compression: %s\n",
         mt_HfCompr ? "yes" : "no", mt_RdCompr ? "yes" : "no");
  printf("Frame & paper length setting units:%s%s\n",
         mt_18Set ? " 1/8\" " : "", mt_PxlSet ? " pixels " : "");
  printf("Max. document size code is 0x%02x - ", mt_DocSizeCode);
  if (doc_max_x == 0.0) printf("unknown format\n");
  else printf("%2.2f x %2.2f %s\n", doc_max_x, doc_max_y, 
              unit_string[size_unit]);
  printf("%d contrast settings, %d exposure time settings\n",
         mt_ContrastSettings, mt_ExposureTimeSettings);
  printf("Model code : 0x%2x ", mt_ModelCode);
  switch (mt_ModelCode) {
  case 0x50 :
    printf("(ScanMaker II/IIXE)\n");  break;
  case 0x51 :
    printf("(ScanMaker 45t)\n");      break;
  case 0x52 :
    printf("(ScanMaker 35t)\n");      break;
  case 0x54 :
    printf("(ScanMaker IISP)\n");     break;
  case 0x55 :
    printf("(ScanMaker IIER)\n");     break;
  case 0x56 :
    printf("(ScanMaker A3t)\n");      break;
  case 0x57 :
    printf("(ScanMaker IIHR)\n");     break;
  case 0x58 :
    printf("(ScanMaker IIG)\n");      break;
  case 0x59 :
    printf("(ScanMaker III)\n");      break;
  case 0x5f :
    printf("(ScanMaker E3)\n");       break;
  case 0x63 :
    printf("(ScanMaker E6)\n");       break;
  default :
    printf("(unknown model code)\n"); break;
  }
  printf("F/W does%s support document feeder,\n", mt_FWsupp ? "" : " not");
  printf("F/W does%s support feeder/backtracking enable/disable\n",
         mt_SWslct ? "" : " not");
  printf("Feeder is%s installed %s\n", mt_FdrInst ? "" : " not",
         mt_FdrInst ? (mt_OutRdy ? "and ready" : "and out of paper") : "");
  switch (mt_TypLoc) {
  case DOC_ON_FLATBED :
    printf("Set to scan opaque document from flatbed\n");  break;
  case DOC_IN_FEEDER :
    printf("Set to scan opaque document in feeder\n");  break;
  case TRANSPARENCY :
    printf("Set to scan transparency\n");  break;
  default :
    printf("Type / Location code is unknown (0x%02x)\n", mt_TypLoc);
  }
  printf("Transparency illuminator is%s installed\n", mt_TxpInst ? "" : " not");
  printf("Expanded resolution range: %s\n", 
         mt_ExpandedResolution ? "yes" : "no");
  printf("Shadow/highlight adjustment%s supported, "
         "midtone adjustment%s supported\n",
         mt_ShadowHighlightAdjustment ? "" : " not",
         mt_MidtoneAdjustment ? "" : " not");
  printf("Gamma adjustment look-up table size: %d bytes\n", 
         mt_MaxLookupTableSize);
  printf("Max. value: %d (entry length %d byte%s)\n", mt_GammaValueMax,
         mt_GammaEntryLength, (mt_GammaEntryLength == 1) ? "" : "s");
  printf("Fast color prescan%s supported, ", 
         mt_FastColorPrescan ? "" : " not");
  printf("data transfer format select%s supported\n",
         mt_DataTransferFmtSelect ? "" : " not");
  printf("Color data sequencing: ");
  switch (mt_ColorDataSequencing) {
  case 0x00 :
    printf("Plane by plane (three-pass color scanner)\n");  break;
  case 0x01 :
    printf("Pixel by pixel\n");  break;
  case 0x02 :
    printf("Line by line, in R-G-B sequence, with no data headers\n");  break;
  case 0x03 :
    printf("Line by line, non-R-G-B sequence, with data headers\n");  break;
  default :
    printf("Unknown value (0x%02x)\n", mt_ColorDataSequencing);
  }
  if (mt_OnePass)
    printf("Three pass scanning%s supported\n", mt_3pass ? "" : " not");
  printf("MODE SELECT 1 and MODE SENSE 1 command%s supported\n", 
         mt_MODE_SELECT_1 ? "" : " not");
  printf("cntr_vals = %d, min_cntr = %d, max_cntr = %d\n",
         cntr_vals, min_cntr, max_cntr);
  printf("exp_vals = %d, min_exp = %d, max_exp = %d\n",
         exp_vals, min_exp, max_exp);
  printf("Supported multi-bit data formats: %s8 bpp  %s%s%s\n",
         mt_4bpp_supp ? "4 bpp  " : "", mt_10bpp_supp ? "10 bpp  " : "",
         mt_12bpp_supp ? "12 bpp  " : "", mt_16bpp_supp ? "16 bpp" : "");
  printf("Offset adjustment (digital brightness control)%s supported\n",
         mt_BrightnessControl ? "" : " not");
  printf("Linearization table can%s be disabled\n",
         mt_DisableLinearizationTable ? "" : " not");
  printf("Start-of-scan recalibration function can%s be disabled\n",
         mt_DisableRecalibration ? "" : " not");
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
/* int print_short_info (void)                                            */
/* Performs a get_scanner_info() call and prints the results in short     */
/* format to stdout.                                                      */
/* Return: RET_FAIL if get_scanner_info() call failed, RET_SUCCESS if     */
/*         successful.                                                    */
/*------------------------------------------------------------------------*/
int print_short_info (void) {
  int maxres = MAX_BASE_RESOLUTION;
  if (get_scanner_info() != RET_SUCCESS) return(RET_FAIL);
  if ((mt_ExpandedResolution) || (IGNORE_XRES_FLAG))
    maxres = MAX_EXPANDED_RESOLUTION;
  if (get_scanner_info() != RET_SUCCESS) return(RET_FAIL);
  printf("%2.2f %2.2f %d %d %d %s%s%s%s %s%s%s %s%s%s%s %s%s%s\n", 
         doc_max_x, doc_max_y, maxres, cntr_vals, exp_vals,
         mt_Hlftone                       ? "a"  : "",
         mt_LineArt                       ? "b"  : "",
         mt_Color                         ? "c"  : "",
         mt_MultBit                       ? "g"  : "",

         (mt_Trnsmsv || mt_TxpInst)       ? "t"  : "",
         (mt_Negtv || INVERT_USING_GAMMA) ? "n"  : "",
         mt_DisableRecalibration          ? "C"  : "",

         mt_ShadowHighlightAdjustment     ? "ls" : "",
         mt_MidtoneAdjustment             ? "m"  : "",
         mt_BrightnessControl             ? "d"  : "",
         (mt_MaxLookupTableSize > 0)      ? "GT" : "",

         mt_4bpp_supp                     ? "4"  : "",
         mt_10bpp_supp                    ? "1"  : "",
         mt_12bpp_supp                    ? "2"  : "");
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
/* void usage (void)                                                      */
/* Print short commant list to stdout                                     */
/*------------------------------------------------------------------------*/
void usage (void) {
  printf("\n"
"%s: User level driver for Microtek scanners v%s.\n"
"  Copyright (c) 1996, 1997  by Jan Schoenepauck / Fast Forward Productions\n"
#ifndef __EMX__
"\nUsage: %s [options] \n\n"
#else	/* OS/2 */
"\nUsage: %s <scanner scsi ID> [options] \n\n"
#endif
"Valid options:\n"
"   -a                      select halftone scan                           \n"
"   -b                      select line art (B/W) scan                     \n"
"   -c                      select color scan                              \n"
"   -g                      select grayscale scan                          \n"
"   -f <x1> <y1> <x2> <y2>  set scanning frame coordinates                 \n"
"   -r <n>                  set scanning resolution to <n> dpi             \n"
"   -o <file>               write to <file> instead of stdout              \n"
"   -t                      select transparency scanning                   \n"
"   -n                      reverse colors (negative)                      \n"
"   -p                      select prescan mode                            \n"
"   -C                      disable calibration (use with caution!)        \n"
"   -P                      same as -p -C                                  \n"
"   -d <n> [ <n> <n> ]      digital brightness adjustment (overall or      \n"
"                           individually for R/G/B, -100..100, default: 0) \n"
"   -k <n>                  contrast adjustment (-42..49, default: 0)      \n"
"   -e <n>                  exposure time (-18..21, default: 0)            \n"
"   -s <n>                  shadow adjustment (0..255, default: 0)         \n"
"   -l <n>                  highlight adjustment (0..255, default: 255)    \n"
"   -m <n>                  midtone adjustment (0..255, default: 128)      \n"
"   -G <n> [ <n> <n> ]      set overall or R/G/B gamma correction values   \n"
"   -T <file>               read gamma table from <file>                   \n"
"   -H <n>                  select halftone pattern (0..11)                \n"
"   -B                      disable backtracking                           \n"
"   -v <n>                  scanning velocity                              \n"
"   -L <n>                  set paper length to <n>                        \n"
"   -V                      verbose mode                                   \n"
"   -h                      this help screen                               \n"
"   -i                      scanner information (short)                    \n"
"   -I                      scanner information (long)                     \n"
"   -S                      perform scanner selftest and exit              \n"
"\n", myname, VERSION, myname);
}


/*------------------------------------------------------------------------*/
/* int getintarg (int *i, int arg, int argc, char **argv)                 */
/* Converts commandline argument at position <arg> to integer and stores  */
/* it in <*i>, with error checking.                                       */
/* Args: i           pointer to int where value is stored                 */
/*       arg         index of arg to convert                              */
/*       argc, argv  argument count and string pointer                    */
/* Return: RET_SUCCESS if conversion was successful, RET_FAIL if not      */
/*         (invalid argument string, or too few arguments)                */
/*------------------------------------------------------------------------*/
int getintarg (int *i, int arg, int argc, char **argv) {
  char *endptr;
  if (arg >= argc) return(RET_FAIL);
  *i = (int)strtol(argv[arg], &endptr, 10);
  if (*endptr != '\0') return(RET_FAIL);
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
/* int getfloatarg (float *f, int arg, int argc, char **argv)             */
/* Converts commandline argument at position <arg> to float and stores    */
/* it in <*f>, with error checking.                                       */
/* Args: f           pointer to float where value is stored               */
/*       arg         index of arg to convert                              */
/*       argc, argv  argument count and string pointer                    */
/* Return: RET_SUCCESS if conversion was successful, RET_FAIL if not      */
/*         (invalid argument string, or too few arguments)                */
/*------------------------------------------------------------------------*/
int getfloatarg (float *f, int arg, int argc, char **argv) {
  char *endptr;
  if (arg >= argc) return(RET_FAIL);
  *f = (float)strtod(argv[arg], &endptr);
  if (*endptr != '\0') return(RET_FAIL);
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
/* int getchararg (char *c, int arg, int argc, char **argv)               */
/* Converts commandline argument at position <arg> to char and stores     */
/* it in <*c>, with error checking.                                       */
/* Args: c           pointer to char where value is stored                */
/*       arg         index of arg to convert                              */
/*       argc, argv  argument count and string pointer                    */
/* Return: RET_SUCCESS if conversion was successful, RET_FAIL if not      */
/*         (invalid argument string, or too few arguments)                */
/*------------------------------------------------------------------------*/
int getchararg (char *c, int arg, int argc, char **argv) {
  char *endptr;
  if (arg >= argc) return(RET_FAIL);
  *c = (char)strtol(argv[arg], &endptr, 10);
  if (*endptr != '\0') return(RET_FAIL);
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
/* int getuchararg (unsigned char *u, int arg, int argc, char **argv)     */
/* Converts commandline argument at position <arg> to unsigned char and   */
/* stores it in <*u>, with error checking.                                */
/* Args: u           pointer to unsigned char where value is stored       */
/*       arg         index of arg to convert                              */
/*       argc, argv  argument count and string pointer                    */
/* Return: RET_SUCCESS if conversion was successful, RET_FAIL if not      */
/*         (invalid argument string, or too few arguments)                */
/*------------------------------------------------------------------------*/
int getuchararg (unsigned char *u, int arg, int argc, char **argv) {
  char *endptr;
  if (arg >= argc) return(RET_FAIL);
  *u = (unsigned char)strtol(argv[arg], &endptr, 10);
  if (*endptr != '\0') return(RET_FAIL);
  return(RET_SUCCESS);
}
  

/*------------------------------------------------------------------------*/
/* int checkopt (int argc, char **argv)                                   */
/* Checks the commandline options.                                        */
/* Args: <argc>, <argv>  Argument count and argument strings as received  */
/*                       by the main function                             */
/* Return: If an error occured, the negative number of the argument which */
/*         caused the error; otherwise RET_SUCCESS                        */
/*------------------------------------------------------------------------*/
int checkopt (int argc, char **argv) {
  int arg = 1;
  while (arg < argc) {
    if ((argv[arg][0] != '-') || (strlen(argv[arg]) > 2)) return(-arg);
    switch (argv[arg][1]) {
    case 'a' :
      scan_type = HALFTONE;
      break;
    case 'b' :
      scan_type = LINEART;
      break;
    case 'B' :
      allow_backtracking = 0;
      break;
    case 'c' :
      scan_type = COLOR;
      break;
    case 'C' :
      allow_calibration = 0;
      break;
    case 'd' :
      if (getchararg(&brightness_adjust_r, ++arg, argc, argv) != RET_SUCCESS)
        return(-argc);
      if (getchararg(&brightness_adjust_g, arg + 1, argc, argv) ==
          RET_SUCCESS) {
        arg++;
        if (getchararg(&brightness_adjust_b, ++arg, argc, argv) != RET_SUCCESS)
          return(-argc);
      }
      else {
        brightness_adjust_g = brightness_adjust_r;
        brightness_adjust_b = brightness_adjust_r;
      }
      break;
    case 'e' :
      if (getintarg(&exposure_time_adjust, ++arg, argc, argv) != RET_SUCCESS) 
        return(-argc);
      break;
    case 'f' :
      if (getfloatarg(&scan_frame_x1, ++arg, argc, argv) != RET_SUCCESS)
        return(-argc);
      if (getfloatarg(&scan_frame_y1, ++arg, argc, argv) != RET_SUCCESS)
        return(-argc);
      if (getfloatarg(&scan_frame_x2, ++arg, argc, argv) != RET_SUCCESS)
        return(-argc);
      if (getfloatarg(&scan_frame_y2, ++arg, argc, argv) != RET_SUCCESS)
        return(-argc);
      break;
    case 'g' :
      scan_type = GRAYSCALE;
      break;
    case 'G' :
      if (getfloatarg(&red_gamma, ++arg, argc, argv) != RET_SUCCESS)
        return(-argc);
      if (getfloatarg(&green_gamma, arg + 1, argc, argv) == RET_SUCCESS) {
        arg++;
        if (getfloatarg(&blue_gamma, ++arg, argc, argv) != RET_SUCCESS) 
          return(-argc);
      }
      else {
        green_gamma = red_gamma;
        blue_gamma = red_gamma;
      }
      break;
    case 'h' :
      usage();
      exit(RET_SUCCESS);
    case 'H' :
      if (getuchararg(&halftone_pattern, ++arg, argc, argv) != RET_SUCCESS)
        return(-argc);
      break;
    case 'i' :
      if (print_short_info() != RET_SUCCESS) {
        fprintf(stderr, "%s: Error executing INQUIRY command.\n", myname);
        exit(RET_FAIL);
      }
      exit(RET_SUCCESS);
    case 'I' :
      if (print_scanner_info() != RET_SUCCESS) {
        fprintf(stderr, "%s: Error executing INQUIRY command.\n", myname);
        exit(RET_FAIL);
      }
      exit(RET_SUCCESS);
    case 'k' :
      if (getintarg(&contrast_adjust, ++arg, argc, argv) != RET_SUCCESS)
        return(-argc);
      break;
    case 'l' :
      if (getuchararg(&highlight_adjust, ++arg, argc, argv) != RET_SUCCESS)
        return(-argc);
      break;
    case 'L' :
      if (getfloatarg(&paper_length, ++arg, argc, argv) != RET_SUCCESS)
        return(-argc);
      break;
    case 'm' :
      if (getuchararg(&midtone_adjust, ++arg, argc, argv) != RET_SUCCESS)
        return(-argc);
      break;
    case 'n' :
      reverse_colors = 1;
      break;
    case 'o' :
      if (++arg >= argc) return(-argc);
      output_filename = argv[arg];
      break;
    case 'P' :
      allow_calibration = 0;
    case 'p' :
      prescan = 1;
      break;
    case 'r' :
      if (getintarg(&scan_resolution, ++arg, argc, argv) != RET_SUCCESS)
        return(-argc);
      break;
    case 's' :
      if (getuchararg(&shadow_adjust, ++arg, argc, argv) != RET_SUCCESS)
        return(-argc);
      break;
    case 'S' :
      perform_selftest = 1;
      break;
    case 't' :
      transparency = 1;
      break;
    case 'T' :
      if (++arg >= argc) return(-argc);
      gammatable_filename = argv[arg];
      break;
    case 'v' :
      if (getuchararg(&scan_velocity, ++arg, argc, argv) != RET_SUCCESS)
        return(-argc);
      break;
    case 'V' :
      verbose = 1;
      break;
    default :
      return(-arg);
    }
    arg++;
  }
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
/* int test_options (void)                                                */
/* Check if the scan options selected are supported by the scanning       */
/* hardware. Must be called after the scan options have been inquired     */
/* with get_scanner_info().                                               */
/* Return: RET_SUCCESS if all options are supported, RET_FAIL if an       */
/*         illegal option was selected.                                   */
/*------------------------------------------------------------------------*/
int test_options (void) {
  if (mt_DeviceType != 0x06)
    ERROR_RETURN("No scanner found at %s.\n", scsi_device);
#ifdef VENDOR_STRING
  if (strncmp(mt_VendorID, VENDOR_STRING, 8) != 0)
    ERROR_RETURN("Scanner at %s has wrong vendor ID \"%s\".\n", scsi_device,
                 mt_VendorID);
#endif
  if ((set_unit == EIGHTHINCH) && (!mt_18Set))
    ERROR_RETURN("Setting unit 1/8\" not supported by scanner.\n");
  if ((set_unit == PIXEL) && (!mt_PxlSet))
    ERROR_RETURN("Setting unit pixel not supprted by scanner.\n");
  if ((scan_type == COLOR) && (!mt_Color))
    ERROR_RETURN("Color scanning not supported by scanner.\n");
  if ((scan_type == GRAYSCALE) && (!mt_MultBit))
    ERROR_RETURN("Multi-bit scanning not supported by scanner.\n");
  if ((scan_type == LINEART) && (!mt_LineArt))
    ERROR_RETURN("Line art scanning not supported by scanner.\n");
  if ((scan_type == HALFTONE) && (!mt_Hlftone))
    ERROR_RETURN("Halftone scanning not supported by scanner.\n");
  if (reverse_colors && (!mt_Negtv))
    ERROR_RETURN("Negative scanning not supported by scanner.\n");
  if ((scan_velocity < 0x01) || (scan_velocity > 0x07))
    ERROR_RETURN("Scanning velocity must be in the range 1..7.\n");
  if (halftone_pattern >= mt_Builtin_Patterns)
    ERROR_RETURN("Illegal halftone pattern number.\n");
  if ((transparency) && ((!mt_Trnsmsv) || (!mt_TxpInst)))
    ERROR_RETURN("Transparency scanning mode not available.\n");
  if ((!allow_backtracking) && (!mt_SWslct))
    ERROR_RETURN("Backtracking cannot be disabled.\n");
  if ((!allow_calibration) && (!mt_DisableRecalibration))
    ERROR_RETURN("Start of scan recalibration cannit be disabled.\n");
  if (((shadow_adjust != 0x00) || (highlight_adjust != 0xff)) &&
     (!mt_ShadowHighlightAdjustment))
    ERROR_RETURN("Scanner does not support shadow/highlight adjustment.\n");
  if ((midtone_adjust != 0x80) && (!mt_MidtoneAdjustment))
    ERROR_RETURN("Scanner does not support midtone adjustment.\n");
  if ((contrast_adjust < -42) || (contrast_adjust > max_cntr))
    ERROR_RETURN("Contrast adjustment must be in the range -42..%d.\n", 
                 max_cntr);
  if ((exposure_time_adjust < -18) || (exposure_time_adjust > max_exp))
    ERROR_RETURN("Exposure time adjustment must be in the range -18..%d.\n",
                 max_exp);
  if (((brightness_adjust_r != 0) || (brightness_adjust_g != 0) ||
       (brightness_adjust_b != 0)) && (!mt_BrightnessControl))
    ERROR_RETURN("Scanner does not support digital brightness adjustment.\n");
  if ((brightness_adjust_r < -100) || (brightness_adjust_r > 100) ||
      (brightness_adjust_g < -100) || (brightness_adjust_g > 100) ||
      (brightness_adjust_b < -100) || (brightness_adjust_b > 100))
    ERROR_RETURN("Brightness adjustment must be in the range -100..100.\n");
  if (((midtone_adjust != 0x80) || (shadow_adjust != 0x00) ||
     (highlight_adjust != 0xff)) &&
     ((scan_type == LINEART) || (scan_type == HALFTONE)))
    ERROR_RETURN("Shadow/highlight/midtone adjustment must not be specified\n"
                 "in single-bit modes.\n");
  return(RET_SUCCESS);
}
