//***************************************************************************
//*                                                                         *
//*  ASPI Router Library                                                    *
//*                                                                         *
//*  This is a sample library which shows how to send SRB's to the          *
//*  ASPI Router device driver. USE AT YOUR OWN RISK!!                      *
//*                                                                         *
//*  Version 1.01 - June 1997                                               *
//*                                                                         *
//*  Changes since 1.00:                                                    *
//*  abort(), AbortSRB added                                                *
//*                                                                         *
//***************************************************************************

#define      INCL_DOSFILEMGR
#define      INCL_DOS
#define      INCL_DOSDEVICES
#define      INCL_DOSDEVIOCTL
#define      INCL_DOSSEMAPHORES
#define      INCL_DOSMEMMGR
#include     <os2.h>
#include     "srb.h"
#include     <string.h>
#include     <stdio.h>

class scsiObj
{
  private:
    HEV         postSema;               // Event Semaphore for posting SRB completion
    HFILE       driver_handle;          // file handle for device driver
    BOOL        initSemaphore();
    BOOL        closeSemaphore();
    BOOL        openDriver();
    BOOL        closeDriver();
    BOOL        waitPost();
    BOOL        initBuffer();

  public:
    scsiObj();
    ~scsiObj();
    BOOL        init(ULONG bufsize);
    BOOL        close();
    ULONG       rewind(UCHAR id, UCHAR lun);
    ULONG       read(UCHAR id, UCHAR lun, ULONG transfer);
    ULONG       locate(UCHAR id, UCHAR lun, ULONG block);
    ULONG       unload(UCHAR id, UCHAR lun);
    ULONG       write(UCHAR id, UCHAR lun, ULONG transfer);
    ULONG       write_filemarks(UCHAR id, UCHAR lun, BOOL setmark, ULONG count);
    ULONG       space(UCHAR id, UCHAR lun, UCHAR code, ULONG count);
    ULONG       read_position(UCHAR id, UCHAR lun, ULONG* pos, ULONG* partition, BOOL* BOP, BOOL* EOP);
    ULONG       HA_inquiry(UCHAR ha);
    ULONG       getDeviceType(UCHAR id, UCHAR lun);
    ULONG       testUnitReady(UCHAR id, UCHAR lun);
    ULONG       resetDevice(UCHAR id, UCHAR lun);
    ULONG       abort();

    SRB         SRBlock;                // SCSI Request Block
    SRB         AbortSRB;               // Abort SRB
    PVOID       buffer;                 // Our data buffer
};
