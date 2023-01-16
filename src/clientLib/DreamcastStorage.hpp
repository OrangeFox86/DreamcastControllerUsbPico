#pragma once

#include "hal/MapleBus/MaplePacket.hpp"
#include "DreamcastPeripheralFunction.hpp"
#include "dreamcast_constants.h"

#include <string.h>

#define U16_TO_UPPER_HALF_WORD(val) ((static_cast<uint32_t>(val) << 24) | ((static_cast<uint32_t>(val) << 8) & 0x00FF0000))
#define U16_TO_LOWER_HALF_WORD(val) (((static_cast<uint32_t>(val) << 8) & 0x0000FF00) | ((static_cast<uint32_t>(val) >> 8) & 0x000000FF))

#define SWAP_UPPER_AND_LOWER_U16(val32) ((((val32) << 8) & 0xFF00FF00) | (((val32) >> 8) & 0x00FF00FF))

namespace client
{
class DreamcastStorage : public DreamcastPeripheralFunction
{
public:
    inline DreamcastStorage() :
        DreamcastPeripheralFunction(DEVICE_FN_STORAGE),
        mWriteData{}
    {
        // Setup freshly formatted and cleared memory for standard 128 KB storage
        memset(mStorage, 0xFF, sizeof(mStorage));
        format();
    }

    inline void format()
    {
        //
        // System Block
        //
        uint32_t* systemBlock =
            reinterpret_cast<uint32_t*>(&mStorage[SYSTEM_BLOCK_NO * BYTES_PER_BLOCK]);
        memset(systemBlock, 0x55, 4 * WORD_SIZE);
        systemBlock[4] = 0x01FFFFFF;
        systemBlock[5] = 0xFF000000;
        memset(systemBlock + 6, 0, BYTES_PER_BLOCK - (6 * WORD_SIZE));
        // Date/time markers
        systemBlock[12] = 0x19990909;
        systemBlock[13] = 0x00001000;
        // Media info data
        systemBlock[16] = 0xFF000000;
        systemBlock[17] = 0xFF00FE00;
        systemBlock[18] = 0x0100FD00;
        systemBlock[19] = 0x0D000000;
        systemBlock[20] = 0xC8001F00;
        systemBlock[21] = 0x00008000;
        //
        // FAT Block
        //
        uint32_t* fatBlock = reinterpret_cast<uint32_t*>(
                &mStorage[(FAT_BLOCK_NO - NUM_FAT_BLOCKS + 1) * BYTES_PER_BLOCK]);
        for (uint32_t i = 0; i < WORDS_PER_BLOCK; ++i)
        {
            fatBlock[i] = U16_TO_UPPER_HALF_WORD(0xFFFC) | U16_TO_LOWER_HALF_WORD(0xFFFC);
        }
        fatBlock += ((NUM_FAT_BLOCKS) * WORDS_PER_BLOCK - 1);
        *fatBlock-- = U16_TO_UPPER_HALF_WORD(0xFFFA) | U16_TO_LOWER_HALF_WORD(0xFFFA);
        *fatBlock-- = U16_TO_UPPER_HALF_WORD(0x00FB) | U16_TO_LOWER_HALF_WORD(0x00FC);
        *fatBlock-- = U16_TO_UPPER_HALF_WORD(0x00F9) | U16_TO_LOWER_HALF_WORD(0x00FA);
        *fatBlock-- = U16_TO_UPPER_HALF_WORD(0x00F7) | U16_TO_LOWER_HALF_WORD(0x00F8);
        *fatBlock-- = U16_TO_UPPER_HALF_WORD(0x00F5) | U16_TO_LOWER_HALF_WORD(0x00F6);
        *fatBlock-- = U16_TO_UPPER_HALF_WORD(0x00F3) | U16_TO_LOWER_HALF_WORD(0x00F4);
        *fatBlock-- = U16_TO_UPPER_HALF_WORD(0x00F1) | U16_TO_LOWER_HALF_WORD(0x00F2);
        *fatBlock-- = U16_TO_UPPER_HALF_WORD(0xFFFC) | U16_TO_LOWER_HALF_WORD(0xFFFA);
        //
        // File Info Blocks
        //
        uint32_t* fileInfoBlock = reinterpret_cast<uint32_t*>(
                &mStorage[(FILE_INFO_BLOCK_NO - NUM_FILE_INFO_BLOCKS + 1) * BYTES_PER_BLOCK]);
        memset(fileInfoBlock, 0, NUM_FILE_INFO_BLOCKS * BYTES_PER_BLOCK);
    }

    inline virtual bool handlePacket(const MaplePacket& in, MaplePacket& out) final
    {
        const uint8_t cmd = in.frame.command;
        switch (cmd)
        {
            case COMMAND_GET_MEMORY_INFORMATION:
            {
                out.frame.command = COMMAND_RESPONSE_DATA_XFER;

                // This media info is usually stored at block 16 offset in the system block. If
                // memory is wiped though, the Dreamcast will get confused by the returned media
                // info and not know how to manage data. Thus, this info is made static here with
                // the exceptions of the volume icon and save area information.
                uint16_t totalSize = SYSTEM_BLOCK_NO; // total size excluding system block
                const uint16_t partitionNo = 0;
                const uint32_t* storageMediaInfo =
                    reinterpret_cast<const uint32_t*>(
                        &mStorage[(SYSTEM_BLOCK_NO * BYTES_PER_BLOCK) + (MEDIA_INFO_WORD_OFFSET * WORD_SIZE)]);
                uint32_t payload[7] = {
                    mFunctionCode,
                    U16_TO_UPPER_HALF_WORD(totalSize) | U16_TO_LOWER_HALF_WORD(partitionNo),
                    U16_TO_UPPER_HALF_WORD(SYSTEM_BLOCK_NO) | U16_TO_LOWER_HALF_WORD(FAT_BLOCK_NO),
                    U16_TO_UPPER_HALF_WORD(NUM_FAT_BLOCKS) | U16_TO_LOWER_HALF_WORD(FILE_INFO_BLOCK_NO),
                    U16_TO_UPPER_HALF_WORD(NUM_FILE_INFO_BLOCKS) | (storageMediaInfo[3] & 0xFFFF),
                    storageMediaInfo[4],
                    0x00008000};

                out.setPayload(payload, 7);
                return true;
            }
            break;

            case COMMAND_BLOCK_READ:
            {
                if (in.payload.size() > 1)
                {
                    uint32_t locationWord = in.payload[1];
                    uint16_t blockOffset = locationWord & 0xFFFF;
                    uint8_t phase = (locationWord >> 16) & 0xFF;
                    if (blockOffset >= NUM_BLOCKS || phase != 0)
                    {
                        // Outside range
                        out.frame.command = COMMAND_RESPONSE_FILE_ERROR;
                    }
                    else
                    {
                        // This takes about 100 microseconds to read
                        out.frame.command = COMMAND_RESPONSE_DATA_XFER;
                        uint32_t byteOffset = (blockOffset * BYTES_PER_BLOCK);
                        out.reservePayload(WORDS_PER_BLOCK + 2);
                        out.setPayload(&mFunctionCode, 1);
                        out.appendPayload(locationWord);
                        const uint32_t* ptr = reinterpret_cast<const uint32_t*>(&mStorage[byteOffset]);
                        out.appendPayload(ptr, WORDS_PER_BLOCK);
                    }
                    return true;
                }
            }
            break;

            case COMMAND_BLOCK_WRITE:
            {
                if (in.payload.size() > 1)
                {
                    uint32_t locationWord = in.payload[1];
                    uint16_t blockOffset = locationWord & 0xFFFF;
                    uint8_t phase = (locationWord >> 16) & 0xFF;
                    if (blockOffset >= NUM_BLOCKS
                        || phase >= WRITES_PER_BLOCK
                        || (in.payload.size() - 2) != (BYTES_PER_WRITE / 4))
                    {
                        // Outside range
                        out.frame.command = COMMAND_RESPONSE_FILE_ERROR;
                    }
                    else
                    {
                        uint32_t byteOffset = (phase * BYTES_PER_WRITE);
                        memcpy(&mWriteData[byteOffset], &in.payload[2], BYTES_PER_WRITE);
                        out.frame.command = COMMAND_RESPONSE_ACK;
                    }
                    return true;
                }
            }
            break;

            case COMMAND_GET_LAST_ERROR:
            {
                if (in.payload.size() > 1)
                {
                    uint32_t locationWord = in.payload[1];
                    uint16_t blockOffset = locationWord & 0xFFFF;
                    uint8_t phase = (locationWord >> 16) & 0xFF;
                    if (blockOffset >= NUM_BLOCKS || phase != WRITES_PER_BLOCK)
                    {
                        // Outside range
                        out.frame.command = COMMAND_RESPONSE_FILE_ERROR;
                    }
                    else
                    {
                        uint32_t byteOffset = (blockOffset * BYTES_PER_BLOCK);
                        memcpy(&mStorage[byteOffset], mWriteData, BYTES_PER_BLOCK);
                        out.frame.command = COMMAND_RESPONSE_ACK;
                    }
                    return true;
                }
            }
            break;

            default:
            {
            }
            break;
        }
        return false;
    }

    inline virtual void reset() final
    {}

    inline virtual uint32_t getFunctionDefinition() final
    {
        return (
            (IS_REMOVABLE ? 0x00000080 : 0)
            | (CRC_NEEDED ? 0x00000040 : 0)
            | ((READS_PER_BLOCK & 0xF) << 8)
            | ((WRITES_PER_BLOCK & 0xF) << 12)
            | (((BYTES_PER_BLOCK / 32 - 1) & 0xFF) << 16)
            | (((NUMBER_OF_PARTITIONS - 1) & 0xFF) << 24)
        );
    }

private:
    static const uint16_t NUMBER_OF_PARTITIONS = 1;
    static const uint16_t BYTES_PER_BLOCK = 512;
    static const uint8_t WORD_SIZE = sizeof(uint32_t);
    static const uint16_t WORDS_PER_BLOCK = BYTES_PER_BLOCK / WORD_SIZE;
    static const uint8_t WRITES_PER_BLOCK = 4;
    static const uint8_t READS_PER_BLOCK = 1;
    static const bool IS_REMOVABLE = false;
    static const bool CRC_NEEDED = false;
    static const uint32_t MEMORY_SIZE_BYTES = 128 * 1024;
    static const uint32_t MEMORY_WORD_COUNT = MEMORY_SIZE_BYTES / WORD_SIZE;
    static const uint16_t NUM_BLOCKS = MEMORY_WORD_COUNT / WORDS_PER_BLOCK;
    static const uint32_t BYTES_PER_WRITE = (WRITES_PER_BLOCK > 0) ? (BYTES_PER_BLOCK / WRITES_PER_BLOCK) : 0;
    static const uint16_t SYSTEM_BLOCK_NO = (NUM_BLOCKS - 1);
    static const uint16_t NUM_SYSTEM_BLOCKS = 1;
    static const uint16_t MEDIA_INFO_WORD_OFFSET = 16;
    static const uint16_t FAT_BLOCK_NO = SYSTEM_BLOCK_NO - NUM_SYSTEM_BLOCKS;
    static const uint16_t NUM_FAT_BLOCKS = 1;
    static const uint16_t FILE_INFO_BLOCK_NO = FAT_BLOCK_NO - NUM_FAT_BLOCKS;
    static const uint16_t NUM_FILE_INFO_BLOCKS = 13;

    //! Storage for a single block of data for write process
    uint8_t mWriteData[BYTES_PER_BLOCK];

    //! Storage area in RAM (flash storage to be implemented later)
    uint8_t mStorage[MEMORY_SIZE_BYTES];
};
}