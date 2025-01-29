#include "FlycastCommandParser.hpp"
#include "hal/MapleBus/MaplePacket.hpp"

#include <stdio.h>
#include <cctype>
#include <cstring>
#include <string>
#include <cstdlib>

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
            printf("*failed write\n");
        }
        else
        {
            printf("*failed read\n");
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

FlycastCommandParser::FlycastCommandParser(
    std::shared_ptr<PrioritizedTxScheduler>* schedulers,
    const uint8_t* senderAddresses,
    uint32_t numSenders,
    const std::vector<std::shared_ptr<PlayerData>>& playerData
) :
    mSchedulers(schedulers),
    mSenderAddresses(senderAddresses),
    mNumSenders(numSenders),
    mPlayerData(playerData)
{}

const char* FlycastCommandParser::getCommandChars()
{
    // X is reserved for command from flycast emulator
    return "X";
}

void FlycastCommandParser::submit(const char* chars, uint32_t len)
{
    if (len == 0)
    {
        // This shouldn't happen, but handle it regardless
        return;
    }

    bool valid = false;
    const char* eol = chars + len;
    std::vector<uint32_t> words;
    const char* iter = chars + 1; // Skip past 'X' (implied)

    // left strip
    while (iter < eol && std::isspace(*iter))
    {
        ++iter;
    }
    // right strip
    while (iter < eol && std::isspace(*(eol - 1)))
    {
        --eol;
    }

    // Check for special commanding
    if (iter < eol)
    {
        switch(*iter)
        {
            // Either X- to reset all or one of {X-0, X-1, X-2, X-3} to reset a specific player
            case '-':
            {
                // Remove minus
                ++iter;
                int idx = -1;
                if (iter < eol)
                {
                    std::string number;
                    number.assign(iter, eol - iter);
                    try
                    {
                        idx = std::stoi(number);
                    }
                    catch(...)
                    {
                        // Default to all
                        idx = -1;
                    }
                }

                // Reset screen data
                if (idx < 0)
                {
                    // all
                    for (std::shared_ptr<PlayerData>& playerData : mPlayerData)
                    {
                        playerData->screenData.resetToDefault();
                    }
                }
                else if (static_cast<std::size_t>(idx) < mPlayerData.size())
                {
                    mPlayerData[idx]->screenData.resetToDefault();
                }
            }
            return;

            // No special case
            default: break;
        }
    }

    while (iter < eol)
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

            if (mNumSenders == 1)
            {
                // Single player special case - always send to the one available, regardless of address
                idx = 0;
            }
            else
            {
                for (uint32_t i = 0; i < mNumSenders && idx < 0; ++i, ++senderAddress)
                {
                    if (sender == *senderAddress)
                    {
                        idx = i;
                    }
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
                printf("*failed invalid sender\n");
            }
        }
        else
        {
            printf("*failed packet invalid\n");
        }
    }
    else
    {
        printf("*failed missing data\n");
    }
}

void FlycastCommandParser::printHelp()
{
    printf("X: commands from a flycast emulator\n");
}
