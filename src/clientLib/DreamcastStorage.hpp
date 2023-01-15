#pragma once

#include "hal/MapleBus/MaplePacket.hpp"
#include "DreamcastPeripheralFunction.hpp"
#include "dreamcast_constants.h"

#include <string.h>

#define U16_TO_UPPER_HALF_WORD(val) ((static_cast<uint32_t>(val) << 24) | ((static_cast<uint32_t>(val) << 8) & 0x00FF0000))
#define U16_TO_LOWER_HALF_WORD(val) (((static_cast<uint32_t>(val) << 8) & 0x0000FF00) | ((static_cast<uint32_t>(val) >> 8) & 0x000000FF))

namespace client
{
class DreamcastStorage : public DreamcastPeripheralFunction
{
public:
    inline DreamcastStorage() :
        DreamcastPeripheralFunction(DEVICE_FN_STORAGE),
        mWriteData{},
        mStorage{}
    {}

    inline virtual bool handlePacket(const MaplePacket& in, MaplePacket& out) final
    {
        const uint8_t cmd = in.getFrameCommand();
        switch (cmd)
        {
            case COMMAND_GET_MEMORY_INFORMATION:
            {
                out.setCommand(COMMAND_RESPONSE_DATA_XFER);
                uint16_t totalSize = (NUM_BLOCKS - 1);
                const uint16_t partitionNo = 0;
                const uint16_t systemBlockNo = totalSize;
                const uint16_t fatBlockNo = systemBlockNo - 1;
                const uint16_t numFatBlocks = 1;
                const uint16_t fileInfoBlockNo = fatBlockNo - numFatBlocks;
                const uint16_t numFileInfoBlocks = 13;
                const uint16_t volumeIcon = 5;
                const uint16_t saveAreaBlockNo = NUM_BLOCKS - 56;
                const uint16_t numSaveAreaBlocks = 31;
                uint32_t payload[7] = {
                    mFunctionCode,
                    U16_TO_UPPER_HALF_WORD(totalSize) | U16_TO_LOWER_HALF_WORD(partitionNo),
                    U16_TO_UPPER_HALF_WORD(systemBlockNo) | U16_TO_LOWER_HALF_WORD(fatBlockNo),
                    U16_TO_UPPER_HALF_WORD(numFatBlocks) | U16_TO_LOWER_HALF_WORD(fileInfoBlockNo),
                    U16_TO_UPPER_HALF_WORD(numFileInfoBlocks) | U16_TO_LOWER_HALF_WORD(volumeIcon),
                    U16_TO_UPPER_HALF_WORD(saveAreaBlockNo) | U16_TO_LOWER_HALF_WORD(numSaveAreaBlocks),
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
                        out.setCommand(COMMAND_RESPONSE_FILE_ERROR);
                    }
                    else
                    {
                        // This takes about 100 microseconds to read
                        out.setCommand(COMMAND_RESPONSE_DATA_XFER);
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
                        out.setCommand(COMMAND_RESPONSE_FILE_ERROR);
                    }
                    else
                    {
                        uint32_t byteOffset = (phase * BYTES_PER_WRITE);
                        memcpy(&mWriteData[byteOffset], &in.payload[2], BYTES_PER_WRITE);
                        out.setCommand(COMMAND_RESPONSE_ACK);
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
                        out.setCommand(COMMAND_RESPONSE_FILE_ERROR);
                    }
                    else
                    {
                        uint32_t byteOffset = (blockOffset * BYTES_PER_BLOCK);
                        memcpy(&mStorage[byteOffset], mWriteData, BYTES_PER_BLOCK);
                        out.setCommand(COMMAND_RESPONSE_ACK);
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
    static const uint16_t WORDS_PER_BLOCK = BYTES_PER_BLOCK / 4;
    static const uint8_t WRITES_PER_BLOCK = 4;
    static const uint8_t READS_PER_BLOCK = 1;
    static const bool IS_REMOVABLE = false;
    static const bool CRC_NEEDED = false;
    static const uint32_t MEMORY_SIZE_BYTES = 128 * 1024;
    static const uint32_t MEMORY_WORD_COUNT = MEMORY_SIZE_BYTES / 4;
    static const uint16_t NUM_BLOCKS = MEMORY_WORD_COUNT / WORDS_PER_BLOCK;
    static const uint32_t BYTES_PER_WRITE = (WRITES_PER_BLOCK > 0) ? (BYTES_PER_BLOCK / WRITES_PER_BLOCK) : 0;

    // Storage for a single block of data for write process
    uint8_t mWriteData[BYTES_PER_BLOCK];

    // Storage area in RAM (flash storage to be implemented later)
    uint8_t mStorage[MEMORY_SIZE_BYTES];
};
}