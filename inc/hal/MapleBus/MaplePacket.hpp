#ifndef __MAPLE_PACKET_H__
#define __MAPLE_PACKET_H__

#include <stdint.h>
#include <vector>
#include <utility>
#include "configuration.h"
#include "dreamcast_constants.h"

// TODO: MaplePacket has gotten waaaay too convoluted - simplify it

struct MaplePacket
{
public:
    //! Deconstructed frame word structure
    struct Frame
    {
        uint8_t length;
        uint8_t senderAddr;
        uint8_t receiverAddr;
        uint8_t command;

        //! Constructor from a frame word
        inline Frame(const uint32_t& frameWord) :
            length(getFramePacketLength(frameWord)),
            senderAddr(getFrameSenderAddr(frameWord)),
            receiverAddr(getFrameRecipientAddr(frameWord)),
            command(getFrameCommand(frameWord))
        {}

        //! Default constructor
        inline Frame() :
            length(0), senderAddr(0), receiverAddr(0), command(COMMAND_INVALID)
        {}

        //! @param[in] frameWord  The frame word to parse
        //! @returns the packet length specified in the given frame word
        static inline uint8_t getFramePacketLength(const uint32_t& frameWord)
        {
            return ((frameWord >> LEN_POSITION) & 0xFF);
        }

        //! @param[in] frameWord  The frame word to parse
        //! @returns the sender address specified in the given frame word
        static inline uint8_t getFrameSenderAddr(const uint32_t& frameWord)
        {
            return ((frameWord >> SENDER_ADDR_POSITION) & 0xFF);
        }

        //! @param[in] frameWord  The frame word to parse
        //! @returns the recipient address specified in the given frame word
        static inline uint8_t getFrameRecipientAddr(const uint32_t& frameWord)
        {
            return ((frameWord >> RECIPIENT_ADDR_POSITION) & 0xFF);
        }

        //! @param[in] frameWord  The frame word to parse
        //! @returns the command specified in the given frame word
        static inline uint8_t getFrameCommand(const uint32_t& frameWord)
        {
            return ((frameWord >> COMMAND_POSITION) & 0xFF);
        }

        //! @returns the accumulated frame word from each of the frame data parts
        inline uint32_t getFrameWord()
        {
            return (static_cast<uint32_t>(length) << LEN_POSITION
                    | static_cast<uint32_t>(senderAddr) << SENDER_ADDR_POSITION
                    | static_cast<uint32_t>(receiverAddr) << RECIPIENT_ADDR_POSITION
                    | static_cast<uint32_t>(command) << COMMAND_POSITION);
        }
    };

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

    //! Resets all data
    inline void reset()
    {
        mFrameWord = COMMAND_INVALID << COMMAND_POSITION;
        mPayload.clear();
    }

    //! Reserves space in payload
    //! @param[in] len  Number of words to reserve
    inline void reservePayload(uint32_t len)
    {
        mPayload.reserve(len);
    }

    //! Sets packet contents from array
    //! @param[in] words  All words to set
    //! @param[in] len  Number of words in words (must be at least 1 for frame word to be valid)
    inline void set(const uint32_t* words, uint8_t len)
    {
        mFrameWord = len > 0 ? words[0] : (COMMAND_INVALID << COMMAND_POSITION);
        mPayload.clear();
        if (len > 1)
        {
            mPayload.insert(mPayload.end(), &words[1], &words[1] + (len - 1));
        }
    }

    //! Append words to payload from array
    //! @param[in] words  Payload words to set
    //! @param[in] len  Number of words in words
    inline void appendPayload(const uint32_t* words, uint8_t len)
    {
        if (len > 0)
        {
            mPayload.insert(mPayload.end(), &words[0], &words[0] + len);
        }
    }

    //! Appends a single word to payload
    //! @param[in] word  The word to append
    inline void appendPayload(uint32_t word)
    {
        appendPayload(&word, 1);
    }

    //! Sets payload from array
    //! @param[in] words  Payload words to set
    //! @param[in] len  Number of words in words
    inline void setPayload(const uint32_t* words, uint8_t len)
    {
        mPayload.clear();
        appendPayload(words, len);
    }

    //! Update length in frame word with the payload size
    void updateFrameLength()
    {
        setFrameByte(LEN_POSITION, 0xFF, mPayload.size());
    }

    //! Sets the sender address of the frame word
    //! @param[in] senderAddress  The sender address to set
    void setSenderAddress(uint8_t senderAddress)
    {
        setFrameByte(SENDER_ADDR_POSITION, 0xFF, senderAddress);
    }

    //! Sets the recipient address of the frame word
    //! @param[in] recipientAddress  The recipient address to set
    void setRecipientAddress(uint8_t recipientAddress)
    {
        setFrameByte(RECIPIENT_ADDR_POSITION, 0xFF, recipientAddress);
    }

    //! Sets the command of the frame word
    //! @param[in] command  The command to set
    void setCommand(uint8_t command)
    {
        setFrameByte(COMMAND_POSITION, 0xFF, command);
    }

    //! @returns true iff frame word is valid
    inline bool isValid() const
    {
        return (getFrameCommand() != COMMAND_INVALID && getFramePacketLength() == payload.size());
    }

    //! @returns the packet length specified in the frame word
    inline uint8_t getFramePacketLength() const
    {
        return Frame::getFramePacketLength(frameWord);
    }

    //! @returns the sender address specified in the frame word
    inline uint8_t getFrameSenderAddr() const
    {
        return Frame::getFrameSenderAddr(frameWord);
    }

    //! @returns the recipient address specified in the frame word
    inline uint8_t getFrameRecipientAddr() const
    {
        return Frame::getFrameRecipientAddr(frameWord);
    }

    //! @returns the command specified in the frame word
    inline uint8_t getFrameCommand() const
    {
        return Frame::getFrameCommand(frameWord);
    }

    //! @returns the frame structure for the packet's frame word
    inline Frame getFrame() const
    {
        return Frame(frameWord);
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

private:
    inline void setFrameByte(uint32_t position, uint32_t mask, uint32_t value)
    {
        mFrameWord = ((frameWord & ~(mask << position)) | (value << position));
    }
};

#endif // __MAPLE_PACKET_H__
