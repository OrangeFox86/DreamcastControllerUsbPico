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
        std::list<std::shared_ptr<Transmission>>& getSchedule()
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
    uint32_t id1 = scheduler.add(highPriority,
                                 txTime,
                                 packet1,
                                 expectResponse,
                                 expectedResponseNumPayloadWords,
                                 autoRepeatUs,
                                 readTimeoutUs);
    EXPECT_EQ(id1, 0);

    // 4
    txTime = 124;
    MaplePacket packet2(0x01, 0x10, 0x11223344);
    uint32_t id2 = scheduler.add(highPriority,
                                 txTime,
                                 packet2,
                                 expectResponse,
                                 expectedResponseNumPayloadWords,
                                 autoRepeatUs,
                                 readTimeoutUs);
    EXPECT_EQ(id2, 1);

    // 1
    // Even though this is scheduled later, it is scheduled to run during all other packets here.
    // Since it is high priority, it takes precidence.
    highPriority = true;
    txTime = 230;
    MaplePacket packet3(0x12, 0x34, 0x56789012);
    uint32_t id3 = scheduler.add(highPriority,
                                 txTime,
                                 packet3,
                                 expectResponse,
                                 expectedResponseNumPayloadWords,
                                 autoRepeatUs,
                                 readTimeoutUs);
    EXPECT_EQ(id3, 2);

    // 2
    highPriority = false;
    txTime = 22;
    MaplePacket packet4(0x35, 0x79, 0x11111111);
    uint32_t id4 = scheduler.add(highPriority,
                                 txTime,
                                 packet4,
                                 expectResponse,
                                 expectedResponseNumPayloadWords,
                                 autoRepeatUs,
                                 readTimeoutUs);
    EXPECT_EQ(id4, 3);

    const std::list<std::shared_ptr<TransmittionScheduler::Transmission>> schedule = scheduler.getSchedule();

    ASSERT_EQ(schedule.size(), 4);
    std::list<std::shared_ptr<TransmittionScheduler::Transmission>>::const_iterator iter = schedule.cbegin();
    EXPECT_EQ((*iter)->transmissionId, 2);
    ++iter;
    EXPECT_EQ((*iter)->transmissionId, 3);
    ++iter;
    EXPECT_EQ((*iter)->transmissionId, 0);
    ++iter;
    EXPECT_EQ((*iter)->transmissionId, 1);
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
    uint32_t id1 = scheduler.add(highPriority,
                                 txTime,
                                 packet1,
                                 expectResponse,
                                 expectedResponseNumPayloadWords,
                                 autoRepeatUs,
                                 readTimeoutUs);
    EXPECT_EQ(id1, 0);

    txTime = 124;
    MaplePacket packet2(0x33, 0xAA, 0x99887766);
    uint32_t id2 = scheduler.add(highPriority,
                                 txTime,
                                 packet2,
                                 expectResponse,
                                 expectedResponseNumPayloadWords,
                                 autoRepeatUs,
                                 readTimeoutUs);
    EXPECT_EQ(id2, 1);

    highPriority = true;
    txTime = 123 + 267 + 1;
    MaplePacket packet3(0x22, 0xAA, 0x99887766);
    uint32_t id3 = scheduler.add(highPriority,
                                 txTime,
                                 packet3,
                                 expectResponse,
                                 expectedResponseNumPayloadWords,
                                 autoRepeatUs,
                                 readTimeoutUs);
    EXPECT_EQ(id3, 2);

    const std::list<std::shared_ptr<TransmittionScheduler::Transmission>> schedule = scheduler.getSchedule();

    ASSERT_EQ(schedule.size(), 3);
    std::list<std::shared_ptr<TransmittionScheduler::Transmission>>::const_iterator iter = schedule.cbegin();
    EXPECT_EQ((*iter)->transmissionId, 0);
    ++iter;
    EXPECT_EQ((*iter)->transmissionId, 2);
    ++iter;
    EXPECT_EQ((*iter)->transmissionId, 1);
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
    uint32_t id1 = scheduler.add(highPriority,
                                 txTime,
                                 packet1,
                                 expectResponse,
                                 expectedResponseNumPayloadWords,
                                 autoRepeatUs,
                                 readTimeoutUs);
    EXPECT_EQ(id1, 0);

    highPriority = false;
    txTime = 123;
    MaplePacket packet2(0x11, 0xAA, 0x99887766);
    uint32_t id2 = scheduler.add(highPriority,
                                 txTime,
                                 packet2,
                                 expectResponse,
                                 expectedResponseNumPayloadWords,
                                 autoRepeatUs,
                                 readTimeoutUs);
    EXPECT_EQ(id2, 1);

    txTime = 124;
    MaplePacket packet3(0x33, 0xAA, 0x99887766);
    uint32_t id3 = scheduler.add(highPriority,
                                 txTime,
                                 packet3,
                                 expectResponse,
                                 expectedResponseNumPayloadWords,
                                 autoRepeatUs,
                                 readTimeoutUs);
    EXPECT_EQ(id3, 2);

    const std::list<std::shared_ptr<TransmittionScheduler::Transmission>> schedule = scheduler.getSchedule();

    ASSERT_EQ(schedule.size(), 3);
    std::list<std::shared_ptr<TransmittionScheduler::Transmission>>::const_iterator iter = schedule.cbegin();
    EXPECT_EQ((*iter)->transmissionId, 1);
    ++iter;
    EXPECT_EQ((*iter)->transmissionId, 0);
    ++iter;
    EXPECT_EQ((*iter)->transmissionId, 2);
}

class TransmissionSchedulePopTest : public TransmissionScheduleTest
{
    public:
        TransmissionSchedulePopTest() {}

    protected:
        MockTransmittionScheduler scheduler;

        virtual void SetUp()
        {
            bool highPriority = false;
            uint64_t txTime = 1;
            MaplePacket packet1(0x11, 0x01, 0x99887766);
            bool expectResponse = true;
            uint32_t expectedResponseNumPayloadWords = 0;
            uint32_t autoRepeatUs = 0;
            uint32_t readTimeoutUs = 8675309;
            scheduler.add(highPriority,
                          txTime,
                          packet1,
                          expectResponse,
                          expectedResponseNumPayloadWords,
                          autoRepeatUs,
                          readTimeoutUs);
            txTime = 2;
            autoRepeatUs = 16000;
            MaplePacket packet2(0x22, 0x02, 0x99887766);
            scheduler.add(highPriority,
                          txTime,
                          packet2,
                          expectResponse,
                          expectedResponseNumPayloadWords,
                          autoRepeatUs,
                          readTimeoutUs);
            txTime = 3;
            MaplePacket packet3(0x33, 0x03, 0x99887766);
            scheduler.add(highPriority,
                          txTime,
                          packet3,
                          expectResponse,
                          expectedResponseNumPayloadWords,
                          autoRepeatUs,
                          readTimeoutUs);
        }
};

TEST_F(TransmissionSchedulePopTest, popTestNull)
{
    EXPECT_EQ(scheduler.popNext(0), nullptr);

    const std::list<std::shared_ptr<TransmittionScheduler::Transmission>> schedule = scheduler.getSchedule();

    ASSERT_EQ(schedule.size(), 3);
    std::list<std::shared_ptr<TransmittionScheduler::Transmission>>::const_iterator iter = schedule.cbegin();
    EXPECT_EQ((*iter)->transmissionId, 0);
    ++iter;
    EXPECT_EQ((*iter)->transmissionId, 1);
    ++iter;
    EXPECT_EQ((*iter)->transmissionId, 2);
}

TEST_F(TransmissionSchedulePopTest, popTest1)
{
    std::shared_ptr<const TransmittionScheduler::Transmission> item = scheduler.popNext(1);
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->packet->getFrameCommand(), 0x11);
    item = scheduler.popNext(2);
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->packet->getFrameCommand(), 0x22);

    const std::list<std::shared_ptr<TransmittionScheduler::Transmission>> schedule = scheduler.getSchedule();

    ASSERT_EQ(schedule.size(), 2);
    std::list<std::shared_ptr<TransmittionScheduler::Transmission>>::const_iterator iter = schedule.cbegin();
    EXPECT_EQ((*iter)->transmissionId, 2);
    ++iter;
    // This one should auto reload
    EXPECT_EQ((*iter)->transmissionId, 1);
    EXPECT_EQ((*iter)->nextTxTimeUs, 16002);
}

class TransmissionScheduleCancelTest : public TransmissionSchedulePopTest
{
    public:
        TransmissionScheduleCancelTest() {}
};

TEST_F(TransmissionScheduleCancelTest, cancelByIdNotFound)
{
    EXPECT_EQ(scheduler.cancelById(100), 0);

    const std::list<std::shared_ptr<TransmittionScheduler::Transmission>> schedule = scheduler.getSchedule();

    ASSERT_EQ(schedule.size(), 3);
    std::list<std::shared_ptr<TransmittionScheduler::Transmission>>::const_iterator iter = schedule.cbegin();
    EXPECT_EQ((*iter)->transmissionId, 0);
    ++iter;
    EXPECT_EQ((*iter)->transmissionId, 1);
    ++iter;
    EXPECT_EQ((*iter)->transmissionId, 2);
}

TEST_F(TransmissionScheduleCancelTest, cancelByIdFound)
{
    EXPECT_EQ(scheduler.cancelById(1), 1);

    const std::list<std::shared_ptr<TransmittionScheduler::Transmission>> schedule = scheduler.getSchedule();

    ASSERT_EQ(schedule.size(), 2);
    std::list<std::shared_ptr<TransmittionScheduler::Transmission>>::const_iterator iter = schedule.cbegin();
    EXPECT_EQ((*iter)->transmissionId, 0);
    ++iter;
    EXPECT_EQ((*iter)->transmissionId, 2);
}

TEST_F(TransmissionScheduleCancelTest, cancelByRecipientNotFound)
{
    EXPECT_EQ(scheduler.cancelByRecipient(100), 0);

    const std::list<std::shared_ptr<TransmittionScheduler::Transmission>> schedule = scheduler.getSchedule();

    ASSERT_EQ(schedule.size(), 3);
    std::list<std::shared_ptr<TransmittionScheduler::Transmission>>::const_iterator iter = schedule.cbegin();
    EXPECT_EQ((*iter)->transmissionId, 0);
    ++iter;
    EXPECT_EQ((*iter)->transmissionId, 1);
    ++iter;
    EXPECT_EQ((*iter)->transmissionId, 2);
}

TEST_F(TransmissionScheduleCancelTest, cancelByRecipientFound)
{
    EXPECT_EQ(scheduler.cancelByRecipient(0x03), 1);

    const std::list<std::shared_ptr<TransmittionScheduler::Transmission>> schedule = scheduler.getSchedule();

    ASSERT_EQ(schedule.size(), 2);
    std::list<std::shared_ptr<TransmittionScheduler::Transmission>>::const_iterator iter = schedule.cbegin();
    EXPECT_EQ((*iter)->transmissionId, 0);
    ++iter;
    EXPECT_EQ((*iter)->transmissionId, 1);
}

TEST_F(TransmissionScheduleCancelTest, cancelAll)
{
    EXPECT_EQ(scheduler.cancelAll(), 3);

    const std::list<std::shared_ptr<TransmittionScheduler::Transmission>> schedule = scheduler.getSchedule();

    ASSERT_EQ(schedule.size(), 0);
}
