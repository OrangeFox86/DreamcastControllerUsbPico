#pragma once

#include "hal/MapleBus/MaplePacket.hpp"
#include "DreamcastPeripheralFunction.hpp"
#include "dreamcast_constants.h"

#include "hal/System/SystemMemory.hpp"

#include <memory>
#include <string.h>

#define U16_TO_UPPER_HALF_WORD(val) ((static_cast<uint32_t>(val) << 24) | ((static_cast<uint32_t>(val) << 8) & 0x00FF0000))
#define U16_TO_LOWER_HALF_WORD(val) (((static_cast<uint32_t>(val) << 8) & 0x0000FF00) | ((static_cast<uint32_t>(val) >> 8) & 0x000000FF))

#define SWAP_UPPER_AND_LOWER_U16(val32) ((((val32) << 8) & 0xFF00FF00) | (((val32) >> 8) & 0x00FF00FF))

namespace client
{
class DreamcastStorage : public DreamcastPeripheralFunction
{
public:
    //! Default constructor
    inline DreamcastStorage(std::shared_ptr<SystemMemory> systemMemory, uint32_t memoryOffset) :
        DreamcastPeripheralFunction(DEVICE_FN_STORAGE),
        mSystemMemory(systemMemory),
        mMemoryOffset(memoryOffset),
        mDataBlock{}
    {}

    //! Read a block of data, blocks until all data read
    //! @param[in] blockNum  The block number to read
    //! @returns pointer to a full block of data
    const uint8_t* readBlock(uint16_t blockNum)
    {
        uint32_t size = BYTES_PER_BLOCK;
        const uint8_t* mem =
            mSystemMemory->read(mMemoryOffset + (blockNum * BYTES_PER_BLOCK), size);
        // TODO: check returned size
        (void)size;

        return mem;
    }

    //! Write a block of data
    //! @param[in] blockNum  The block number to write
    //! @param[in] data  The block of data to write
    //! @returns true if successful or false if a failure occurred
    bool writeBlock(uint16_t blockNum, const void* data)
    {
        uint32_t size = BYTES_PER_BLOCK;
        return mSystemMemory->write(mMemoryOffset + (blockNum * BYTES_PER_BLOCK), data, size);
    }

    //! Formats the storage
    //! @returns true iff format writes completed
    inline bool format()
    {
        //
        // System Block
        //
        uint32_t* systemBlock = reinterpret_cast<uint32_t*>(mDataBlock);
        memset(systemBlock, 0, BYTES_PER_BLOCK);
        for (uint32_t i = 0; i < NUM_SYSTEM_BLOCKS - 1; ++i)
        {
            if (!writeBlock(SYSTEM_BLOCK_NO - i, systemBlock))
            {
                return false;
            }
        }
        memset(systemBlock, 0x55, 4 * WORD_SIZE);
        systemBlock[4] = 0x01FFFFFF;
        systemBlock[5] = 0xFF000000;
        // Date/time markers
        systemBlock[12] = 0x19990909;
        systemBlock[13] = 0x00001000;
        // Media info data
        systemBlock[16] = U16_TO_UPPER_HALF_WORD(NUM_BLOCKS - 1) | U16_TO_LOWER_HALF_WORD(0);
        systemBlock[17] = U16_TO_UPPER_HALF_WORD(SYSTEM_BLOCK_NO) | U16_TO_LOWER_HALF_WORD(FAT_BLOCK_NO);
        systemBlock[18] = U16_TO_UPPER_HALF_WORD(NUM_FAT_BLOCKS) | U16_TO_LOWER_HALF_WORD(FILE_INFO_BLOCK_NO);
        systemBlock[19] = U16_TO_UPPER_HALF_WORD(NUM_FILE_INFO_BLOCKS) | U16_TO_LOWER_HALF_WORD(0);
        systemBlock[20] = U16_TO_UPPER_HALF_WORD(SAVE_AREA_BLOCK_NO) | U16_TO_LOWER_HALF_WORD(NUM_SAVE_AREA_BLOCKS);
        systemBlock[21] = 0x00008000;
        if (!writeBlock(SYSTEM_BLOCK_NO - NUM_SYSTEM_BLOCKS + 1, systemBlock))
        {
            return false;
        }
        //
        // FAT Block(s)
        //
        uint32_t* fatBlock = reinterpret_cast<uint32_t*>(mDataBlock);
        for (uint32_t i = 0; i < WORDS_PER_BLOCK; ++i)
        {
            fatBlock[i] = U16_TO_UPPER_HALF_WORD(0xFFFC) | U16_TO_LOWER_HALF_WORD(0xFFFC);
        }
        for (uint32_t i = 0; i < NUM_FAT_BLOCKS - 1; ++i)
        {
            if (!writeBlock(FAT_BLOCK_NO - i, fatBlock))
            {
                return false;
            }
        }
        fatBlock += (WORDS_PER_BLOCK - 1);
        bool lower = true;
        for (uint32_t i = 0; i < (NUM_SYSTEM_BLOCKS + NUM_FAT_BLOCKS); ++i)
        {
            setFatAddr(fatBlock, lower, 0xFFFA);
        }
        uint16_t addr = NUM_BLOCKS - NUM_SYSTEM_BLOCKS - NUM_FAT_BLOCKS - 1;
        for (uint32_t i = 0; i < NUM_FILE_INFO_BLOCKS - 1; ++i)
        {
            setFatAddr(fatBlock, lower, --addr);
        }
        setFatAddr(fatBlock, lower, 0xFFFA);
        fatBlock = reinterpret_cast<uint32_t*>(mDataBlock);
        if (!writeBlock(FAT_BLOCK_NO - NUM_FAT_BLOCKS + 1, fatBlock))
        {
            return false;
        }
        //
        // File Info Blocks
        //
        uint32_t* fileInfoBlock = reinterpret_cast<uint32_t*>(mDataBlock);
        memset(fileInfoBlock, 0, BYTES_PER_BLOCK);
        for (uint32_t i = 0; i < NUM_FILE_INFO_BLOCKS; ++i)
        {
            if (!writeBlock(FILE_INFO_BLOCK_NO - i, fileInfoBlock))
            {
                return false;
            }
        }
        return true;
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
                const uint32_t* storageMediaInfo =
                    reinterpret_cast<const uint32_t*>(readBlock(SYSTEM_BLOCK_NO))
                    + MEDIA_INFO_WORD_OFFSET;
                uint32_t payload[7] = {
                    mFunctionCode,
                    U16_TO_UPPER_HALF_WORD(NUM_BLOCKS - 1) | U16_TO_LOWER_HALF_WORD(0),
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
                        out.reservePayload(WORDS_PER_BLOCK + 2);
                        out.setPayload(&mFunctionCode, 1);
                        out.appendPayload(locationWord);
                        out.appendPayload(reinterpret_cast<const uint32_t*>(readBlock(blockOffset)),
                                          WORDS_PER_BLOCK);
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
                        memcpy(&mDataBlock[byteOffset], &in.payload[2], BYTES_PER_WRITE);
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
                        if (writeBlock(blockOffset, mDataBlock))
                        {
                            out.frame.command = COMMAND_RESPONSE_ACK;
                        }
                        else
                        {
                            out.frame.command = COMMAND_RESPONSE_FILE_ERROR;
                            // TODO: need to set file error payload
                        }
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

    inline virtual void reset()
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
    inline static void setFatAddr(uint32_t*& fatBlock, bool& lower, uint16_t value)
    {
        if (lower)
        {
            lower = false;
            *fatBlock = (*fatBlock & 0xFFFF0000) | U16_TO_LOWER_HALF_WORD(value);
        }
        else
        {
            lower = true;
            *fatBlock = U16_TO_UPPER_HALF_WORD(value) | (*fatBlock & 0x0000FFFF);
            --fatBlock;
        }
    }

public:
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
    static const uint16_t SAVE_AREA_BLOCK_NO = 200;
    static const uint16_t NUM_SAVE_AREA_BLOCKS = 31;

protected:
    //! The system memory object where data is to be written
    std::shared_ptr<SystemMemory> mSystemMemory;
    //! Byte offset into memory
    uint32_t mMemoryOffset;
    //! Storage for a single block of data for read or write purposes
    uint8_t mDataBlock[BYTES_PER_BLOCK];
};
}