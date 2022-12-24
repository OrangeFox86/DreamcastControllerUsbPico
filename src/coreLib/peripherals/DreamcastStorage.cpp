#include "DreamcastStorage.hpp"
#include "dreamcast_constants.h"

#include <assert.h>
#include <string.h>

DreamcastStorage::DreamcastStorage(uint8_t addr,
                                   std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                                   PlayerData playerData) :
    DreamcastPeripheral("storage", addr, scheduler, playerData.playerIndex),
    mExiting(false),
    mClock(playerData.clock),
    mUsbFileSystem(playerData.fileSystem),
    mFileName{},
    mReadState(READ_WRITE_IDLE),
    mReadingTxId(0),
    mReadingBlock(-1),
    mReadPacket(nullptr),
    mReadKillTime(0),
    mWriteState(READ_WRITE_IDLE),
    mWritingTxId(0),
    mWritingBlock(0),
    mWriteBuffer(nullptr),
    mWriteBufferLen(0),
    mWriteKillTime(0)
{
    int32_t idx = subPeripheralIndex(mAddr);
    if (idx >= 0)
    {
        if (idx == 0)
        {
            snprintf(mFileName, sizeof(mFileName), "vmu%lu.bin", (long unsigned int)mPlayerIndex);
        }
        else
        {
            snprintf(mFileName, sizeof(mFileName), "vmu%lu-%li.bin", (long unsigned int)mPlayerIndex, (long int)idx);
        }

        mUsbFileSystem.add(this);
    }
}

DreamcastStorage::~DreamcastStorage()
{
    mExiting = true;
    // The following is externally serialized with any read() call
    mUsbFileSystem.remove(this);
}

void DreamcastStorage::task(uint64_t currentTimeUs)
{
    switch(mReadState)
    {
        case READ_WRITE_STARTED:
        {
            uint32_t payload[2] = {FUNCTION_CODE, mReadingBlock};
            mReadingTxId = mEndpointTxScheduler->add(0, this, COMMAND_BLOCK_READ, payload, 2, true, 130);
            mReadState = READ_WRITE_SENT;
        }
        break;

        case READ_WRITE_SENT:
        {
            if (currentTimeUs >= mReadKillTime)
            {
                // Timeout
                mEndpointTxScheduler->cancelById(mReadingTxId);
                mReadState = READ_WRITE_IDLE;
            }
        }
        break;

        case READ_WRITE_PROCESSING:
            // Already processing, so no need to check timeout value
        default:
            break;
    }

    switch(mWriteState)
    {
        case READ_WRITE_STARTED:
        {
            // Build the payload with write data
            uint32_t payload[2 + mWriteBufferLen] = {FUNCTION_CODE, mWritingBlock};
            const uint32_t* pDataIn = static_cast<const uint32_t*>(mWriteBuffer);
            uint32_t *pDataOut = &payload[2];
            for (int32_t i = 0; i < mWriteBufferLen; i += 4, ++pDataIn, ++pDataOut)
            {
                // Assumption: Data stored as little endian
                *pDataOut = flipWordBytes(*pDataIn);
            }

            mWritingTxId = mEndpointTxScheduler->add(
                0,
                this,
                COMMAND_BLOCK_WRITE,
                payload,
                sizeof(payload) / sizeof(payload[0]),
                true,
                0);

            mWriteState = READ_WRITE_SENT;
        }
        break;

        case READ_WRITE_SENT:
        {
            if (currentTimeUs >= mWriteKillTime)
            {
                // Timeout
                mEndpointTxScheduler->cancelById(mWritingTxId);
                mWriteBufferLen = -1;
                mWriteState = READ_WRITE_IDLE;
            }
        }
        break;

        case READ_WRITE_PROCESSING:
            // Already processing, so no need to check timeout value
        default:
            break;
    }
}

void DreamcastStorage::txStarted(std::shared_ptr<const Transmission> tx)
{
    if (mReadState != READ_WRITE_IDLE && tx->transmissionId == mReadingTxId)
    {
        mReadState = READ_WRITE_PROCESSING;
    }
    if (mWriteState != READ_WRITE_IDLE && tx->transmissionId == mWritingTxId)
    {
        mWriteState = READ_WRITE_PROCESSING;
    }
}

void DreamcastStorage::txFailed(bool writeFailed,
                                bool readFailed,
                                std::shared_ptr<const Transmission> tx)
{
    if (mReadState != READ_WRITE_IDLE && tx->transmissionId == mReadingTxId)
    {
        // Failure
        mReadState = READ_WRITE_IDLE;
    }
    if (mWriteState != READ_WRITE_IDLE && tx->transmissionId == mWritingTxId)
    {
        // Failure
        mWriteBufferLen = 0;
        mWriteState = READ_WRITE_IDLE;
    }
}

void DreamcastStorage::txComplete(std::shared_ptr<const MaplePacket> packet,
                                  std::shared_ptr<const Transmission> tx)
{
    if (mReadState != READ_WRITE_IDLE && tx->transmissionId == mReadingTxId)
    {
        // Complete!
        mReadPacket = packet;
        mReadState = READ_WRITE_IDLE;
    }
    if (mWriteState != READ_WRITE_IDLE && tx->transmissionId == mWritingTxId)
    {
        // Complete!
        mWriteState = READ_WRITE_IDLE;
    }
}

const char* DreamcastStorage::getFileName()
{
    return mFileName;
}

uint32_t DreamcastStorage::getFileSize()
{
    return (128 * 1024);
}

int32_t DreamcastStorage::read(uint8_t blockNum,
                               void* buffer,
                               uint16_t bufferLen,
                               uint32_t timeoutUs)
{
    assert(mReadState == READ_WRITE_IDLE);
    // Set data
    mReadingTxId = 0;
    mReadingBlock = blockNum;
    mReadPacket = nullptr;
    mReadKillTime = mClock.getTimeUs() + timeoutUs;
    // Commit it
    mReadState = READ_WRITE_STARTED;

    // Wait for maple bus state machine to finish read
    // I'm not too happy about this blocking operation, but it works
    while(mReadState != READ_WRITE_IDLE && !mExiting);

    int32_t numRead = -1;
    if (mReadPacket)
    {
        uint16_t copyLen = (bufferLen > (mReadPacket->payload.size() * 4)) ? (mReadPacket->payload.size() * 4) : bufferLen;
        // Need to flip each word before copying
        uint8_t* buffer8 = (uint8_t*)buffer;
        for (uint32_t i = 2; i < (2U + (bufferLen / 4)); ++i)
        {
            uint32_t flippedWord = flipWordBytes(mReadPacket->payload[i]);
            memcpy(buffer8, &flippedWord, 4);
            buffer8 += 4;
        }
        numRead = copyLen;
    }

    mReadPacket = nullptr;

    return numRead;
}

int32_t DreamcastStorage::write(uint8_t blockNum,
                                const void* buffer,
                                uint16_t bufferLen,
                                uint32_t timeoutUs)
{
    assert(mWriteState == READ_WRITE_IDLE);
    assert(bufferLen % 4 == 0);
    // Set data
    mWritingBlock = blockNum;
    mWriteBuffer = buffer;
    mWriteBufferLen = bufferLen;
    mWritingTxId = 0;
    mWriteKillTime = mClock.getTimeUs() + timeoutUs;
    // Commit it
    mWriteState = READ_WRITE_STARTED;

    // Wait for maple bus state machine to finish read
    // I'm not too happy about this blocking operation, but it works
    while(mWriteState != READ_WRITE_IDLE && !mExiting);

    return mWriteBufferLen;
}

uint32_t DreamcastStorage::flipWordBytes(const uint32_t& word)
{
    return (word << 24) | (word << 8 & 0xFF0000) | (word >> 8 & 0xFF00) | (word >> 24);
}
