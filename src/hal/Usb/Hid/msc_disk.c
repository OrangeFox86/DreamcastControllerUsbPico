/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "bsp/board.h"
#include "tusb.h"

#if CFG_TUD_MSC

// whether host does safe-eject
static bool ejected = false;

#define README_CONTENTS \
"This is where Dreamcast MU data will show up when detected"

// Size of string minus null terminator byte
#define README_SIZE (sizeof(README_CONTENTS) - 1)

#define CHAR_TO_UPPER(c) ((c >= 'a' && c <= 'z') ? (c - ('a' - 'A')) : c)
// Computes the seconds increment for a given seconds value (seconds may be floating point)
#define DIRECTORY_ENTRY_TIME_SEC_INC(seconds) (int)(((seconds - ((int)seconds - ((int)seconds % 2))) * 100) + 0.5)
#define DIRECTORY_ENTRY_TIME(hours, minutes, seconds) ((((hours - 1) & 0x1F) << 11) | ((minutes & 0x3F) << 5) | (((int)seconds / 2) & 0x1F))
#define DIRECTORY_ENTRY_DATE(year, month, day) ((((year - 1980) & 0x7F) << 9) | ((month & 0x0F) << 5) | (day & 0x1F))

#define DIRECTORY_ENTRY(name8, ext3, attr1, attr2, creation_time_sec_inc, creation_time, creation_date, access_date, file_attr, mod_time, mod_date, starting_page, file_size) \
      CHAR_TO_UPPER(name8[0]), CHAR_TO_UPPER(name8[1]), CHAR_TO_UPPER(name8[2]), CHAR_TO_UPPER(name8[3]), \
      CHAR_TO_UPPER(name8[4]), CHAR_TO_UPPER(name8[5]), CHAR_TO_UPPER(name8[6]), CHAR_TO_UPPER(name8[7]), \
      CHAR_TO_UPPER(ext3[0]), CHAR_TO_UPPER(ext3[1]), CHAR_TO_UPPER(ext3[2]),         \
      attr1, attr2,                                                                   \
      creation_time_sec_inc,                                                          \
      U16_TO_U8S_LE(creation_time),                                                   \
      U16_TO_U8S_LE(creation_date),                                                   \
      U16_TO_U8S_LE(access_date),                                                     \
      U16_TO_U8S_LE(file_attr),                                                       \
      U16_TO_U8S_LE(mod_time),                                                        \
      U16_TO_U8S_LE(mod_date),                                                        \
      U16_TO_U8S_LE(starting_page),                                                   \
      U32_TO_U8S_LE(file_size)

#define DEFAULT_FILE_TIME_SECONDS 4
#define DEFAULT_FILE_TIME_SEC_INC DIRECTORY_ENTRY_TIME_SEC_INC(DEFAULT_FILE_TIME_SECONDS)
#define DEFAULT_FILE_TIME DIRECTORY_ENTRY_TIME(7, 1, DEFAULT_FILE_TIME_SECONDS)
#define DEFAULT_FILE_DATE DIRECTORY_ENTRY_DATE(1999, 9, 9)

#define SIMPLE_DIR_ENTRY(name8, ext3, starting_page, file_size) \
      DIRECTORY_ENTRY(name8,                      \
                      ext3,                       \
                      0x20,                       \
                      0,                          \
                      DEFAULT_FILE_TIME_SEC_INC,  \
                      DEFAULT_FILE_TIME,          \
                      DEFAULT_FILE_DATE,          \
                      DEFAULT_FILE_DATE,          \
                      0,                          \
                      DEFAULT_FILE_TIME,          \
                      DEFAULT_FILE_DATE,          \
                      starting_page,              \
                      file_size)

enum
{
  DISK_BLOCK_NUM  = 16, // 8KB is the smallest size that windows allow to mount
  DISK_BLOCK_SIZE = 512
};

uint8_t msc_disk[DISK_BLOCK_NUM][DISK_BLOCK_SIZE] =
{
  //------------- Block0: Boot Sector -------------//
  {
      // Jump instruction
      0xEB, 0x3C, 0x90,
      // OEM Name
      'M', 'S', 'D', 'O', 'S', '5', '.', '0',

      // BIOS Parameter Block
      // Bytes per sector
      U16_TO_U8S_LE(DISK_BLOCK_SIZE),
      // Sectors per cluster
      0x01,
      // Reserved sectors
      U16_TO_U8S_LE(1),
      // Number of copies of the file allocation tables
      0x01,
      // Number of root entries (maximum number of files under root)
      U16_TO_U8S_LE(16),
      // Number of sectors (small)
      U16_TO_U8S_LE(DISK_BLOCK_NUM),
      // Media type (hard disk)
      0xF8,
      // Sectors per FAT
      U16_TO_U8S_LE(1),
      // Sectors per track
      U16_TO_U8S_LE(1),
      // Number of heads
      U16_TO_U8S_LE(1),
      // Hidden sectors
      U32_TO_U8S_LE(0),
      // Number of sectors (large) (only used if small value is 0)
      U32_TO_U8S_LE(0),
      // Physical disk number
      0x80,
      // Current head
      0x00,
      // signature (must be either 0x28 or 0x29)
      0x29,
      // Volume serial number
      0x34, 0x12, 0x00, 0x00,
      // Volume label (11 bytes) (no longer actually used)
      'D' , 'r' , 'e' , 'a' , 'm' , 'c' , 'a' , 's' , 't' , 'M' , 'U' ,
      // System ID (8 bytes)
      'F', 'A', 'T', '1', '6', ' ', ' ', ' ',

      // Zero up to 2 last bytes of FAT magic code
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      // FAT magic code
      0x55, 0xAA
  },

  //------------- Block1: FAT12 Table -------------//
  {
      // first 16 bit entry must be FFF8
      U16_TO_U8S_LE(0xFFF8),
      // End of chain indicator / maintenance flags
      U16_TO_U8S_LE(0xFFFF),
      // The rest is pointers to next cluster number of 0xFFFF if end (for page 2+)
      U16_TO_U8S_LE(0xFFFF)
  },

  //------------- Block2-5: Root Directory -------------//
  {
      // first entry is volume label
      'D' , 'r' , 'e' , 'a' , 'm' , 'c' , 'a' , 's' , 't' , 'M' , 'U' , 0x08, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4F, 0x6D, 0x65, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      // second entry is readme file
      SIMPLE_DIR_ENTRY("README  ", "TXT", 2, README_SIZE)
  },

  //------------- Block6+: File Content -------------//
  {README_CONTENTS}
};

// Invoked when received SCSI_CMD_INQUIRY
// Application fill vendor id, product id and revision with string up to 8, 16, 4 characters respectively
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
{
  (void) lun;

  const char vid[] = "OngFx86";
  const char pid[] = "Dreamcast Ctrlr";
  const char rev[] = "1.0";

  memcpy(vendor_id  , vid, strlen(vid));
  memcpy(product_id , pid, strlen(pid));
  memcpy(product_rev, rev, strlen(rev));
}

// Invoked when received Test Unit Ready command.
// return true allowing host to read/write this LUN e.g SD card inserted
bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
  (void) lun;

  // RAM disk is ready until ejected
  if (ejected) {
    tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3a, 0x00);
    return false;
  }

  return true;
}

// Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY to determine the disk size
// Application update block count and block size
void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size)
{
  (void) lun;

  *block_count = DISK_BLOCK_NUM;
  *block_size  = DISK_BLOCK_SIZE;
}

// Invoked when received Start Stop Unit command
// - Start = 0 : stopped power mode, if load_eject = 1 : unload disk storage
// - Start = 1 : active mode, if load_eject = 1 : load disk storage
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
{
  (void) lun;
  (void) power_condition;

  if ( load_eject )
  {
    if (start)
    {
      // load disk storage
    }else
    {
      // unload disk storage
      ejected = true;
    }
  }

  return true;
}

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and return number of copied bytes.
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
{
  (void) lun;

  // out of ramdisk
  if ( lba >= DISK_BLOCK_NUM ) return -1;

  uint8_t const* addr = msc_disk[lba] + offset;
  memcpy(buffer, addr, bufsize);

  return bufsize;
}

bool tud_msc_is_writable_cb (uint8_t lun)
{
  (void) lun;

  // Read only drive
  return false;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and return number of written bytes
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize)
{
  (void) lun;

  // out of ramdisk
  if ( lba >= DISK_BLOCK_NUM ) return -1;

  // Read only drive
  (void) lba; (void) offset; (void) buffer;

  return bufsize;
}

// Callback invoked when received an SCSI command not in built-in list below
// - READ_CAPACITY10, READ_FORMAT_CAPACITY, INQUIRY, MODE_SENSE6, REQUEST_SENSE
// - READ10 and WRITE10 has their own callbacks
int32_t tud_msc_scsi_cb (uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize)
{
  // read10 & write10 has their own callback and MUST not be handled here

  void const* response = NULL;
  int32_t resplen = 0;

  // most scsi handled is input
  bool in_xfer = true;

  switch (scsi_cmd[0])
  {
    case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
      // Host is about to read/write etc ... better not to disconnect disk
      resplen = 0;
    break;

    default:
      // Set Sense = Invalid Command Operation
      tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);

      // negative means error -> tinyusb could stall and/or response with failed status
      resplen = -1;
    break;
  }

  // return resplen must not larger than bufsize
  if ( resplen > bufsize ) resplen = bufsize;

  if ( response && (resplen > 0) )
  {
    if(in_xfer)
    {
      memcpy(buffer, response, resplen);
    }else
    {
      // SCSI output
    }
  }

  return resplen;
}

#endif
