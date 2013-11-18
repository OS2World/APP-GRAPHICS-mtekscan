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
//*  abort() added                                                          *
//*                                                                         *
//***************************************************************************


#include "aspilib.h"


//***************************************************************************
//*                                                                         *
//*  scsiObj()                                                              *
//*                                                                         *
//*  Standard constructor                                                   *
//*                                                                         *
//***************************************************************************
scsiObj::scsiObj()
{
}


//***************************************************************************
//*                                                                         *
//*  ~scsiObj()                                                             *
//*                                                                         *
//*  Standard destructor                                                    *
//*                                                                         *
//***************************************************************************
scsiObj::~scsiObj()
{
}


//***************************************************************************
//*                                                                         *
//*  BOOL openDriver()                                                      *
//*                                                                         *
//*  Opens the ASPI Router device driver and sets device_handle.            *
//*  Returns:                                                               *
//*    TRUE - Success                                                       *
//*    FALSE - Unsuccessful opening of device driver                        *
//*                                                                         *
//*  Preconditions: ASPI Router driver has be loaded                        *
//*                                                                         *
//***************************************************************************
BOOL scsiObj::openDriver()
{
  ULONG rc;                                             // return value
  ULONG ActionTaken;                                    // return value

  rc = DosOpen((PSZ) "aspirou$",                        // open driver
               &driver_handle,
               &ActionTaken,
               0,
               0,
               FILE_OPEN,
               OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_READWRITE,
               NULL);
  if (rc) return FALSE;                                 // opening failed -> return false
  return TRUE;
}


//***************************************************************************
//*                                                                         *
//*  BOOL closeDriver()                                                     *
//*                                                                         *
//*  Closes the device driver                                               *
//*  Returns:                                                               *
//*    TRUE - Success                                                       *
//*    FALSE - Unsuccessful closing of device driver                        *
//*                                                                         *
//*  Preconditions: ASPI Router driver has be opened with openDriver        *
//*                                                                         *
//***************************************************************************
BOOL scsiObj::closeDriver()
{
  ULONG rc;                                             // return value

  rc = DosClose(driver_handle);
  if (rc) return FALSE;                                 // closing failed -> return false
  return TRUE;
}


//***************************************************************************
//*                                                                         *
//*  BOOL initSemaphore()                                                   *
//*                                                                         *
//*  Creates a new Event Semaphore and passes its handle to ASPI Router.    *
//*  Returns:                                                               *
//*    TRUE - Success                                                       *
//*    FALSE - Unsuccessful creation of event semaphore                     *
//*                                                                         *
//*  Preconditions: driver_handle has to be set with openDriver             *
//*                                                                         *
//***************************************************************************
BOOL scsiObj::initSemaphore()
{
  ULONG  rc;                                            // return value
  USHORT openSemaReturn;                                // return value
  unsigned long cbreturn;
  unsigned long cbParam;

  rc = DosCreateEventSem(NULL, &postSema,               // create event semaphore
                         DC_SEM_SHARED, 0);
  if (rc) return FALSE;                                 // DosCreateEventSem failed
  rc = DosDevIOCtl(driver_handle, 0x92, 0x03,           // pass semaphore handle
                   (void*) &postSema, sizeof(HEV),      // to driver
                   &cbParam, (void*) &openSemaReturn,
                   sizeof(USHORT), &cbreturn);
  if (rc) return FALSE;                                 // DosDevIOCtl failed
  if (openSemaReturn) return FALSE;                     // Driver could not open semaphore

  return TRUE;
}


//***************************************************************************
//*                                                                         *
//*  BOOL closeSemaphore()                                                  *
//*                                                                         *
//*  Closes the Event Semaphore                                             *
//*  Returns:                                                               *
//*    TRUE - Success                                                       *
//*    FALSE - Unsuccessful closing of event semaphore                      *
//*                                                                         *
//*  Preconditions: init_Semaphore has to be called successfully before     *
//*                                                                         *
//***************************************************************************
BOOL scsiObj::closeSemaphore()
{
  ULONG  rc;                                            // return value

  rc = DosCloseEventSem(postSema);                      // close event semaphore
  if (rc) return FALSE;                                 // DosCloseEventSem failed
  return TRUE;
}


//***************************************************************************
//*                                                                         *
//*  BOOL initBuffer()                                                      *
//*                                                                         *
//*  Sends the address of the data buffer to ASPI Router so that it can     *
//*  lock down the segment.                                                 *
//*  Returns:                                                               *
//*    TRUE - Success                                                       *
//*    FALSE - Unsuccessful locking of buffer segment                       *
//*                                                                         *
//*  Preconditions: (called from init())                                    *
//*                                                                         *
//***************************************************************************
BOOL scsiObj::initBuffer()
{
  ULONG  rc;                                            // return value
  USHORT lockSegmentReturn;                             // return value
  unsigned long cbreturn;
  unsigned long cbParam;

  rc = DosDevIOCtl(driver_handle, 0x92, 0x04,           // pass buffer pointer
                   (void*) buffer, sizeof(PVOID),       // to driver
                   &cbParam, (void*) &lockSegmentReturn,
                   sizeof(USHORT), &cbreturn);
  if (rc) return FALSE;                                 // DosDevIOCtl failed
  if (lockSegmentReturn) return FALSE;                  // Driver could not lock segment

  return TRUE;
}


//***************************************************************************
//*                                                                         *
//*  BOOL init(ULONG bufsize)                                               *
//*                                                                         *
//*  This inits the ASPI library and ASPI router driver.                    *
//*  Allocates the data buffer and passes its address to the driver         *
//*  Returns:                                                               *
//*    TRUE - Success                                                       *
//*    FALSE - Unsuccessful initialization of driver and library            *
//*                                                                         *
//*  Preconditions: ASPI router device driver has to be loaded              *
//*                                                                         *
//***************************************************************************
BOOL scsiObj::init(ULONG bufsize)
{
  BOOL  success;
  ULONG rc;

  rc = DosAllocMem(&buffer, bufsize, OBJ_TILE | PAG_READ | PAG_WRITE | PAG_COMMIT);
  if (rc) return FALSE;
  success=openDriver();                         // call openDriver member function
  if (!success) return FALSE;
  success=initSemaphore();                      // call initSemaphore member function
  if (!success) return FALSE;

  success=initBuffer();

  return TRUE;
}


//***************************************************************************
//*                                                                         *
//*  BOOL close()                                                           *
//*                                                                         *
//*  This closes the ASPI library and ASPI router driver and frees          *
//*  the memory allocated for the data buffer.
//*  Returns:                                                               *
//*    TRUE - Success                                                       *
//*    FALSE - Unsuccessful closing of library and driver                   *
//*                                                                         *
//*  Preconditions: init() should be called successfully before             *
//*                                                                         *
//***************************************************************************
BOOL scsiObj::close()
{
  BOOL success;
  ULONG rc;

  success=closeSemaphore();                     // call closeSemaphore member function
  if (!success)
  {
    printf("closeSemaphore() unsuccessful.\n");
    return FALSE;
  }
  success=closeDriver();                        // call closeDriver member function
  if (!success)
  {
    return FALSE;
    printf("closeDriver() unsucessful.\n");
  }
  rc = DosFreeMem(buffer);
  if (rc)
  {
    printf("DosFreeMem unsuccessful. return code: %ld\n", rc);
    return FALSE;
  }
  return TRUE;
}


//***************************************************************************
//*                                                                         *
//*  BOOL waitPost()                                                        *
//*                                                                         *
//*  Waits for postSema being posted by device driver                       *
//*  Returns:                                                               *
//*    TRUE - Success                                                       *
//*    FALSE - Unsuccessful access of event semaphore                       *
//*                                                                         *
//*  Preconditions: init() has to be called successfully before             *
//*                                                                         *
//***************************************************************************
BOOL scsiObj::waitPost()
{
  ULONG count=0;
  ULONG rc;                                             // return value

  rc = DosWaitEventSem(postSema, -1);                   // wait forever
  if (rc) return FALSE;                                 // DosWaitEventSem failed
  rc = DosResetEventSem(postSema, &count);              // reset semaphore
  if (rc) return FALSE;                                 // DosResetEventSem failed
  return TRUE;
}

//***************************************************************************
//*                                                                         *
//*  ULONG rewind(UCHAR id, UCHAR lun)                                      *
//*                                                                         *
//*  Sends a SRB containing a rewind command                                *
//*  Returns:                                                               *
//*    0  - Success                                                         *
//*    1  - DevIOCtl failed                                                 *
//*    2  - Semaphore access failure                                        *
//*    3  - SCSI command failed                                             *
//*                                                                         *
//*  Preconditions: init() has to be called successfully before             *
//*                                                                         *
//***************************************************************************
ULONG scsiObj::rewind(UCHAR id, UCHAR lun)
{
  ULONG rc;                                     // return value
  BOOL  success;                                // return value
  unsigned long cbreturn;
  unsigned long cbParam;

  SRBlock.cmd=SRB_Command;                      // execute SCSI cmd
  SRBlock.ha_num=0;                             // host adapter number
  SRBlock.flags=SRB_NoTransfer | SRB_Post;      // no data transfer, posting enabled
  SRBlock.u.cmd.target=id;                      // Target SCSI ID
  SRBlock.u.cmd.lun=lun;                        // Target SCSI LUN
  SRBlock.u.cmd.data_len=0;                     // # of bytes transferred
  SRBlock.u.cmd.sense_len=32;                   // length of sense buffer
  SRBlock.u.cmd.data_ptr=NULL;                  // pointer to data buffer
  SRBlock.u.cmd.link_ptr=NULL;                  // pointer to next SRB
  SRBlock.u.cmd.cdb_len=6;                      // SCSI command length
  SRBlock.u.cmd.cdb_st[0]=1;                    // rewind command
  SRBlock.u.cmd.cdb_st[1]=0;
  SRBlock.u.cmd.cdb_st[2]=0;
  SRBlock.u.cmd.cdb_st[3]=0;
  SRBlock.u.cmd.cdb_st[4]=0;
  SRBlock.u.cmd.cdb_st[5]=0;

  rc = DosDevIOCtl(driver_handle, 0x92, 0x02, (void*) &SRBlock, sizeof(SRB), &cbParam,
                  (void*) &SRBlock, sizeof(SRB), &cbreturn);
  if (rc)
    return 1;                                   // DosDevIOCtl failed
  else
  {
    success=waitPost();                         // wait for SRB being processed
    if (!success) return 2;                     // semaphore could not be accessed
  }
  if (SRBlock.status != SRB_Done) return 3;
  if (SRBlock.u.cmd.ha_status != SRB_NoError) return 3;
  if (SRBlock.u.cmd.target_status != SRB_NoStatus) return 3;
  return 0;
}


//***************************************************************************
//*                                                                         *
//*  ULONG read(UCHAR id, UCHAR lun, ULONG transfer)                        *
//*                                                                         *
//*  Sends a SRB containing a read command                                  *
//*  Returns:                                                               *
//*    0  - Success                                                         *
//*    1  - DevIOCtl failed                                                 *
//*    2  - Semaphore access failure                                        *
//*    3  - SCSI command failed                                             *
//*                                                                         *
//*  Preconditions: init() has to be called successfully before             *
//*                                                                         *
//***************************************************************************
ULONG scsiObj::read(UCHAR id, UCHAR lun, ULONG transfer)
{
  ULONG rc;                                     // return value
  BOOL  success;                                // return value
  unsigned long cbreturn;
  unsigned long cbParam;

  SRBlock.cmd=SRB_Command;                      // execute SCSI cmd
  SRBlock.ha_num=0;                             // host adapter number
  SRBlock.flags=SRB_Read | SRB_Post;            // data transfer, posting enabled
  SRBlock.u.cmd.target=id;                      // Target SCSI ID
  SRBlock.u.cmd.lun=lun;                        // Target SCSI LUN
  SRBlock.u.cmd.data_len=512*transfer;          // # of bytes transferred
  SRBlock.u.cmd.sense_len=32;                   // length of sense buffer
  SRBlock.u.cmd.data_ptr=NULL;                  // pointer to data buffer
  SRBlock.u.cmd.link_ptr=NULL;                  // pointer to next SRB
  SRBlock.u.cmd.cdb_len=6;                      // SCSI command length
  SRBlock.u.cmd.cdb_st[0]=0x08;                 // read command
  SRBlock.u.cmd.cdb_st[1]=1;                    // fixed length
  SRBlock.u.cmd.cdb_st[2]=(transfer >> 16) & 0xFF;  // transfer length MSB
  SRBlock.u.cmd.cdb_st[3]=(transfer >> 8) & 0xFF;   // transfer length
  SRBlock.u.cmd.cdb_st[4]=transfer & 0xFF;          // transfer length LSB
  SRBlock.u.cmd.cdb_st[5]=0;

  rc = DosDevIOCtl(driver_handle, 0x92, 0x02, (void*) &SRBlock, sizeof(SRB), &cbParam,
                  (void*) &SRBlock, sizeof(SRB), &cbreturn);
  if (rc)
    return 1;                                   // DosDevIOCtl failed
  else
  {
    success=waitPost();                         // wait for SRB being processed
    if (!success) return 2;                     // semaphore could not be accessed
  }
  if (SRBlock.status != SRB_Done) return 3;
  if (SRBlock.u.cmd.ha_status != SRB_NoError) return 3;
  if (SRBlock.u.cmd.target_status != SRB_NoStatus) return 3;
  return 0;
}


//***************************************************************************
//*                                                                         *
//*  ULONG locate(UCHAR id, UCHAR lun, ULONG block)                         *
//*                                                                         *
//*  Sends a SRB containing a locate command                                *
//*  Returns:                                                               *
//*    0  - Success                                                         *
//*    1  - DevIOCtl failed                                                 *
//*    2  - Semaphore access failure                                        *
//*    3  - SCSI command failed                                             *
//*                                                                         *
//*  Preconditions: init() has to be called successfully before             *
//*                                                                         *
//***************************************************************************
ULONG scsiObj::locate(UCHAR id, UCHAR lun, ULONG block)
{
  ULONG rc;                                     // return value
  BOOL  success;                                // return value
  unsigned long cbreturn;
  unsigned long cbParam;

  SRBlock.cmd=SRB_Command;                      // execute SCSI cmd
  SRBlock.ha_num=0;                             // host adapter number
  SRBlock.flags=SRB_NoTransfer | SRB_Post;      // no data transfer, posting enabled
  SRBlock.u.cmd.target=id;                      // Target SCSI ID
  SRBlock.u.cmd.lun=lun;                        // Target SCSI LUN
  SRBlock.u.cmd.data_len=0;                     // # of bytes transferred
  SRBlock.u.cmd.sense_len=32;                   // length of sense buffer
  SRBlock.u.cmd.data_ptr=NULL;                  // pointer to data buffer
  SRBlock.u.cmd.link_ptr=NULL;                  // pointer to next SRB
  SRBlock.u.cmd.cdb_len=10;                     // SCSI command length
  SRBlock.u.cmd.cdb_st[0]=0x2B;                 // locate command
  SRBlock.u.cmd.cdb_st[1]=0;
  SRBlock.u.cmd.cdb_st[2]=0;
  SRBlock.u.cmd.cdb_st[3]=(block >> 24);        // block MSB
  SRBlock.u.cmd.cdb_st[4]=(block >> 16) & 0xFF; // block
  SRBlock.u.cmd.cdb_st[5]=(block >> 8) & 0xFF;  // block
  SRBlock.u.cmd.cdb_st[6]=block & 0xFF;         // block LSB
  SRBlock.u.cmd.cdb_st[7]=0;
  SRBlock.u.cmd.cdb_st[8]=0;
  SRBlock.u.cmd.cdb_st[9]=0;

  rc = DosDevIOCtl(driver_handle, 0x92, 0x02, (void*) &SRBlock, sizeof(SRB), &cbParam,
                  (void*) &SRBlock, sizeof(SRB), &cbreturn);
  if (rc)
    return 1;                                   // DosDevIOCtl failed
  else
  {
    success=waitPost();                         // wait for SRB being processed
    if (!success) return 2;                     // semaphore could not be accessed
  }
  if (SRBlock.status != SRB_Done) return 3;
  if (SRBlock.u.cmd.ha_status != SRB_NoError) return 3;
  if (SRBlock.u.cmd.target_status != SRB_NoStatus) return 3;
  return 0;
}


//***************************************************************************
//*                                                                         *
//*  ULONG unload(UCHAR id, UCHAR lun)                                      *
//*                                                                         *
//*  Sends a SRB containing a unload command                                *
//*  Returns:                                                               *
//*    0  - Success                                                         *
//*    1  - DevIOCtl failed                                                 *
//*    2  - Semaphore access failure                                        *
//*    3  - SCSI command failed                                             *
//*                                                                         *
//*  Preconditions: init() has to be called successfully before             *
//*                                                                         *
//***************************************************************************
ULONG scsiObj::unload(UCHAR id, UCHAR lun)
{
  ULONG rc;                                     // return value
  BOOL  success;                                // return value
  unsigned long cbreturn;
  unsigned long cbParam;

  SRBlock.cmd=SRB_Command;                      // execute SCSI cmd
  SRBlock.ha_num=0;                             // host adapter number
  SRBlock.flags=SRB_NoTransfer | SRB_Post;      // no data transfer, posting enabled
  SRBlock.u.cmd.target=id;                      // Target SCSI ID
  SRBlock.u.cmd.lun=lun;                        // Target SCSI LUN
  SRBlock.u.cmd.data_len=0;                     // # of bytes transferred
  SRBlock.u.cmd.sense_len=32;                   // length of sense buffer
  SRBlock.u.cmd.data_ptr=NULL;                  // pointer to data buffer
  SRBlock.u.cmd.link_ptr=NULL;                  // pointer to next SRB
  SRBlock.u.cmd.cdb_len=6;                      // SCSI command length
  SRBlock.u.cmd.cdb_st[0]=0x1B;                 // unload command
  SRBlock.u.cmd.cdb_st[1]=0;
  SRBlock.u.cmd.cdb_st[2]=0;
  SRBlock.u.cmd.cdb_st[3]=0;
  SRBlock.u.cmd.cdb_st[4]=0;
  SRBlock.u.cmd.cdb_st[5]=0;

  rc = DosDevIOCtl(driver_handle, 0x92, 0x02, (void*) &SRBlock, sizeof(SRB), &cbParam,
                  (void*) &SRBlock, sizeof(SRB), &cbreturn);
  if (rc)
    return 1;                                   // DosDevIOCtl failed
  else
  {
    success=waitPost();                         // wait for SRB being processed
    if (!success) return 2;                     // semaphore could not be accessed
  }
  if (SRBlock.status != SRB_Done) return 3;
  if (SRBlock.u.cmd.ha_status != SRB_NoError) return 3;
  if (SRBlock.u.cmd.target_status != SRB_NoStatus) return 3;
  return 0;
}


//***************************************************************************
//*                                                                         *
//*  ULONG read_position(UCHAR id, UCHAR lun, ULONG* pos, ULONG* partition, *
//*                      BOOL* BOP, BOOL* EOP)                              *
//*                                                                         *
//*  Sends a SRB containing a read_position command                         *
//*  Returns:                                                               *
//*    0  - Success                                                         *
//*    1  - DevIOCtl failed                                                 *
//*    2  - Semaphore access failure                                        *
//*    3  - SCSI command failed                                             *
//*                                                                         *
//*  Preconditions: init() has to be called successfully before             *
//*                                                                         *
//***************************************************************************
ULONG scsiObj::read_position(UCHAR id, UCHAR lun, ULONG* pos, ULONG* partition,
                             BOOL* BOP, BOOL* EOP)
{
  ULONG rc;                                     // return value
  BOOL  success;                                // return value
  unsigned long cbreturn;
  unsigned long cbParam;
  UCHAR* p;

  SRBlock.cmd=SRB_Command;                      // execute SCSI cmd
  SRBlock.ha_num=0;                             // host adapter number
  SRBlock.flags=SRB_Read | SRB_Post;            // data transfer, posting enabled
  SRBlock.u.cmd.target=id;                      // Target SCSI ID
  SRBlock.u.cmd.lun=lun;                        // Target SCSI LUN
  SRBlock.u.cmd.data_len=20;                    // # of bytes transferred
  SRBlock.u.cmd.sense_len=32;                   // length of sense buffer
  SRBlock.u.cmd.data_ptr=NULL;                  // pointer to data buffer
  SRBlock.u.cmd.link_ptr=NULL;                  // pointer to next SRB
  SRBlock.u.cmd.cdb_len=10;                     // SCSI command length
  SRBlock.u.cmd.cdb_st[0]=0x34;                 // read position command
  SRBlock.u.cmd.cdb_st[1]=0;
  SRBlock.u.cmd.cdb_st[2]=0;
  SRBlock.u.cmd.cdb_st[3]=0;
  SRBlock.u.cmd.cdb_st[4]=0;
  SRBlock.u.cmd.cdb_st[5]=0;
  SRBlock.u.cmd.cdb_st[6]=0;
  SRBlock.u.cmd.cdb_st[7]=0;
  SRBlock.u.cmd.cdb_st[8]=0;
  SRBlock.u.cmd.cdb_st[9]=0;

  rc = DosDevIOCtl(driver_handle, 0x92, 0x02, (void*) &SRBlock, sizeof(SRB), &cbParam,
                  (void*) &SRBlock, sizeof(SRB), &cbreturn);
  if (rc)
    return 1;                                   // DosDevIOCtl failed
  else
  {
    success=waitPost();                         // wait for SRB being processed
    if (!success) return 2;                     // semaphore could not be accessed
  }
  if (SRBlock.status != SRB_Done) return 3;
  if (SRBlock.u.cmd.ha_status != SRB_NoError) return 3;
  if (SRBlock.u.cmd.target_status != SRB_NoStatus) return 3;

  *pos=0;
  p=(UCHAR*)buffer;
  *BOP=((*p & 0x80) == 0x80);
  *EOP=((*p & 0x40) == 0x40);
  *partition=0;
  p+=1;
  *partition=*p;
  p+=3;
  *pos+=*p++ << 24;
  *pos+=*p++ << 16;
  *pos+=*p++ << 8;
  *pos+=*p++;
  return 0;
}


//***************************************************************************
//*                                                                         *
//*  ULONG space(UCHAR id, UCHAR lun, UCHAR code, ULONG count)              *
//*                                                                         *
//*  Sends a SRB containing a space command                                 *
//*  Returns:                                                               *
//*    0  - Success                                                         *
//*    1  - DevIOCtl failed                                                 *
//*    2  - Semaphore access failure                                        *
//*    3  - SCSI command failed                                             *
//*                                                                         *
//*  Preconditions: init() has to be called successfully before             *
//*                                                                         *
//***************************************************************************
ULONG scsiObj::space(UCHAR id, UCHAR lun, UCHAR code, ULONG count)
{
  ULONG rc;                                     // return value
  BOOL  success;                                // return value
  unsigned long cbreturn;
  unsigned long cbParam;

  SRBlock.cmd=SRB_Command;                      // execute SCSI cmd
  SRBlock.ha_num=0;                             // host adapter number
  SRBlock.flags=SRB_NoTransfer | SRB_Post;      // no data transfer, posting enabled
  SRBlock.u.cmd.target=id;                      // Target SCSI ID
  SRBlock.u.cmd.lun=lun;                        // Target SCSI LUN
  SRBlock.u.cmd.data_len=0;                     // # of bytes transferred
  SRBlock.u.cmd.sense_len=32;                   // length of sense buffer
  SRBlock.u.cmd.data_ptr=NULL;                  // pointer to data buffer
  SRBlock.u.cmd.link_ptr=NULL;                  // pointer to next SRB
  SRBlock.u.cmd.cdb_len=6;                      // SCSI command length
  SRBlock.u.cmd.cdb_st[0]=0x11;                 // space command
  SRBlock.u.cmd.cdb_st[1]=code & 0x7;           // code
  SRBlock.u.cmd.cdb_st[2]=(count >> 16) & 0xFF; // count MSB
  SRBlock.u.cmd.cdb_st[3]=(count >> 8) & 0xFF;  // count
  SRBlock.u.cmd.cdb_st[4]=count & 0xFF;         // count LSB
  SRBlock.u.cmd.cdb_st[5]=0;

  rc = DosDevIOCtl(driver_handle, 0x92, 0x02, (void*) &SRBlock, sizeof(SRB), &cbParam,
                  (void*) &SRBlock, sizeof(SRB), &cbreturn);
  if (rc)
    return 1;                                   // DosDevIOCtl failed
  else
  {
    success=waitPost();                         // wait for SRB being processed
    if (!success) return 2;                     // semaphore could not be accessed
  }
  if (SRBlock.status != SRB_Done) return 3;
  if (SRBlock.u.cmd.ha_status != SRB_NoError) return 3;
  if (SRBlock.u.cmd.target_status != SRB_NoStatus) return 3;
  return 0;
}


//***************************************************************************
//*                                                                         *
//*  ULONG testUnitReady(UCHAR id, UCHAR lun)                               *
//*                                                                         *
//*  Sends a SRB containing a test unit ready command                       *
//*  Returns:                                                               *
//*    0  - Success                                                         *
//*    1  - DevIOCtl failed                                                 *
//*    2  - Semaphore access failure                                        *
//*    3  - SCSI command failed                                             *
//*                                                                         *
//*  Preconditions: init() has to be called successfully before             *
//*                                                                         *
//***************************************************************************
ULONG scsiObj::testUnitReady(UCHAR id, UCHAR lun)
{
  ULONG rc;                                     // return value
  BOOL  success;                                // return value
  unsigned long cbreturn;
  unsigned long cbParam;

  SRBlock.cmd=SRB_Command;                      // execute SCSI cmd
  SRBlock.ha_num=0;                             // host adapter number
  SRBlock.flags=SRB_NoTransfer | SRB_Post;      // no data transfer, posting enabled
  SRBlock.u.cmd.target=id;                      // Target SCSI ID
  SRBlock.u.cmd.lun=lun;                        // Target SCSI LUN
  SRBlock.u.cmd.data_len=0;                     // # of bytes transferred
  SRBlock.u.cmd.sense_len=32;                   // length of sense buffer
  SRBlock.u.cmd.data_ptr=NULL;                  // pointer to data buffer
  SRBlock.u.cmd.link_ptr=NULL;                  // pointer to next SRB
  SRBlock.u.cmd.cdb_len=6;                      // SCSI command length
  SRBlock.u.cmd.cdb_st[0]=0x0;                  // test unit ready command
  SRBlock.u.cmd.cdb_st[1]=(lun << 5);           // lun
  SRBlock.u.cmd.cdb_st[2]=0;
  SRBlock.u.cmd.cdb_st[3]=0;
  SRBlock.u.cmd.cdb_st[4]=0;
  SRBlock.u.cmd.cdb_st[5]=0;

  rc = DosDevIOCtl(driver_handle, 0x92, 0x02, (void*) &SRBlock, sizeof(SRB), &cbParam,
                  (void*) &SRBlock, sizeof(SRB), &cbreturn);
  if (rc)
    return 1;                                   // DosDevIOCtl failed
  else
  {
    success=waitPost();                         // wait for SRB being processed
    if (!success) return 2;                     // semaphore could not be accessed
  }
  if (SRBlock.status != SRB_Done) return 3;
  if (SRBlock.u.cmd.ha_status != SRB_NoError) return 3;
  if (SRBlock.u.cmd.target_status != SRB_NoStatus) return 3;
  return 0;
}


//***************************************************************************
//*                                                                         *
//*  ULONG write(UCHAR id, UCHAR lun, ULONG transfer)                       *
//*                                                                         *
//*  Sends a SRB containing a write command                                 *
//*  Returns:                                                               *
//*    0  - Success                                                         *
//*    1  - DevIOCtl failed                                                 *
//*    2  - Semaphore access failure                                        *
//*    3  - SCSI command failed                                             *
//*                                                                         *
//*  Preconditions: init() has to be called successfully before             *
//*                                                                         *
//***************************************************************************
ULONG scsiObj::write(UCHAR id, UCHAR lun, ULONG transfer)
{
  ULONG rc;                                     // return value
  BOOL  success;                                // return value
  unsigned long cbreturn;
  unsigned long cbParam;

  SRBlock.cmd=SRB_Command;                      // execute SCSI cmd
  SRBlock.ha_num=0;                             // host adapter number
  SRBlock.flags=SRB_Write | SRB_Post;           // data transfer, posting enabled
  SRBlock.u.cmd.target=id;                      // Target SCSI ID
  SRBlock.u.cmd.lun=lun;                        // Target SCSI LUN
  SRBlock.u.cmd.data_len=512*transfer;          // # of bytes transferred
  SRBlock.u.cmd.sense_len=32;                   // length of sense buffer
  SRBlock.u.cmd.data_ptr=NULL;                  // pointer to data buffer
  SRBlock.u.cmd.link_ptr=NULL;                  // pointer to next SRB
  SRBlock.u.cmd.cdb_len=6;                      // SCSI command length
  SRBlock.u.cmd.cdb_st[0]=0xA;                  // write command
  SRBlock.u.cmd.cdb_st[1]=1;                    // fixed length
  SRBlock.u.cmd.cdb_st[2]=(transfer >> 16) & 0xFF; // transfer length MSB
  SRBlock.u.cmd.cdb_st[3]=(transfer >> 8) & 0xFF;  // transfer length
  SRBlock.u.cmd.cdb_st[4]=(transfer & 0xFF);       // transfer length LSB
  SRBlock.u.cmd.cdb_st[5]=0;

  rc = DosDevIOCtl(driver_handle, 0x92, 0x02, (void*) &SRBlock, sizeof(SRB), &cbParam,
                  (void*) &SRBlock, sizeof(SRB), &cbreturn);
  if (rc)
    return 1;                                   // DosDevIOCtl failed
  else
  {
    success=waitPost();                         // wait for SRB being processed
    if (!success) return 2;                     // semaphore could not be accessed
  }
  if (SRBlock.status != SRB_Done) return 3;
  if (SRBlock.u.cmd.ha_status != SRB_NoError) return 3;
  if (SRBlock.u.cmd.target_status != SRB_NoStatus) return 3;
  return 0;
}


//***************************************************************************
//*                                                                         *
//*  ULONG write_filemarks(UCHAR id, UCHAR lun, BOOL setmark, ULONG count)  *
//*                                                                         *
//*  Sends a SRB containing a unload command                                *
//*  Returns:                                                               *
//*    0  - Success                                                         *
//*    1  - DevIOCtl failed                                                 *
//*    2  - Semaphore access failure                                        *
//*    3  - SCSI command failed                                             *
//*                                                                         *
//*  Preconditions: init() has to be called successfully before             *
//*                                                                         *
//***************************************************************************
ULONG scsiObj::write_filemarks(UCHAR id, UCHAR lun, BOOL setmark, ULONG count)
{
  ULONG rc;                                     // return value
  BOOL  success;                                // return value
  unsigned long cbreturn;
  unsigned long cbParam;

  SRBlock.cmd=SRB_Command;                      // execute SCSI cmd
  SRBlock.ha_num=0;                             // host adapter number
  SRBlock.flags=SRB_NoTransfer | SRB_Post;      // no data transfer, posting enabled
  SRBlock.u.cmd.target=id;                      // Target SCSI ID
  SRBlock.u.cmd.lun=lun;                        // Target SCSI LUN
  SRBlock.u.cmd.data_len=0;                     // # of bytes transferred
  SRBlock.u.cmd.sense_len=32;                   // length of sense buffer
  SRBlock.u.cmd.data_ptr=NULL;                  // pointer to data buffer
  SRBlock.u.cmd.link_ptr=NULL;                  // pointer to next SRB
  SRBlock.u.cmd.cdb_len=6;                      // SCSI command length
  SRBlock.u.cmd.cdb_st[0]=0x10;                 // write filemarks command
  if (setmark)
    SRBlock.u.cmd.cdb_st[1]=2;                  // write setmark(s) instead of filemark(s)
  else
    SRBlock.u.cmd.cdb_st[1]=0;                  // write filemark(s)
  SRBlock.u.cmd.cdb_st[2]=(count >> 16) & 0xFF; // count MSB
  SRBlock.u.cmd.cdb_st[3]=(count >> 8) & 0xFF;  // count
  SRBlock.u.cmd.cdb_st[4]=count & 0xFF;         // count LSB
  SRBlock.u.cmd.cdb_st[5]=0;

  rc = DosDevIOCtl(driver_handle, 0x92, 0x02, (void*) &SRBlock, sizeof(SRB), &cbParam,
                  (void*) &SRBlock, sizeof(SRB), &cbreturn);
  if (rc)
    return 1;                                   // DosDevIOCtl failed
  else
  {
    success=waitPost();                         // wait for SRB being processed
    if (!success) return 2;                     // semaphore could not be accessed
  }
  if (SRBlock.status != SRB_Done) return 3;
  if (SRBlock.u.cmd.ha_status != SRB_NoError) return 3;
  if (SRBlock.u.cmd.target_status != SRB_NoStatus) return 3;
  return 0;
}


//***************************************************************************
//*                                                                         *
//*  ULONG HA_inquiry(UCHAR ha)                                             *
//*                                                                         *
//*  Sends a SRB containing a Host Adapter Inquiry command                  *
//*  Returns:                                                               *
//*    0  - Success                                                         *
//*    1  - DevIOCtl failed                                                 *
//*    2  - Host Adapter not installed                                      *
//*                                                                         *
//*  Preconditions: driver has to be opened                                 *
//*                                                                         *
//***************************************************************************
ULONG scsiObj::HA_inquiry(UCHAR ha)
{
  ULONG rc;                                     // return value
  unsigned long cbreturn;
  unsigned long cbParam;

  SRBlock.cmd=SRB_Inquiry;                      // host adapter inquiry
  SRBlock.ha_num=ha;                            // host adapter number
  SRBlock.flags=0;                              // no flags set

  rc = DosDevIOCtl(driver_handle, 0x92, 0x02, (void*) &SRBlock, sizeof(SRB), &cbParam,
                  (void*) &SRBlock, sizeof(SRB), &cbreturn);
  if (rc)
    return 1;                                   // DosDevIOCtl failed
  if (SRBlock.status != SRB_Done) return 2;
  return 0;
}


//***************************************************************************
//*                                                                         *
//*  ULONG getDeviceType(UCHAR id, UCHAR lun)                               *
//*                                                                         *
//*  Sends a SRB containing a Get Device Type command                       *
//*  Returns:                                                               *
//*    0  - Success                                                         *
//*    1  - DevIOCtl failed                                                 *
//*    2  - Device not installed                                            *
//*                                                                         *
//*  Preconditions: driver has to be opened                                 *
//*                                                                         *
//***************************************************************************
ULONG scsiObj::getDeviceType(UCHAR id, UCHAR lun)
{
  ULONG rc;                                     // return value
  unsigned long cbreturn;
  unsigned long cbParam;

  SRBlock.cmd=SRB_Device;                       // get device type
  SRBlock.ha_num=0;                             // host adapter number
  SRBlock.flags=0;                              // no flags set
  SRBlock.u.dev.target=id;                      // target id
  SRBlock.u.dev.lun=lun;                        // target LUN

  rc = DosDevIOCtl(driver_handle, 0x92, 0x02, (void*) &SRBlock, sizeof(SRB), &cbParam,
                  (void*) &SRBlock, sizeof(SRB), &cbreturn);
  if (rc)
    return 1;                                   // DosDevIOCtl failed
  if (SRBlock.status != SRB_Done) return 2;
  return 0;
}


//***************************************************************************
//*                                                                         *
//*  ULONG resetDevice(UCHAR id, UCHAR lun)                                 *
//*                                                                         *
//*  Sends a SRB containing a Reset Device command                          *
//*  Returns:                                                               *
//*    0  - Success                                                         *
//*    1  - DevIOCtl failed                                                 *
//*    2  - Semaphore access failure                                        *
//*    3  - SCSI command failed                                             *
//*                                                                         *
//*  Preconditions: init() has to be called successfully before             *
//*                                                                         *
//***************************************************************************
ULONG scsiObj::resetDevice(UCHAR id, UCHAR lun)
{
  ULONG rc;                                     // return value
  unsigned long cbreturn;
  unsigned long cbParam;
  BOOL  success;

  SRBlock.cmd=SRB_Reset;                        // reset device
  SRBlock.ha_num=0;                             // host adapter number
  SRBlock.flags=SRB_Post;                       // posting enabled
  SRBlock.u.res.target=id;                      // target id
  SRBlock.u.res.lun=lun;                        // target LUN

  rc = DosDevIOCtl(driver_handle, 0x92, 0x02, (void*) &SRBlock, sizeof(SRB), &cbParam,
                  (void*) &SRBlock, sizeof(SRB), &cbreturn);
  if (rc)
    return 1;                                   // DosDevIOCtl failed
  else
  {
    success=waitPost();                         // wait for SRB being processed
    if (!success) return 2;                     // semaphore could not be accessed
  }
  if (SRBlock.status != SRB_Done) return 3;
  return 0;
}


//***************************************************************************
//*                                                                         *
//*  ULONG abort()                                                          *
//*                                                                         *
//*  Sends a SRB containing a Get Device Type command                       *
//*  Returns:                                                               *
//*    0  - Success                                                         *
//*    1  - DevIOCtl failed                                                 *
//*    2  - Abort SRB not successful                                        *
//*                                                                         *
//*  Preconditions: driver has to be opened                                 *
//*                                                                         *
//***************************************************************************
ULONG scsiObj::abort()
{
  ULONG rc;                                     // return value
  unsigned long cbreturn;
  unsigned long cbParam;

  AbortSRB.cmd=SRB_Abort;                       // abort SRB
  AbortSRB.ha_num=0;                            // host adapter number
  AbortSRB.flags=0;                             // no flags set
  AbortSRB.u.abt.srb=&SRBlock;                  // SRB to abort

  rc = DosDevIOCtl(driver_handle, 0x92, 0x02, (void*) &AbortSRB, sizeof(SRB), &cbParam,
                  (void*) &AbortSRB, sizeof(SRB), &cbreturn);
  if (rc)
    return 1;                                   // DosDevIOCtl failed
  if (SRBlock.status != SRB_Done) return 2;
  return 0;
}



