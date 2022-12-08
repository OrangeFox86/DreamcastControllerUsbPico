#ifndef DREAMCAST_CONTROLLER_USB_PICO_TEST

#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "configuration.h"

#include "DreamcastMainNode.hpp"
#include "PlayerData.hpp"
#include "CriticalSectionMutex.hpp"
#include "Mutex.hpp"
#include "Clock.hpp"

#include "hal/System/LockGuard.hpp"
#include "hal/MapleBus/MapleBusInterface.hpp"
#include "hal/Usb/usb_interface.hpp"

#include <memory>
#include <algorithm>

#define MAX_DEVICES 4

const uint8_t MAPLE_HOST_ADDRESSES[MAX_DEVICES] = {0x00, 0x40, 0x80, 0xC0};

Mutex cdcRxMutex;
std::vector<char> cdcRx;

class EchoTransmitter : public Transmitter
{
    virtual void txStarted(std::shared_ptr<const Transmission> tx) final
    {}

    virtual void txFailed(bool writeFailed,
                          bool readFailed,
                          std::shared_ptr<const Transmission> tx) final
    {
        if (writeFailed)
        {
            printf("%lu: failed write\n", tx->transmissionId);
        }
        else
        {
            printf("%lu: failed read\n", tx->transmissionId);
        }
    }

    virtual void txComplete(std::shared_ptr<const MaplePacket> packet,
                            std::shared_ptr<const Transmission> tx) final
    {
        printf("%lu: complete {", tx->transmissionId);
        printf("%08lX", packet->frameWord);
        for (std::vector<uint32_t>::const_iterator iter = packet->payload.begin();
             iter != packet->payload.end();
             ++iter)
        {
            printf(" %08lX", *iter);
        }
        printf("}\n");
    }
} echoTransmitter;

// Second Core Process
// The second core is in charge of handling communication with Dreamcast peripherals
void core1()
{
    set_sys_clock_khz(CPU_FREQ_KHZ, true);

    // Wait for steady state
    sleep_ms(100);

    uint32_t numUsbControllers = get_num_usb_controllers();
    uint32_t numDevices = std::min(numUsbControllers, (uint32_t)MAX_DEVICES);

    uint32_t maplePins[MAX_DEVICES] = {
        P1_BUS_START_PIN, P2_BUS_START_PIN, P3_BUS_START_PIN, P4_BUS_START_PIN
    };
    CriticalSectionMutex screenMutexes[numDevices];
    std::shared_ptr<ScreenData> screenData[numDevices];
    std::shared_ptr<PlayerData> playerData[numDevices];
    DreamcastControllerObserver** observers = get_usb_controller_observers();
    std::shared_ptr<MapleBusInterface> buses[numDevices];
    std::shared_ptr<DreamcastMainNode> dreamcastMainNodes[numDevices];
    std::shared_ptr<PrioritizedTxScheduler> schedulers[numDevices];
    Clock clock;
    for (uint32_t i = 0; i < numDevices; ++i)
    {
        screenData[i] = std::make_shared<ScreenData>(screenMutexes[i]);
        playerData[i] = std::make_shared<PlayerData>(i,
                                                     *(observers[i]),
                                                     *screenData[i],
                                                     clock,
                                                     usb_msc_get_file_system());
        buses[i] = create_maple_bus(maplePins[i], MAPLE_HOST_ADDRESSES[i]);
        schedulers[i] = std::make_shared<PrioritizedTxScheduler>(DreamcastMainNode::MAX_PRIORITY);
        dreamcastMainNodes[i] = std::make_shared<DreamcastMainNode>(
            *buses[i],
            *playerData[i],
            schedulers[i]);
    }

    uint32_t lastSize = 0;
    while(true)
    {
        for (uint32_t i = 0; i < numDevices; ++i)
        {
            // Worst execution duration of below is ~350 us at 133 MHz when debug print is disabled
            dreamcastMainNodes[i]->task(time_us_64());
        }

#if 1
        {
            LockGuard lockGuard(cdcRxMutex, true);

            if (lastSize != cdcRx.size())
            {
                std::vector<char>::iterator eol = std::find(cdcRx.begin(), cdcRx.end(), '\n');
                if (eol != cdcRx.end())
                {
                    bool valid = false;
                    std::vector<uint32_t> words;
                    std::vector<char>::iterator iter = cdcRx.begin();
                    while(iter != eol)
                    {
                        uint32_t word = 0;
                        uint32_t i = 0;
                        while (i < 8 && iter != eol)
                        {
                            char v = *iter;
                            if (v >= '0' && v <= '9')
                            {
                                word |= ((v - '0') << ((8 - i) * 4 - 4));
                                ++i;
                            }
                            else if (v >= 'a' && v <= 'f')
                            {
                                word |= ((v - 'a' + 10) << ((8 - i) * 4 - 4));
                                ++i;
                            }
                            else if (v >= 'A' && v <= 'F')
                            {
                                word |= ((v - 'A' + 10) << ((8 - i) * 4 - 4));
                                ++i;
                            }
                            ++iter;
                        }
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
                            uint8_t sender = packet.getFrameSenderAddr();
                            int32_t idx = -1;
                            if (sender == 0 && numDevices > 0)
                            {
                                idx = 0;
                            }
                            else if (sender == 0x40 && numDevices > 1)
                            {
                                idx = 1;
                            }
                            else if (sender == 0x80 && numDevices > 2)
                            {
                                idx = 2;
                            }
                            else if (sender == 0xC0 && numDevices > 3)
                            {
                                idx = 3;
                            }

                            if (idx >= 0)
                            {
                                uint32_t id = schedulers[idx]->add(
                                    0,
                                    PrioritizedTxScheduler::TX_TIME_ASAP,
                                    &echoTransmitter,
                                    packet,
                                    true);
                                std::vector<uint32_t>::iterator iter = words.begin();
                                printf("%lu: added {%08lX", id, *iter++);
                                for(; iter < words.end(); ++iter)
                                {
                                    printf(" %08lX", *iter);
                                }
                                printf("}\n");
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

                    cdcRx.erase(cdcRx.begin(), eol + 1);
                }


                lastSize = cdcRx.size();
            }
        }
#endif
    }
}

// First Core Process
// The first core is in charge of initialization and USB communication
int main()
{
    set_sys_clock_khz(CPU_FREQ_KHZ, true);

#if SHOW_DEBUG_MESSAGES
    stdio_uart_init();
#endif

    multicore_launch_core1(core1);

    Mutex fileMutex;
    Mutex cdcStdioMutex;
    usb_init(&fileMutex, &cdcStdioMutex, &cdcRxMutex, &cdcRx);

    while(true)
    {
        usb_task();
    }
}

#endif
