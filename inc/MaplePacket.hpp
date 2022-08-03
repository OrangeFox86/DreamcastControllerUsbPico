#ifndef __MAPLE_PACKET_H__
#define __MAPLE_PACKET_H__

#include <stdint.h>
#include <vector>
#include <utility>
#include "configuration.h"
#include "dreamcast_constants.h"

struct MaplePacket
{
private:
    //! Frame word
    uint32_t mFrameWord;
    //! Following payload words
    std::vector<uint32_t> mPayload;

public:
    //! Read-only reference to frame word
    const uint32_t& frameWord;
    //! Read-only reference to payload words
    const std::vector<uint32_t>& payload;

    //! Byte position of the command in the frame word
    static const uint32_t COMMAND_POSITION = 24;
    //! Byte position of the recipient address in the frame word
    static const uint32_t RECIPIENT_ADDR_POSITION = 16;
    //! Byte position of the sender address in the frame word
    static const uint32_t SENDER_ADDR_POSITION = 8;
    //! Byte position of the payload length in the frame word
    static const uint32_t LEN_POSITION = 0;

    //! Constructor 1 (frame word is built without sender address)
    //! @param[in] command  The command byte - should be a value in Command enumeration
    //! @param[in] recipientAddr  The address of the device receiving this command
    //! @param[in] payload  The payload words to set
    //! @param[in] len  Number of words in payload
    inline MaplePacket(uint8_t command,
                       uint8_t recipientAddr,
                       const uint32_t* payload,
                       uint8_t len):
        mFrameWord((len << LEN_POSITION)
                  | (recipientAddr << RECIPIENT_ADDR_POSITION)
                  | (command << COMMAND_POSITION)),
        mPayload(payload, payload + len),
        frameWord(mFrameWord),
        payload(mPayload)
    {}

    //! Constructor 2 (frame word is built without sender address)
    //! @param[in] command  The command byte - should be a value in Command enumeration
    //! @param[in] recipientAddr  The address of the device receiving this command
    //! @param[in] payload  The single payload word to set
    inline MaplePacket(uint8_t command, uint8_t recipientAddr, uint32_t payload):
        mFrameWord((1 << LEN_POSITION)
                  | (recipientAddr << RECIPIENT_ADDR_POSITION)
                  | (command << COMMAND_POSITION)),
        mPayload(&payload, &payload + 1),
        frameWord(mFrameWord),
        payload(mPayload)
    {}

    //! Constructor 3
    //! @param[in] frameWord  The first word to put out on the bus
    //! @param[in] payload  The payload words to set
    //! @param[in] len  Number of words in payload
    inline MaplePacket(uint32_t frameWord, const uint32_t* payload, uint8_t len):
        mFrameWord(frameWord),
        mPayload(payload, payload + len),
        frameWord(mFrameWord),
        payload(mPayload)
    {}

    //! Constructor 4
    //! @param[in] words  All words to set
    //! @param[in] len  Number of words in words (must be at least 1 for frame word to be valid)
    inline MaplePacket(const uint32_t* words, uint8_t len):
        mFrameWord(len > 0 ? words[0] : (COMMAND_INVALID << COMMAND_POSITION)),
        mPayload(&words[1], &words[1] + (len > 1 ? (len - 1) : 0)),
        frameWord(mFrameWord),
        payload(mPayload)
    {}

    //! Copy constructor
    inline MaplePacket(const MaplePacket& rhs) :
        mFrameWord(rhs.mFrameWord),
        mPayload(rhs.mPayload),
        frameWord(mFrameWord),
        payload(mPayload)
    {}

    //! Move constructor
    inline MaplePacket(MaplePacket&& rhs) :
        mFrameWord(rhs.mFrameWord),
        mPayload(std::move(rhs.mPayload)),
        frameWord(mFrameWord),
        payload(mPayload)
    {}

    //! Default constructor - makes an invalid packet
    inline MaplePacket() :
        mFrameWord(COMMAND_INVALID << COMMAND_POSITION),
        mPayload(),
        frameWord(mFrameWord),
        payload(mPayload)
    {}

    //! == operator for this class
    inline bool operator==(const MaplePacket& rhs) const
    {
        return mFrameWord == rhs.mFrameWord && mPayload == rhs.mPayload;
    }

    //! Sets packet contents from array
    //! @param[in] words  All words to set
    //! @param[in] len  Number of words in words (must be at least 1 for frame word to be valid)
    inline void set(const uint32_t* words, uint8_t len)
    {
        mFrameWord = len > 0 ? words[0] : (COMMAND_INVALID << COMMAND_POSITION);
        mPayload.clear();
        mPayload.insert(mPayload.end(), &words[1], &words[1] + (len > 1 ? (len - 1) : 0));
    }

    //! @returns true iff frame word is valid
    inline bool isValid() const
    {
        return (getFrameCommand() != COMMAND_INVALID && getFramePacketLength() == payload.size());
    }

    //! @returns the packet length specified in the frame word
    inline uint8_t getFramePacketLength() const
    {
        return ((frameWord >> LEN_POSITION) & 0xFF);
    }

    //! @returns the sender address specified in the frame word
    inline uint8_t getFrameSenderAddr() const
    {
        return ((frameWord >> SENDER_ADDR_POSITION) & 0xFF);
    }

    //! @returns the recipient address specified in the frame word
    inline uint8_t getFrameRecipientAddr() const
    {
        return ((frameWord >> RECIPIENT_ADDR_POSITION) & 0xFF);
    }

    //! @returns the command specified in the frame word
    inline uint8_t getFrameCommand() const
    {
        return ((frameWord >> COMMAND_POSITION) & 0xFF);
    }

    //! @returns the frame word with an overloaded sender address
    inline uint32_t getFrameWord(uint8_t overloadedSenderAddress) const
    {
        return ((frameWord & ~(0xFF << SENDER_ADDR_POSITION))
                | (overloadedSenderAddress << SENDER_ADDR_POSITION));
    }

    //! @param[in] numPayloadWords  Number of payload words in the packet
    //! @returns number of bits that a packet makes up
    inline static uint32_t getNumTotalBits(uint32_t numPayloadWords)
    {
        // payload size + frame word size + crc byte
        return (((numPayloadWords + 1) * 4 + 1) * 8);
    }

    //! @returns number of bits that this packet makes up
    inline uint32_t getNumTotalBits() const
    {
        return getNumTotalBits(payload.size());
    }

    //! @param[in] numPayloadWords  Number of payload words in the packet
    //! @param[in] nsPerBit  Nanoseconds to transmit each bit
    //! @returns number of nanoseconds it takes to transmit a packet
    inline static uint32_t getTxTimeNs(uint32_t numPayloadWords, uint32_t nsPerBit)
    {
        // Start and stop sequence takes less than 14 bit periods
        return (getNumTotalBits(numPayloadWords) + 14) * nsPerBit;
    }

    //! @returns number of nanoseconds it takes to transmit this packet
    inline uint32_t getTxTimeNs() const
    {
        return getTxTimeNs(payload.size(), MAPLE_NS_PER_BIT);
    }
};

#endif // __MAPLE_PACKET_H__
