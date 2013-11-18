/*
   options.h
   
   Copyright (c) 1996, 1997 Jan Schoenepauck / Fast Forward Productions
   <schoenep@uni-wuppertal.de>

   $Id: options.h 1.1 1997/09/13 02:45:10 parent Exp $
*/

#ifndef _OPTIONS_H
#define _OPTIONS_H

extern int perform_selftest;
extern int scan_resolution;
extern int scan_type;
extern int reverse_colors;
extern int prescan;
extern int use_adf;
extern int transparency;
extern int allow_backtracking;
extern int allow_calibration;
extern int bits_per_color;
extern unsigned char scan_velocity;
extern unsigned char halftone_pattern;
extern int contrast_adjust;
extern int exposure_time_adjust;
extern char brightness_adjust_r;
extern char brightness_adjust_g;
extern char brightness_adjust_b;
extern unsigned char shadow_adjust;
extern unsigned char highlight_adjust;
extern unsigned char midtone_adjust;
extern float red_gamma;
extern float green_gamma;
extern float blue_gamma;
extern float scan_frame_x1;
extern float scan_frame_y1;
extern float scan_frame_x2;
extern float scan_frame_y2;
extern float paper_length;

extern int src_unit;
extern int set_unit;

extern char *output_filename;
extern char *gammatable_filename;

extern int verbose;

extern char unit_string[6][10];
extern float doc_max_x;
extern float doc_max_y;
extern int cntr_vals;
extern int min_cntr, max_cntr;
extern int exp_vals;
extern int min_exp, max_exp;

extern int mt_DeviceType;
extern int mt_SCSI_firmware_ver_major, mt_SCSI_firmware_ver_minor;
extern int mt_scanner_firmware_ver_major, mt_scanner_firmware_ver_minor;
extern int mt_response_data_format;
extern int mt_res_1percent, mt_res_5percent;
extern int mt_LineArt, mt_Hlftone, mt_MultBit, mt_Color;
extern int mt_Trnsmsv, mt_OnePass, mt_Negtv;
extern int mt_Builtin_Patterns, mt_DnLoad;
extern int mt_SnsMvmt, mt_PprMvmt, mt_AutoFdr, mt_HfCompr, mt_RdCompr;
extern int mt_18Set, mt_PxlSet;
extern int mt_DocSizeCode;
extern int mt_ContrastSettings, mt_ExposureTimeSettings;
extern int mt_ModelCode;
extern int mt_FWsupp, mt_SWslct, mt_FdrInst, mt_OutRdy, mt_TypLoc, mt_TxpInst;
extern int mt_ExpandedResolution;
extern int mt_ShadowHighlightAdjustment, mt_MidtoneAdjustment;
extern int mt_MaxLookupTableSize, mt_GammaValueMax, mt_GammaEntryLength;
extern int mt_FastColorPrescan, mt_DataTransferFmtSelect;
extern int mt_ColorDataSequencing;
extern int mt_3pass, mt_MODE_SELECT_1;
extern int mt_4bpp_supp, mt_10bpp_supp, mt_12bpp_supp, mt_16bpp_supp;
extern int mt_BrightnessControl;
extern int mt_DisableLinearizationTable, mt_DisableRecalibration;

extern char mt_VendorID[];
extern char mt_ModelName[];
extern char mt_RevisionNo[];

/* Function declarations */

extern int get_scanner_info (void);
extern int print_scanner_info (void);
extern int print_short_info (void);
extern void usage (void);
extern int checkopt (int argc, char **argv);
extern int test_options (void);

#endif
