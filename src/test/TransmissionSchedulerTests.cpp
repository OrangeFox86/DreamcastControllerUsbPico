#include "MockedMapleBus.hpp"
#include "MockedDreamcastControllerObserver.hpp"
#include "MockedDreamcastPeripheral.hpp"
#include "MockedMutex.hpp"
#include "MockedUsbController.hpp"

#include "TransmissionScheduler.hpp"

#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::_;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::DoAll;

class MockTransmittionScheduler : public TransmittionScheduler
{
    public:
        const std::list<std::shared_ptr<Transmission>> getSchedule()
        {
            return mSchedule;
        }
};

class TransmissionScheduleTest : public ::testing::Test
{
    public:
        TransmissionScheduleTest() {}

    protected:
        MockTransmittionScheduler scheduler;
};

TEST_F(TransmissionScheduleTest, multiAdd)
{
    // 3
    bool highPriority = false;
    uint64_t txTime = 123;
    MaplePacket packet1(0x55, 0xAA, 0x99887766);
    bool expectResponse = true;
    uint32_t expectedResponseNumPayloadWords = 3;
    uint32_t autoRepeatUs = 16000;
    uint32_t readTimeoutUs = 8675309;
    scheduler.add(highPriority,
                  txTime,
                  packet1,
                  expectResponse,
                  expectedResponseNumPayloadWords,
                  autoRepeatUs,
                  readTimeoutUs);

    // 4
    txTime = 124;
    MaplePacket packet2(0x01, 0x10, 0x11223344);
    scheduler.add(highPriority,
                  txTime,
                  packet2,
                  expectResponse,
                  expectedResponseNumPayloadWords,
                  autoRepeatUs,
                  readTimeoutUs);

    // 1
    // Even though this is scheduled later, it is scheduled to run during all other packets here.
    // Since it is high priority, it takes precidence.
    highPriority = true;
    txTime = 230;
    MaplePacket packet3(0x12, 0x34, 0x56789012);
    scheduler.add(highPriority,
                  txTime,
                  packet3,
                  expectResponse,
                  expectedResponseNumPayloadWords,
                  autoRepeatUs,
                  readTimeoutUs);

    // 2
    highPriority = false;
    txTime = 22;
    MaplePacket packet4(0x35, 0x79, 0x11111111);
    scheduler.add(highPriority,
                  txTime,
                  packet4,
                  expectResponse,
                  expectedResponseNumPayloadWords,
                  autoRepeatUs,
                  readTimeoutUs);

    const std::list<std::shared_ptr<TransmittionScheduler::Transmission>> schedule = scheduler.getSchedule();

    ASSERT_EQ(schedule.size(), 4);
    std::list<std::shared_ptr<TransmittionScheduler::Transmission>>::const_iterator iter = schedule.cbegin();
    EXPECT_EQ((*iter)->packet->getFrameCommand(), 0x12);
    ++iter;
    EXPECT_EQ((*iter)->packet->getFrameCommand(), 0x35);
    ++iter;
    EXPECT_EQ((*iter)->packet->getFrameCommand(), 0x55);
    ++iter;
    EXPECT_EQ((*iter)->packet->getFrameCommand(), 0x01);
}

TEST_F(TransmissionScheduleTest, multiAddBoundary1)
{
    bool highPriority = false;
    uint64_t txTime = 123;
    MaplePacket packet1(0x11, 0xAA, 0x99887766);
    bool expectResponse = true;
    uint32_t expectedResponseNumPayloadWords = 3; // 267 us
    uint32_t autoRepeatUs = 16000;
    uint32_t readTimeoutUs = 8675309;
    scheduler.add(highPriority,
                  txTime,
                  packet1,
                  expectResponse,
                  expectedResponseNumPayloadWords,
                  autoRepeatUs,
                  readTimeoutUs);

    txTime = 124;
    MaplePacket packet2(0x33, 0xAA, 0x99887766);
    scheduler.add(highPriority,
                  txTime,
                  packet2,
                  expectResponse,
                  expectedResponseNumPayloadWords,
                  autoRepeatUs,
                  readTimeoutUs);

    highPriority = true;
    txTime = 123 + 267 + 1;
    MaplePacket packet3(0x22, 0xAA, 0x99887766);
    scheduler.add(highPriority,
                  txTime,
                  packet3,
                  expectResponse,
                  expectedResponseNumPayloadWords,
                  autoRepeatUs,
                  readTimeoutUs);

    const std::list<std::shared_ptr<TransmittionScheduler::Transmission>> schedule = scheduler.getSchedule();

    ASSERT_EQ(schedule.size(), 3);
    std::list<std::shared_ptr<TransmittionScheduler::Transmission>>::const_iterator iter = schedule.cbegin();
    EXPECT_EQ((*iter)->packet->getFrameCommand(), 0x11);
    ++iter;
    EXPECT_EQ((*iter)->packet->getFrameCommand(), 0x22);
    ++iter;
    EXPECT_EQ((*iter)->packet->getFrameCommand(), 0x33);
}

TEST_F(TransmissionScheduleTest, multiAddBoundary2)
{
    bool highPriority = true;
    uint64_t txTime = 123 + 267 + 1;
    MaplePacket packet1(0x22, 0xAA, 0x99887766);
    bool expectResponse = true;
    uint32_t expectedResponseNumPayloadWords = 3; // 267 us
    uint32_t autoRepeatUs = 16000;
    uint32_t readTimeoutUs = 8675309;
    scheduler.add(highPriority,
                  txTime,
                  packet1,
                  expectResponse,
                  expectedResponseNumPayloadWords,
                  autoRepeatUs,
                  readTimeoutUs);

    highPriority = false;
    txTime = 123;
    MaplePacket packet2(0x11, 0xAA, 0x99887766);
    scheduler.add(highPriority,
                  txTime,
                  packet2,
                  expectResponse,
                  expectedResponseNumPayloadWords,
                  autoRepeatUs,
                  readTimeoutUs);

    txTime = 124;
    MaplePacket packet3(0x33, 0xAA, 0x99887766);
    scheduler.add(highPriority,
                  txTime,
                  packet3,
                  expectResponse,
                  expectedResponseNumPayloadWords,
                  autoRepeatUs,
                  readTimeoutUs);

    const std::list<std::shared_ptr<TransmittionScheduler::Transmission>> schedule = scheduler.getSchedule();

    ASSERT_EQ(schedule.size(), 3);
    std::list<std::shared_ptr<TransmittionScheduler::Transmission>>::const_iterator iter = schedule.cbegin();
    EXPECT_EQ((*iter)->packet->getFrameCommand(), 0x11);
    ++iter;
    EXPECT_EQ((*iter)->packet->getFrameCommand(), 0x22);
    ++iter;
    EXPECT_EQ((*iter)->packet->getFrameCommand(), 0x33);
}
