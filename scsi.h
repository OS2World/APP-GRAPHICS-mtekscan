/*
   scsi.h

   Copyright (c) 1996, 1997 Jan Schoenepauck / Fast Forward Productions
   <schoenep@uni-wuppertal.de>

   $Id: scsi.h 1.2 1997/09/16 04:02:59 parent Exp $
*/

#ifndef _SCSI_H
#define _SCSI_H

#define SCSI_HDRLEN sizeof(struct sg_header)

#ifndef __EMX__
#ifdef SG_BIG_BUFF
#define SCSI_BUFSIZE (SG_BIG_BUFF)
#else
#define SCSI_BUFSIZE (4096 + SCSI_HDRLEN)
#endif
#else	/* OS/2 */
#define SCSI_BUFSIZE (32767L)
#endif

typedef unsigned char scsi_6byte_cmd[6];
typedef unsigned char scsi_10byte_cmd[10];
typedef unsigned char scsi_12byte_cmd[12];

extern int scsi_debug_level;
extern char *scsi_device;
extern int scsi_fd;
extern unsigned char scsi_cmd_buffer[];
extern unsigned int scsi_packet_id;
extern int scsi_memsize;
extern unsigned char scsi_sensebuffer[];

struct inquiry_data {
  char peripheral_qualifier;
  char peripheral_device_type;
  char RMB;
  char device_type_modifier;
  char ISO_version;
  char ECMA_version;
  char ANSI_version;
  char AENC;
  char TrmIOP;
  char response_data_format;
  char RelAdr;
  char WBus32;
  char WBus16;
  char Sync;
  char Linked;
  char CmdQue;
  char SftRe;
  char vendor[9];
  char model[17];
  char revision[5];
  char additional_inquiry;
};

extern int scsi_open_device (void);
extern void scsi_close_device (void);
extern int scsi_handle_cmd (unsigned char *scsi_cmd,   int scsi_cmd_len,
                            unsigned char *scsi_data,  int data_size,
                            unsigned char *scsi_reply, int reply_size);
extern int scsi_inquiry (struct inquiry_data *inq);
extern void scsi_print_inquiry_data (struct inquiry_data *inq);

#endif
