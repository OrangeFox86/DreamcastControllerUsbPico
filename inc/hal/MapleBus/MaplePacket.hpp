#ifndef __MAPLE_PACKET_H__
#define __MAPLE_PACKET_H__

#include <stdint.h>
#include <vector>
#include <utility>
#include "configuration.h"
#include "dreamcast_constants.h"

struct MaplePacket
{
    //! Deconstructed frame word structure
    struct Frame
    {
        //! Command byte
        uint8_t command;
        //! Recipient address byte
        uint8_t recipientAddr;
        //! Sender address byte
        uint8_t senderAddr;
        //! Length of payload in words [0,255]
        uint8_t length;

        //! Byte position of the command in the frame word
        static const uint32_t COMMAND_POSITION = 24;
        //! Byte position of the recipient address in the frame word
        static const uint32_t RECIPIENT_ADDR_POSITION = 16;
        //! Byte position of the sender address in the frame word
        static const uint32_t SENDER_ADDR_POSITION = 8;
        //! Byte position of the payload length in the frame word
        static const uint32_t LEN_POSITION = 0;

        //! Set frame data from word
        inline void setFromFrameWord(uint32_t frameWord)
        {
            length = getFramePacketLength(frameWord);
            senderAddr = getFrameSenderAddr(frameWord);
            recipientAddr = getFrameRecipientAddr(frameWord);
            command = getFrameCommand(frameWord);
        }

        //! Generate a default, invalid frame
        inline static Frame defaultFrame()
        {
            Frame f = {.command=COMMAND_INVALID};
            return f;
        }

        //! Generate a frame from a frame word
        inline static Frame fromWord(uint32_t frameWord)
        {
            Frame f;
            f.setFromFrameWord(frameWord);
            return f;
        }

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
        inline uint32_t toWord() const
        {
            return (static_cast<uint32_t>(length) << LEN_POSITION
                    | static_cast<uint32_t>(senderAddr) << SENDER_ADDR_POSITION
                    | static_cast<uint32_t>(recipientAddr) << RECIPIENT_ADDR_POSITION
                    | static_cast<uint32_t>(command) << COMMAND_POSITION);
        }

        // Assignment operator from uint32 value
        Frame& operator=(const uint32_t& rhs)
        {
            setFromFrameWord(rhs);
            return *this;
        }

        // Assignment operator from int32 value
        Frame& operator=(const int32_t& rhs)
        {
            operator=(static_cast<uint32_t>(rhs));
            return *this;
        }

        //! == operator for this class
        inline bool operator==(const Frame& rhs) const
        {
            return (
                length == rhs.length
                && senderAddr == rhs.senderAddr
                && recipientAddr == rhs.recipientAddr
                && command == rhs.command
            );
        }

        //! @returns true iff frame word is valid
        inline bool isValid() const
        {
            return (command != COMMAND_INVALID);
        }
    };

    //! Constructor 1
    //! @param[in] frame  Frame data to initialize
    //! @param[in] payload  The payload words to set
    //! @param[in] len  Number of words in payload
    inline MaplePacket(Frame frame, const uint32_t* payload, uint8_t len) :
        mPayload(payload, payload + len),
        frame(frame),
        payload(mPayload)
    {
        updateFrameLength();
    }

    //! Constructor 2 (default) - initializes with invalid packet
    inline MaplePacket() :
        MaplePacket(Frame::defaultFrame(), NULL, 0)
    {
        updateFrameLength();
    }

    //! Constructor 3 - initializes with empty payload
    //! @param[in] frame  Frame data to initialize
    inline MaplePacket(Frame frame) :
        MaplePacket(frame, NULL, 0)
    {
        updateFrameLength();
    }

    //! Constructor 4 - initializes with frame and 1 payload word
    //! @param[in] frame  Frame data to initialize
    //! @param[in] payload  The single payload word to set
    inline MaplePacket(Frame frame, uint32_t payload) :
        MaplePacket(frame, &payload, 1)
    {
        updateFrameLength();
    }

    //! Constructor 5
    //! @param[in] words  All words to set
    //! @param[in] len  Number of words in words (must be at least 1 for frame word to be valid)
    inline MaplePacket(const uint32_t* words, uint8_t len) :
        MaplePacket(
            len > 0 ? Frame::fromWord(*words) : Frame::defaultFrame(),
            words + 1,
            len > 0 ? len - 1 : 0)
    {
        updateFrameLength();
    }

    //! Copy constructor
    inline MaplePacket(const MaplePacket& rhs) :
        mPayload(rhs.mPayload),
        frame(rhs.frame),
        payload(mPayload)
    {}

    //! Move constructor
    inline MaplePacket(MaplePacket&& rhs) :
        mPayload(std::move(rhs.mPayload)),
        frame(rhs.frame),
        payload(mPayload)
    {}

    //! == operator for this class
    inline bool operator==(const MaplePacket& rhs) const
    {
        return frame == rhs.frame && payload == rhs.payload;
    }

    //! Resets all data
    inline void reset()
    {
        frame = Frame();
        mPayload.clear();
        updateFrameLength();
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
        if (len > 0)
        {
            frame = words[0];
        }
        else
        {
            frame = Frame();
        }
        mPayload.clear();
        if (len > 1)
        {
            mPayload.insert(mPayload.end(), &words[1], &words[1] + (len - 1));
        }
        updateFrameLength();
    }

    //! Append words to payload from array
    //! @param[in] words  Payload words to set
    //! @param[in] len  Number of words in words
    inline void appendPayload(const uint32_t* words, uint8_t len)
    {
        if (len > 0)
        {
            mPayload.insert(mPayload.end(), &words[0], &words[0] + len);
            updateFrameLength();
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

    //! Sets a single word in payload
    //! @param[in] word  The word to set
    inline void setPayload(uint32_t word)
    {
        setPayload(&word, 1);
    }

    //! Update length in frame word with the payload size
    void updateFrameLength()
    {
        frame.length = payload.size();
    }

    //! @returns true iff frame word is valid
    inline bool isValid() const
    {
        return (frame.isValid() && frame.length == payload.size());
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
    //! Writeable packet payload - external write to payload protected so that length value in frame
    //! word doesn't go out of sync
    std::vector<uint32_t> mPayload;

public:
    //! Packet frame word value
    Frame frame;
    //! Read-only packet payload
    const std::vector<uint32_t>& payload;
};

#endif // __MAPLE_PACKET_H__
