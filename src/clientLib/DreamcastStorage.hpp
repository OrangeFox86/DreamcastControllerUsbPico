#pragma once

#include "hal/MapleBus/MaplePacket.hpp"
#include "DreamcastPeripheralFunction.hpp"
#include "dreamcast_constants.h"

#include "hardware/flash.h"
#include "boards/pico.h"

#include <string.h>

namespace client
{
class DreamcastStorage : public DreamcastPeripheralFunction
{
public:
    inline DreamcastStorage() :
        DreamcastPeripheralFunction(DEVICE_FN_STORAGE),
        mWriteData{}
    {}

    inline virtual bool handlePacket(const MaplePacket& in, MaplePacket& out) final
    {
        const uint8_t cmd = in.getFrameCommand();
        switch (cmd)
        {
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
                        out.reservePayload(WORDS_PER_BLOCK + 1);
                        out.setPayload(&mFunctionCode, 1);
                        const uint32_t* ptr = reinterpret_cast<const uint32_t*>(&getStorage()[byteOffset]);
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
                        uint32_t byteOffset = (blockOffset * BYTES_PER_BLOCK)
                                              + (phase * BYTES_PER_WRITE);
                        memcpy(&mWriteData[byteOffset], &out.payload[2], BYTES_PER_WRITE);
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
                        #if 0
                        // This cannot be handled within the 1 ms we have to process
                        uint32_t flashAddr = FLASH_OFFSET + (blockOffset * BYTES_PER_BLOCK);
                        // can only write in 4096 byte sectors because flash can only be erased in 4096 byte sectors
                        uint8_t sector[4096];
                        uint32_t offset = flashAddr % 4096;
                        memcpy(sector, &getStorage()[flashAddr - offset], 4096);
                        memcpy(&sector[offset], mWriteData, BYTES_PER_BLOCK);
                        flash_range_erase(flashAddr - offset, 4096);
                        flash_range_program(flashAddr - offset, sector, 4096);
                        out.setCommand(COMMAND_RESPONSE_ACK);
                        #else
                        out.setCommand(COMMAND_RESPONSE_FILE_ERROR);
                        #endif
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

    inline const uint8_t* getStorage()
    {
        return (const uint8_t *) (XIP_BASE + FLASH_OFFSET);
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
    static const uint32_t NUM_BLOCKS = MEMORY_WORD_COUNT / WORDS_PER_BLOCK;
    static const uint32_t FLASH_OFFSET = PICO_FLASH_SIZE_BYTES - MEMORY_SIZE_BYTES;
    static const uint32_t BYTES_PER_WRITE = BYTES_PER_BLOCK / WRITES_PER_BLOCK;
    uint8_t mWriteData[BYTES_PER_BLOCK];
};
}