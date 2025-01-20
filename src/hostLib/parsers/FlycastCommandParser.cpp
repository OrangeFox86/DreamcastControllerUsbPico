#include "FlycastCommandParser.hpp"
#include "hal/MapleBus/MaplePacket.hpp"

#include <stdio.h>

// Simple definition of a transmitter which just echos status and received data
class FlycastEchoTransmitter : public Transmitter
{
public:
    virtual void txStarted(std::shared_ptr<const Transmission> tx) final
    {}

    virtual void txFailed(bool writeFailed,
                          bool readFailed,
                          std::shared_ptr<const Transmission> tx) final
    {
        if (writeFailed)
        {
            printf("%lu: failed write\n", (long unsigned int)tx->transmissionId);
        }
        else
        {
            printf("%lu: failed read\n", (long unsigned int)tx->transmissionId);
        }
    }

    virtual void txComplete(std::shared_ptr<const MaplePacket> packet,
                            std::shared_ptr<const Transmission> tx) final
    {
        char buffer[9];
        snprintf(buffer, sizeof(buffer), "%08lX", (long unsigned int)packet->frame.toWord());
        for (int i = 0; i < 8; i += 2)
        {
            printf(" %c%c", buffer[i], buffer[i + 1]);
        }
        printf("\n");
    }
} flycastEchoTransmitter;

FlycastCommandParser::FlycastCommandParser(std::shared_ptr<PrioritizedTxScheduler>* schedulers,
                                                             const uint8_t* senderAddresses,
                                                             uint32_t numSenders) :
    mSchedulers(schedulers),
    mSenderAddresses(senderAddresses),
    mNumSenders(numSenders)
{}

const char* FlycastCommandParser::getCommandChars()
{
    // X is reserved for command from flycast emulator
    return "X";
}

void FlycastCommandParser::submit(const char* chars, uint32_t len)
{
    bool valid = false;
    const char* const eol = chars + len;
    std::vector<uint32_t> words;
    const char* iter = chars + 1; // Skip past 'X' (implied)
    while(iter < eol)
    {
        uint32_t word = 0;
        uint32_t i = 0;
        while (i < 8 && iter < eol)
        {
            char v = *iter++;
            uint_fast8_t value = 0;

            if (v >= '0' && v <= '9')
            {
                value = v - '0';
            }
            else if (v >= 'a' && v <= 'f')
            {
                value = v - 'a' + 0xa;
            }
            else if (v >= 'A' && v <= 'F')
            {
                value = v - 'A' + 0xA;
            }
            else
            {
                // Ignore this character
                continue;
            }

            // Apply value into current word
            word |= (value << ((8 - i) * 4 - 4));
            ++i;
        }

        // Invalid if a partial word was given
        valid = ((i == 8) || (i == 0));

        if (i == 8)
        {
            words.push_back(word);
        }
    }

    if (valid)
    {
        MaplePacket packet(&words[0], words.size());
        if (packet.isValid())
        {
            uint8_t sender = packet.frame.senderAddr;
            int32_t idx = -1;
            const uint8_t* senderAddress = mSenderAddresses;

            for (uint32_t i = 0; i < mNumSenders && idx < 0; ++i, ++senderAddress)
            {
                if (sender == *senderAddress)
                {
                    idx = i;
                }
            }

            if (idx >= 0)
            {
                mSchedulers[idx]->add(
                    PrioritizedTxScheduler::EXTERNAL_TRANSMISSION_PRIORITY,
                    PrioritizedTxScheduler::TX_TIME_ASAP,
                    &flycastEchoTransmitter,
                    packet,
                    true);
            }
            else
            {
                printf("0: failed invalid sender\n");
            }
        }
        else
        {
            printf("0: failed packet invalid\n");
        }
    }
    else
    {
        printf("0: failed missing data\n");
    }
}

void FlycastCommandParser::printHelp()
{
    printf("X<cmd>: cmd as ASCII hex value to send to maple bus without CRC\n");
}
