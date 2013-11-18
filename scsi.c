/*
   scsi.c

   SCSI communication functions that work with the device supplied
   by the generic SCSI driver in the kernel.

   Copyright (c) 1996, 1997 Jan Schoenepauck / Fast Forward Productions
   <schoenep@uni-wuppertal.de>

   $Id: scsi.c 1.4 1997/09/27 07:17:28 parent Exp parent $
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#ifndef __EMX__
#include <sg.h>
#else
#define      INCL_DOSFILEMGR
#define      INCL_DOS
#define      INCL_DOSDEVICES
#define      INCL_DOSDEVIOCTL
#define      INCL_DOSSEMAPHORES
#define      INCL_DOSMEMMGR
#include     <os2.h>
#include "srb.h"
#endif

#include "scsi.h"
#include "config.h"
#include "global.h"

int scsi_debug_level = 0;
char *scsi_device = NULL;
int scsi_fd = -1;
unsigned int scsi_pack_id = 0;
unsigned char scsi_sensebuffer[16];

#ifdef __EMX__
int scsi_id;			// Scanner's SCSI ID #.
HEV postSema = 0;		// Event Semaphore for posting SRB completion
HFILE driver_handle = 0;	// file handle for device driver
PVOID buffer = 0;		// Big data buffer.
int scsi_memsize = SCSI_BUFSIZE;
#else

int scsi_memsize = 4096;

#endif


/* Some handy macros... */

#define SCSI_ERR(string...) { fprintf (stderr, ##string); return(RET_FAIL); }

#define SCSI_MSG(lvl,string...) { if (scsi_debug_level >= lvl) { fprintf \
(stderr, ##string); } }


/*------------------------------------------------------------------------*/
/* int block_sigint (void)                                                */
/* Disable handling of SIGINT.                                            */
/* Return: RET_SUCCESS if successful, RET_FAIL if there were errors.                      */
/*------------------------------------------------------------------------*/
int block_sigint (void) {
  sigset_t s;
  if (sigemptyset(&s) < 0) {
    perror("sigemptyset");
    return(RET_FAIL);
  }
  if (sigaddset(&s, SIGINT) < 0) {
    perror("sigaddset");
    return(RET_FAIL);
  }
  if (sigprocmask(SIG_BLOCK, &s, NULL) < 0) {
    perror("sigprocmask");
    return(RET_FAIL);
  }
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
/* int unblock_sigint (void)                                              */
/* Enable handling of SIGINT.                                             */
/* Return: RET_SUCCESS if successful, RET_FAIL if there were errors.                      */
/*------------------------------------------------------------------------*/
int unblock_sigint (void) {
  sigset_t s;
  if (sigemptyset(&s) < 0) {
    perror("sigemptyset");
    return(RET_FAIL);
  }
  if (sigaddset(&s, SIGINT) < 0) {
    perror("sigaddset");
    return(RET_FAIL);
  }
  if (sigprocmask(SIG_UNBLOCK, &s, NULL) < 0) {
    perror("sigprocmask");
    return(RET_FAIL);
  }
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
/* int scsi_open_device (void)                                            */
/* Opens the device specified in scsi_device. If open fails, an error     */
/* message is printed to stderr, and the function returns with RET_FAIL.  */
/* This happens if either the scsi_device was not set (NULL) or the       */
/* open() call failed, i.e. returned with a value < 0.                    */
/* If successful, returns the filedescriptor of the scsi device.          */
/*------------------------------------------------------------------------*/
#ifndef __EMX__
int scsi_open_device (void) {
#ifdef SG_BIG_BUFF
  int msize;
  scsi_6byte_cmd INQUIRY = { 0x12, 0x00, 0x00, 0x00, 0xff, 0x00 };
  union {
    unsigned char buf[SG_BIG_BUFF + SCSI_HDRLEN];
    struct sg_header hdr;
  } scsi_packet;
#endif
  if (!scsi_device) {             /* Return if no SCSI device is specified */
    fprintf(stderr, "scsi_open_device: no device specified\n");
    return(RET_FAIL);
  }
  scsi_fd = open(scsi_device, O_RDWR);
  if (scsi_fd < 0) {
    perror("scsi_open_device");
    return(RET_FAIL);
  }
#ifdef SG_BIG_BUFF
/* This is a rather brute-force approach to determine the actual buffer */
/* size, which is (or can be) _lower_ than SG_BIG_BUFF bytes. Don't     */
/* know why...                                                          */
  if (block_sigint() < 0)
    SCSI_ERR("scsi_open_device: Cannot block SIGINT.\n");
  for (msize = 4096; msize < SG_BIG_BUFF; msize += 512) {
    scsi_packet.hdr.pack_len  = 6 + SCSI_HDRLEN;
    scsi_packet.hdr.reply_len = msize;
/* Fix from Itai Nahshon <nahshon@almaden.ibm.com> */
/* was: memcpy(scsi_packet.buf, INQUIRY, 6);       */
    memcpy(scsi_packet.buf + SCSI_HDRLEN, INQUIRY, 6);
    if (write(scsi_fd, &scsi_packet, SCSI_HDRLEN + 6) < 0) break;
    read(scsi_fd, &scsi_packet, msize + SCSI_HDRLEN);
    scsi_memsize = msize;
  }
  if (unblock_sigint() < 0)
    SCSI_ERR("scsi_open_device: Cannot unblock SIGINT.\n");
#endif
  SCSI_MSG(3, "scsi_open_device: SCSI memory size is %d bytes.\n", 
           scsi_memsize);
  return(scsi_fd);
}

/*
 *	Close device.
 */

void scsi_close_device
	(
	)
	{
	if (scsi_fd != -1)
		close(scsi_fd);
	scsi_fd = -1;
	}
//-----------------------------------------------------------------------------
#else	/* OS/2 */
int scsi_open_device (void) {
  ULONG rc;                                             // return value
  ULONG ActionTaken;                                    // return value
  USHORT openSemaReturn;                                // return value
  USHORT lockSegmentReturn;                             // return value
  unsigned long cbreturn;
  unsigned long cbParam;


  if (!scsi_device) {             /* Return if no SCSI device is specified */
    fprintf(stderr, "scsi_open_device: no device specified\n");
    return(RET_FAIL);
  }
  scsi_id = atoi(scsi_device);
  if (scsi_id < 0 || scsi_id > 15) {
     fprintf(stderr, "scsi_open_device: bad SCSI ID %s\n", scsi_device);
     return (RET_FAIL);
  } /* endif */
  					// Allocate data buffer.
  rc = DosAllocMem(&buffer, SCSI_BUFSIZE, 
			OBJ_TILE | PAG_READ | PAG_WRITE | PAG_COMMIT);
  if (rc) {
     fprintf(stderr, "scsi_open_device: can't allocate memory\n");
     return (RET_FAIL);
  }
  rc = DosOpen((PSZ) "aspirou$",                        // open driver
               &driver_handle,
               &ActionTaken,
               0,
               0,
               FILE_OPEN,
               OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_READWRITE,
               NULL);
  if (rc) {                                 // opening failed -> return false
     fprintf(stderr, "scsi_open_device:  opening failed.\n");
     return (RET_FAIL);
  }
  rc = DosCreateEventSem(NULL, &postSema,   // create event semaphore
                         DC_SEM_SHARED, 0);
  if (rc) {                                 // DosCreateEventSem failed
     fprintf(stderr, "scsi_open_device:  couldn't create semaphore.\n");
     return (RET_FAIL);
  }
  rc = DosDevIOCtl(driver_handle, 0x92, 0x03,     // pass semaphore handle
                   (void*) &postSema, sizeof(HEV),      // to driver
                   &cbParam, (void*) &openSemaReturn,
                   sizeof(USHORT), &cbreturn);
  if (rc || openSemaReturn) {
     fprintf(stderr, "scsi_open_device:  couldn't set semaphore.\n");
     return (RET_FAIL);
  } /* endif */
						// Lock buffer.
  rc = DosDevIOCtl(driver_handle, 0x92, 0x04,           // pass buffer pointer
                   (void*) buffer, sizeof(PVOID),       // to driver
                   &cbParam, (void*) &lockSegmentReturn,
                   sizeof(USHORT), &cbreturn);
  if (rc || lockSegmentReturn) {                        // DosDevIOCtl failed
     fprintf(stderr, "scsi_open_device:  Can't lock buffer.\n");
     return (RET_FAIL);
  }
  return (1);
 }

/*
 *	Close driver and free everything.
 */

void scsi_close_device
	(
	)
	{
	if (postSema)
		DosCloseEventSem(postSema);	// Close event semaphore.
	postSema = 0;
	if (driver_handle)			// Close driver.
		DosClose(driver_handle);
	driver_handle = 0;
	if (buffer)				// Free buffer.
		DosFreeMem(buffer);
	buffer = 0;
	}

#endif	/* OS/2 */


/*------------------------------------------------------------------------*/
/* int scsi_handle_cmd ()                                                 */
/* Assembles the command packet to be sent to the SCSI device from the    */
/* specified SCSI command and data, writes it, and reads the reply. If a  */
/* reply size and buffer is specified, the reply data is copied into that */
/* buffer.                                                                */
/* The sense data is copied to scsi_sensebuffer.                          */
/* Return: RET_SUCCESS if successful, RET_FAIL if errors occured.         */
/*------------------------------------------------------------------------*/
#ifndef __EMX__
int scsi_handle_cmd (unsigned char *scsi_cmd,   int scsi_cmd_len,
                     unsigned char *scsi_data,  int data_size,
                     unsigned char *scsi_reply, int reply_size) {
  struct sg_header *scsi_hdr;
  unsigned char scsi_buffer[SCSI_BUFSIZE];
  int status = 0;
  int scsi_pack_len  = SCSI_HDRLEN + scsi_cmd_len + data_size;
  int scsi_reply_len = SCSI_HDRLEN + reply_size;
  
  if (scsi_cmd == NULL) 
    SCSI_ERR("scsi_handle_cmd: No command.\n");
  if ((scsi_cmd_len != 6) && (scsi_cmd_len != 10) && (scsi_cmd_len != 12))
    SCSI_ERR("scsi_handle_cmd: Illegal command length (%d).\n", scsi_cmd_len);
  if (data_size && (scsi_data == NULL)) 
    SCSI_ERR("scsi_handle_cmd: Input data expected.\n");
  if (reply_size && (scsi_reply == NULL)) 
    SCSI_ERR("scsi_handle_cmd: No space allocated for expected reply.\n");
  if (scsi_pack_len > SCSI_BUFSIZE)
    SCSI_ERR("scsi_handle_cmd: Packet length exceeds buffer size\n   "
             "              (buffer size %d bytes, packet length %d bytes).\n",
             SCSI_BUFSIZE, scsi_pack_len);
  if (scsi_reply_len > SCSI_BUFSIZE)
    SCSI_ERR("scsi_handle_cmd: Reply length exceeds buffer size\n   "
             "              (buffer size %d bytes, reply length %d bytes).\n",
             SCSI_BUFSIZE, scsi_reply_len);

  SCSI_MSG(5, "scsi_handle_cmd: Sending command, opcode 0x%02x.\n",
           scsi_cmd[0]);
  SCSI_MSG(5, "scsi_handle_cmd: Pack length = %d, expected reply size = %d\n",
           scsi_pack_len, scsi_reply_len);

  scsi_hdr = (struct sg_header *)scsi_buffer;
  scsi_hdr->pack_len    = scsi_pack_len;

/* The reply length should be set to the actual reply length expected, */
/* but this causes various strange errors (kernel panics or silent     */
/* system crashes) with my AVA-1502E adapter card; it works if the     */
/* value is set to the maximum value.                                  */
  
#if (SCSI_REPLYLEN_FIX)
  scsi_hdr->reply_len   = scsi_memsize + SCSI_HDRLEN;
#else
  scsi_hdr->reply_len   = scsi_reply_len;
#endif
  scsi_hdr->pack_id     = ++scsi_pack_id;
  scsi_hdr->twelve_byte = (scsi_cmd_len == 12);
  scsi_hdr->result      = 0;

/* copy command (and data, if available) to the scsi buffer (after */
/* the header data)                                                */
  memcpy(scsi_buffer+SCSI_HDRLEN, scsi_cmd, scsi_cmd_len);
  if (data_size)
    memcpy(scsi_buffer+SCSI_HDRLEN+scsi_cmd_len, scsi_data, data_size);
/* If SIGINT is caught between write and read, the device is */
/* blocked forever. So we block SIGINT instead.              */
  if (block_sigint() < 0)
    SCSI_ERR("scsi_handle_cmd: Cannot block SIGINT.\n");
  status = write(scsi_fd, scsi_buffer, scsi_pack_len);

  if (status < 0) {
    perror("scsi_handle_cmd");
    return(RET_FAIL);
  }
  if (status != scsi_pack_len)
    SCSI_ERR("scsi_handle_cmd: Write incomplete (%d of %d bytes written).\n",
             status, scsi_pack_len);
/* I never get a result other than 0 after a write(), so this */
/* is probably unneccessary                                   */
#if 1
  if (scsi_hdr->result)
    SCSI_ERR("scsi_handle_cmd: SCSI error (opcode 0x%02x, result 0x%02x).\n",
             scsi_cmd[0], scsi_hdr->result);  
#endif

  status = read(scsi_fd, scsi_buffer, scsi_reply_len);

/* Now we can allow SIGINT again. */
  if (unblock_sigint() < 0)
    SCSI_ERR("scsi_handle_cmd: Cannot unblock SIGINT.\n");

  SCSI_MSG(5, "scsi_handle_cmd: %d bytes read, result = 0x%02x.\n", 
           status, scsi_hdr->result);
  SCSI_MSG(4, "scsi_handle_cmd: Sense buffer [0] = 0x%02x\n",
          scsi_hdr->sense_buffer[0]);

  if (status < 0) {
    perror("scsi_handle_cmd");
    return(RET_FAIL);
  }
  if (status != scsi_reply_len) 
    SCSI_ERR("scsi_handle_cmd: Read error (%d bytes read, "
             "%d bytes expected).\n", status, scsi_reply_len);
  if (scsi_hdr->pack_id != scsi_pack_id)
    SCSI_ERR("scsi_handle_cmd: SCSI packet IDs do not match.\n");

  memcpy(scsi_sensebuffer, scsi_hdr->sense_buffer, 16);

  if (scsi_hdr->result || scsi_hdr->sense_buffer[0]) {

/* Sense data is only written to stderr if scsi_debug_level is > 0.  */
/* Otherwise, the function just returns RET_FAIL, and the calling    */
/* function is left to analyze the sense data in scsi_sensebuffer.   */
    if (scsi_debug_level > 0) {

      if (scsi_hdr->result)
        fprintf(stderr, "scsi_handle_cmd: %s\n", strerror(scsi_hdr->result));

      fprintf(stderr, "scsi_handle_cmd: error reading from SCSI device\n");
      fprintf(stderr, "                 SCSI opcode = 0x%02x   "
                      "result = 0x%02x   status = %d\n", scsi_cmd[0],
                      scsi_hdr->result, status);
      fprintf(stderr, "                 sense buffer: "
                      "%02x %02x %02x %02x %02x %02x %02x %02x "
                      "%02x %02x %02x %02x %02x %02x %02x %02x\n",
                      scsi_hdr->sense_buffer[0],  scsi_hdr->sense_buffer[1],
                      scsi_hdr->sense_buffer[2],  scsi_hdr->sense_buffer[3],
                      scsi_hdr->sense_buffer[4],  scsi_hdr->sense_buffer[5],
                      scsi_hdr->sense_buffer[6],  scsi_hdr->sense_buffer[7],
                      scsi_hdr->sense_buffer[8],  scsi_hdr->sense_buffer[9],
                      scsi_hdr->sense_buffer[10], scsi_hdr->sense_buffer[11],
                      scsi_hdr->sense_buffer[12], scsi_hdr->sense_buffer[13],
                      scsi_hdr->sense_buffer[14], scsi_hdr->sense_buffer[15]);
    }
    return(RET_FAIL);
  }
  if (scsi_reply) memcpy(scsi_reply, scsi_buffer+SCSI_HDRLEN, reply_size);
  return(RET_SUCCESS);
}
//-----------------------------------------------------------------------------
#else	/* OS/2 */
int scsi_handle_cmd (unsigned char *scsi_cmd,   int scsi_cmd_len,
                     unsigned char *scsi_data,  int data_size,
                     unsigned char *scsi_reply, int reply_size) {
  ULONG rc;                                     // return value
  unsigned long cbreturn;
  unsigned long cbParam;
  SRB SRBlock;                			// SCSI Request Block
  ULONG count=0;				// For semaphore.

  memset((char *) &SRBlock, 0, sizeof(SRBlock));// Okay, I'm paranoid.
  if (scsi_cmd == NULL) 
    SCSI_ERR("scsi_handle_cmd: No command.\n");
  if ((scsi_cmd_len != 6) && (scsi_cmd_len != 10) && (scsi_cmd_len != 12))
    SCSI_ERR("scsi_handle_cmd: Illegal command length (%d).\n", scsi_cmd_len);
  if (data_size && (scsi_data == NULL)) 
    SCSI_ERR("scsi_handle_cmd: Input data expected.\n");
  if (reply_size && (scsi_reply == NULL)) 
    SCSI_ERR("scsi_handle_cmd: No space allocated for expected reply.\n");
  if (data_size && reply_size)
    SCSI_ERR("scsi_handle_cmd: Can't do both input and output.\n");
  if (data_size > SCSI_BUFSIZE)
    SCSI_ERR("scsi_handle_cmd: Packet length exceeds buffer size\n   "
             "             (buffer size %ld bytes, packet length %d bytes).\n",
             SCSI_BUFSIZE, data_size);
  if (reply_size > SCSI_BUFSIZE)
    SCSI_ERR("scsi_handle_cmd: Reply length %d exceeds buffer size %ld\n",
					         reply_size, SCSI_BUFSIZE);

  SCSI_MSG(5, "scsi_handle_cmd: Sending command, opcode 0x%02x.\n",
           scsi_cmd[0]);
  SCSI_MSG(5, "scsi_handle_cmd: Pack length = %d, expected reply size = %d\n",
           data_size, reply_size);
  SRBlock.cmd=SRB_Command;                      // execute SCSI cmd
  SRBlock.ha_num=0;                             // host adapter number
  SRBlock.flags=SRB_Write | SRB_Post;           // posting enabled
  if (data_size)
	{					// Writing?  Copy data in.
	SRBlock.flags |= SRB_Write;
	memcpy(buffer, scsi_data, data_size);
	}
  else if (reply_size)
	SRBlock.flags |= SRB_Read;
  else
	SRBlock.flags |= SRB_NoTransfer;
  SRBlock.u.cmd.target=scsi_id;                 // Target SCSI ID
  SRBlock.u.cmd.lun=0;                          // Target SCSI LUN
					        // # of bytes transferred
  SRBlock.u.cmd.data_len=data_size ? data_size : reply_size;
  SRBlock.u.cmd.sense_len=16;                   // length of sense buffer
  SRBlock.u.cmd.data_ptr=NULL;                  // pointer to data buffer
  SRBlock.u.cmd.link_ptr=NULL;                  // pointer to next SRB
  SRBlock.u.cmd.cdb_len=scsi_cmd_len;           // SCSI command length
  memcpy(&SRBlock.u.cmd.cdb_st[0], scsi_cmd, scsi_cmd_len);
						// Do the command.
  rc = DosDevIOCtl(driver_handle, 0x92, 0x02, (void*) &SRBlock, 
		sizeof(SRB), &cbParam,
                  (void*) &SRBlock, sizeof(SRB), &cbreturn);

  if (rc) {
    perror("scsi_handle_cmd");
    return(RET_FAIL);
  }
  if (DosWaitEventSem(postSema, -1) ||           // wait forever for sema.
      DosResetEventSem(postSema, &count))        // reset semaphore
	SCSI_ERR("scsi_handle_cmd:  semaphore failure.\n");
					// Clear sense buffer.
  memset(scsi_sensebuffer, 0, sizeof(scsi_sensebuffer));
					// Get sense data if available.
  if ((SRBlock.status == SRB_Aborted || SRBlock.status == SRB_Error) &&
      SRBlock.u.cmd.target_status == SRB_CheckStatus)
	  memcpy(scsi_sensebuffer, &SRBlock.u.cmd.cdb_st[scsi_cmd_len], 
  						sizeof(scsi_sensebuffer));
  if (SRBlock.status != SRB_Done ||
      SRBlock.u.cmd.ha_status != SRB_NoError ||
      SRBlock.u.cmd.target_status != SRB_NoStatus)
	SCSI_ERR("scsi_handle_cmd:  command 0x%02x failed.\n",
							scsi_cmd[0]);
  if (reply_size)				// Reading?
	memcpy(scsi_reply, buffer, reply_size);
 return(RET_SUCCESS);
}
#endif	/* __EMX__ */

/*------------------------------------------------------------------------*/
/* int scsi_inquiry (struct inquiry_data *inq)                            */
/* Executes an INQUIRY command on the SCSI device and stores the data     */
/* returned in the specified inquiry_data structure.                      */
/* The inquiry data returned by the scanner is assumed to be in the       */
/* format specified by the SCSI-2 standard.                               */
/* Args: inq  - pointer to a struct inquiry_data where the data is to be  */
/*              stored.                                                   */
/* Return: RET_SUCCESS if successful, RET_FAIL if INQUIRY command failed. */
/*------------------------------------------------------------------------*/
int scsi_inquiry (struct inquiry_data *inq) {
  scsi_6byte_cmd INQUIRY = {0x12, 0x00, 0x00, 0x00, 0x60, 0x00};
  char inquiry[96];
  if (scsi_handle_cmd(INQUIRY, 6, NULL, 0, inquiry, 96) < 0) {
    fprintf(stderr, "scsi_inquiry: INQUIRY command failed\n");
    return(RET_FAIL);
  }
  inq->peripheral_qualifier   = (char)inquiry[0] >> 5;
  inq->peripheral_device_type = (char)inquiry[0] & 0x1f;
  inq->RMB = (inquiry[1] & 0x80) > 0;
  inq->device_type_modifier   = (char)inquiry[1] & 0x7f;
  inq->ISO_version            = (char)inquiry[2] >> 6;
  inq->ECMA_version           = ((char)inquiry[2] & 0x38) >> 3;
  inq->ANSI_version           = (char)inquiry[2] & 0x07;
  inq->AENC                   = (inquiry[3] & 0x80) > 0;
  inq->TrmIOP                 = (inquiry[3] & 0x40) > 0;
  inq->response_data_format   = (char)inquiry[3] & 0x0f;
  inq->RelAdr                 = (inquiry[7] & 0x80) > 0;
  inq->WBus32                 = (inquiry[7] & 0x40) > 0;
  inq->WBus16                 = (inquiry[7] & 0x20) > 0;
  inq->Sync                   = (inquiry[7] & 0x10) > 0;
  inq->Linked                 = (inquiry[7] & 0x08) > 0;
  inq->CmdQue                 = (inquiry[7] & 0x02) > 0;
  inq->SftRe                  = (inquiry[7] & 0x01) > 0;
  inq->additional_inquiry     = inquiry[4];
  strncpy(inq->vendor,   &inquiry[8],  8);
  strncpy(inq->model,    &inquiry[16], 16);
  strncpy(inq->revision, &inquiry[32], 4);
  inq->vendor[8]   = 0;
  inq->model[16]   = 0;
  inq->revision[4] = 0;
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
/* void scsi_print_inquiry_data (struct inquiry_data *inq)                */
/* Prints the inquiry data stored in the struct pointed to by <inq> to    */
/* stdout in a human-readable, "decrypted" fashion.                       */
/* Args: inq  - pointer to the struct inquiry_data which holds the data   */
/*              to be printed.                                            */
/*------------------------------------------------------------------------*/
void scsi_print_inquiry_data (struct inquiry_data *inq) {
  printf("Vendor: %s   Model: %s   Revision: %s\n", 
         inq->vendor, inq->model, inq->revision);
  printf("Peripheral Qualifier: 0x%02x    Peripheral Device Type: 0x%02x\n",
         inq->peripheral_qualifier, inq->peripheral_device_type);
  printf("%sRemovable medium\n",inq->RMB ? "" : "No ");
  printf("Device Type Modifier: 0x%02x\n", inq->device_type_modifier);
  printf("ISO Version: 0x%02x   ECMA Version: 0x%02x   "
         "ANSI-Approved Version: 0x%02x\n",
         inq->ISO_version, inq->ECMA_version, inq->ANSI_version);
  printf("(%s) AENC   (%s) TrmIOP\n", 
         inq->AENC ? "X" : "-", inq->TrmIOP ? "X" : "-");
  printf("Response Data Format: 0x%02x\n", inq->response_data_format);
  printf("(%s) RelAdr  (%s) WBus32  (%s) WBus16  (%s) Sync  "
         "(%s) Linked  (%s) CmdQue  (%s) SftRe\n", inq->RelAdr ? "X" : "-", 
         inq->WBus32 ? "X" : "-", inq->WBus16 ? "X" : "-", 
         inq->Sync ? "X" : "-",   inq->Linked ? "X" : "-", 
         inq->CmdQue ? "X" : "-", inq->SftRe ? "X" : "-");
}
