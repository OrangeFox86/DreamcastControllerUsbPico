#include "MockMapleBus.hpp"
#include "MockDreamcastControllerObserver.hpp"
#include "MockDreamcastPeripheral.hpp"
#include "MockMutex.hpp"
#include "MockUsbController.hpp"

#include "PrioritizedTxScheduler.hpp"

#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::_;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::DoAll;

class PrioritizedTxSchedulerUnitTest : public PrioritizedTxScheduler
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
        PrioritizedTxSchedulerUnitTest scheduler;
};

TEST_F(TransmissionScheduleTest, multiAdd)
{
    // 3
    uint8_t priority = 255;
    uint64_t txTime = 123;
    MaplePacket packet1(0x55, 0xAA, 0x99887766);
    bool expectResponse = true;
    uint32_t expectedResponseNumPayloadWords = 3;
    uint32_t autoRepeatUs = 16000;
    uint32_t readTimeoutUs = 8675309;
    uint32_t id1 = scheduler.add(priority,
                                 txTime,
                                 packet1,
                                 expectResponse,
                                 expectedResponseNumPayloadWords,
                                 autoRepeatUs,
                                 readTimeoutUs);
    EXPECT_EQ(id1, 1);

    // 4
    txTime = 124;
    MaplePacket packet2(0x01, 0x10, 0x11223344);
    uint32_t id2 = scheduler.add(priority,
                                 txTime,
                                 packet2,
                                 expectResponse,
                                 expectedResponseNumPayloadWords,
                                 autoRepeatUs,
                                 readTimeoutUs);
    EXPECT_EQ(id2, 2);

    // 1
    // Even though this is scheduled later, it is scheduled to run during all other packets here.
    // Since it is high priority, it takes precidence.
    priority = 0;
    txTime = 230;
    MaplePacket packet3(0x12, 0x34, 0x56789012);
    uint32_t id3 = scheduler.add(priority,
                                 txTime,
                                 packet3,
                                 expectResponse,
                                 expectedResponseNumPayloadWords,
                                 autoRepeatUs,
                                 readTimeoutUs);
    EXPECT_EQ(id3, 3);

    // 2
    priority = 255;
    txTime = 22;
    MaplePacket packet4(0x35, 0x79, 0x11111111);
    uint32_t id4 = scheduler.add(priority,
                                 txTime,
                                 packet4,
                                 expectResponse,
                                 expectedResponseNumPayloadWords,
                                 autoRepeatUs,
                                 readTimeoutUs);
    EXPECT_EQ(id4, 4);

    const std::list<std::shared_ptr<PrioritizedTxScheduler::Transmission>> schedule = scheduler.getSchedule();

    ASSERT_EQ(schedule.size(), 4);
    std::list<std::shared_ptr<PrioritizedTxScheduler::Transmission>>::const_iterator iter = schedule.cbegin();
    EXPECT_EQ((*iter++)->transmissionId, 3);
    EXPECT_EQ((*iter++)->transmissionId, 4);
    EXPECT_EQ((*iter++)->transmissionId, 1);
    EXPECT_EQ((*iter++)->transmissionId, 2);
}

TEST_F(TransmissionScheduleTest, multiAddBoundary1)
{
    uint8_t priority = 255;
    uint64_t txTime = 123;
    MaplePacket packet1(0x11, 0xAA, 0x99887766);
    bool expectResponse = true;
    uint32_t expectedResponseNumPayloadWords = 3; // 267 us
    uint32_t autoRepeatUs = 16000;
    uint32_t readTimeoutUs = 8675309;
    uint32_t id1 = scheduler.add(priority,
                                 txTime,
                                 packet1,
                                 expectResponse,
                                 expectedResponseNumPayloadWords,
                                 autoRepeatUs,
                                 readTimeoutUs);
    EXPECT_EQ(id1, 1);

    txTime = 124;
    MaplePacket packet2(0x33, 0xAA, 0x99887766);
    uint32_t id2 = scheduler.add(priority,
                                 txTime,
                                 packet2,
                                 expectResponse,
                                 expectedResponseNumPayloadWords,
                                 autoRepeatUs,
                                 readTimeoutUs);
    EXPECT_EQ(id2, 2);

    priority = 0;
    txTime = 123 + 267;
    MaplePacket packet3(0x22, 0xAA, 0x99887766);
    uint32_t id3 = scheduler.add(priority,
                                 txTime,
                                 packet3,
                                 expectResponse,
                                 expectedResponseNumPayloadWords,
                                 autoRepeatUs,
                                 readTimeoutUs);
    EXPECT_EQ(id3, 3);

    const std::list<std::shared_ptr<PrioritizedTxScheduler::Transmission>> schedule = scheduler.getSchedule();

    ASSERT_EQ(schedule.size(), 3);
    std::list<std::shared_ptr<PrioritizedTxScheduler::Transmission>>::const_iterator iter = schedule.cbegin();
    EXPECT_EQ((*iter++)->transmissionId, 1);
    EXPECT_EQ((*iter++)->transmissionId, 3);
    EXPECT_EQ((*iter++)->transmissionId, 2);
}

TEST_F(TransmissionScheduleTest, multiAddBoundary2)
{
    bool priority = 0;
    uint64_t txTime = 123 + 267 + 1;
    MaplePacket packet1(0x22, 0xAA, 0x99887766);
    bool expectResponse = true;
    uint32_t expectedResponseNumPayloadWords = 3; // 267 us
    uint32_t autoRepeatUs = 16000;
    uint32_t readTimeoutUs = 8675309;
    uint32_t id1 = scheduler.add(priority,
                                 txTime,
                                 packet1,
                                 expectResponse,
                                 expectedResponseNumPayloadWords,
                                 autoRepeatUs,
                                 readTimeoutUs);
    EXPECT_EQ(id1, 1);

    priority = 255;
    txTime = 123;
    MaplePacket packet2(0x11, 0xAA, 0x99887766);
    uint32_t id2 = scheduler.add(priority,
                                 txTime,
                                 packet2,
                                 expectResponse,
                                 expectedResponseNumPayloadWords,
                                 autoRepeatUs,
                                 readTimeoutUs);
    EXPECT_EQ(id2, 2);

    txTime = 124;
    MaplePacket packet3(0x33, 0xAA, 0x99887766);
    uint32_t id3 = scheduler.add(priority,
                                 txTime,
                                 packet3,
                                 expectResponse,
                                 expectedResponseNumPayloadWords,
                                 autoRepeatUs,
                                 readTimeoutUs);
    EXPECT_EQ(id3, 3);

    const std::list<std::shared_ptr<PrioritizedTxScheduler::Transmission>> schedule = scheduler.getSchedule();

    ASSERT_EQ(schedule.size(), 3);
    std::list<std::shared_ptr<PrioritizedTxScheduler::Transmission>>::const_iterator iter = schedule.cbegin();
    EXPECT_EQ((*iter++)->transmissionId, 2);
    EXPECT_EQ((*iter++)->transmissionId, 1);
    EXPECT_EQ((*iter++)->transmissionId, 3);
}

TEST_F(TransmissionScheduleTest, multiAddBoundary3)
{
    uint8_t priority = 255;
    uint64_t txTime = 1;
    MaplePacket packet1(0x11, 0x01, 0x99887766);
    bool expectResponse = true;
    uint32_t expectedResponseNumPayloadWords = 10;
    uint32_t autoRepeatUs = 0;
    uint32_t readTimeoutUs = 8675309;
    uint32_t id1 = scheduler.add(priority,
                    txTime,
                    packet1,
                    expectResponse,
                    expectedResponseNumPayloadWords,
                    autoRepeatUs,
                    readTimeoutUs);
    EXPECT_EQ(id1, 1);

    txTime = 2;
    expectedResponseNumPayloadWords = 3; // 267 us
    MaplePacket packet2(0x22, 0x02, 0x99887766);
    uint32_t id2 = scheduler.add(priority,
                    txTime,
                    packet2,
                    expectResponse,
                    expectedResponseNumPayloadWords,
                    autoRepeatUs,
                    readTimeoutUs);
    EXPECT_EQ(id2, 2);

    priority = 0;
    txTime = 2 + 300;
    expectedResponseNumPayloadWords = 0;
    MaplePacket packet3(0x33, 0x02, 0x99887766);
    uint32_t id3 = scheduler.add(priority,
                    txTime,
                    packet3,
                    expectResponse,
                    expectedResponseNumPayloadWords,
                    autoRepeatUs,
                    readTimeoutUs);
    EXPECT_EQ(id3, 3);

    const std::list<std::shared_ptr<PrioritizedTxScheduler::Transmission>> schedule = scheduler.getSchedule();

    ASSERT_EQ(schedule.size(), 3);
    std::list<std::shared_ptr<PrioritizedTxScheduler::Transmission>>::const_iterator iter = schedule.cbegin();
    EXPECT_EQ((*iter++)->transmissionId, 3);
    EXPECT_EQ((*iter++)->transmissionId, 1);
    EXPECT_EQ((*iter++)->transmissionId, 2);
}

class TransmissionSchedulePopTestA : public TransmissionScheduleTest
{
    public:
        TransmissionSchedulePopTestA() {}

    protected:
        virtual void SetUp()
        {
            uint8_t priority = 255;
            uint64_t txTime = 1;
            MaplePacket packet1(0x11, 0x01, 0x99887766);
            bool expectResponse = true;
            uint32_t expectedResponseNumPayloadWords = 0;
            uint32_t autoRepeatUs = 0;
            uint32_t readTimeoutUs = 8675309;
            scheduler.add(priority,
                          txTime,
                          packet1,
                          expectResponse,
                          expectedResponseNumPayloadWords,
                          autoRepeatUs,
                          readTimeoutUs);
            txTime = 2;
            autoRepeatUs = 16000;
            MaplePacket packet2(0x22, 0x02, 0x99887766);
            scheduler.add(priority,
                          txTime,
                          packet2,
                          expectResponse,
                          expectedResponseNumPayloadWords,
                          autoRepeatUs,
                          readTimeoutUs);
            txTime = 3;
            MaplePacket packet3(0x33, 0x02, 0x99887766);
            scheduler.add(priority,
                          txTime,
                          packet3,
                          expectResponse,
                          expectedResponseNumPayloadWords,
                          autoRepeatUs,
                          readTimeoutUs);
        }
};

TEST_F(TransmissionSchedulePopTestA, popTestNull)
{
    EXPECT_EQ(scheduler.popNext(0), nullptr);

    const std::list<std::shared_ptr<PrioritizedTxScheduler::Transmission>> schedule = scheduler.getSchedule();

    ASSERT_EQ(schedule.size(), 3);
    std::list<std::shared_ptr<PrioritizedTxScheduler::Transmission>>::const_iterator iter = schedule.cbegin();
    EXPECT_EQ((*iter++)->transmissionId, 1);
    EXPECT_EQ((*iter++)->transmissionId, 2);
    EXPECT_EQ((*iter++)->transmissionId, 3);
}

TEST_F(TransmissionSchedulePopTestA, popTestAutoReload1)
{
    std::shared_ptr<const PrioritizedTxScheduler::Transmission> item = scheduler.popNext(1);
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->packet->getFrameCommand(), 0x11);
    item = scheduler.popNext(2);
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->packet->getFrameCommand(), 0x22);

    const std::list<std::shared_ptr<PrioritizedTxScheduler::Transmission>> schedule = scheduler.getSchedule();

    ASSERT_EQ(schedule.size(), 2);
    std::list<std::shared_ptr<PrioritizedTxScheduler::Transmission>>::const_iterator iter = schedule.cbegin();
    EXPECT_EQ((*iter++)->transmissionId, 3);
    // This one should auto reload
    EXPECT_EQ((*iter)->transmissionId, 2);
    EXPECT_EQ((*iter)->nextTxTimeUs, 16002);
}

TEST_F(TransmissionSchedulePopTestA, popTestAutoReload2)
{
    std::shared_ptr<const PrioritizedTxScheduler::Transmission> item = scheduler.popNext(1);
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->packet->getFrameCommand(), 0x11);
    item = scheduler.popNext(16003);
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->packet->getFrameCommand(), 0x22);

    const std::list<std::shared_ptr<PrioritizedTxScheduler::Transmission>> schedule = scheduler.getSchedule();

    ASSERT_EQ(schedule.size(), 2);
    std::list<std::shared_ptr<PrioritizedTxScheduler::Transmission>>::const_iterator iter = schedule.cbegin();
    EXPECT_EQ((*iter++)->transmissionId, 3);
    // This one should auto reload
    EXPECT_EQ((*iter)->transmissionId, 2);
    // The one at 16002 was missed, so it should reload to the one after
    EXPECT_EQ((*iter)->nextTxTimeUs, 32002);
}

class TransmissionSchedulePopTestB : public TransmissionScheduleTest
{
    public:
        TransmissionSchedulePopTestB() {}

    protected:
        virtual void SetUp()
        {
            uint8_t priority = 255;
            uint64_t txTime = 1;
            MaplePacket packet1(0x11, 0x01, 0x99887766);
            bool expectResponse = true;
            uint32_t expectedResponseNumPayloadWords = 10;
            uint32_t autoRepeatUs = 0;
            uint32_t readTimeoutUs = 8675309;
            scheduler.add(priority,
                          txTime,
                          packet1,
                          expectResponse,
                          expectedResponseNumPayloadWords,
                          autoRepeatUs,
                          readTimeoutUs);
            priority = 0;
            txTime = 2 + 300;
            expectedResponseNumPayloadWords = 0;
            MaplePacket packet3(0x22, 0x02, 0x99887766);
            scheduler.add(priority,
                          txTime,
                          packet3,
                          expectResponse,
                          expectedResponseNumPayloadWords,
                          autoRepeatUs,
                          readTimeoutUs);
            priority = 255;
            txTime = 2;
            expectedResponseNumPayloadWords = 3; // 267 us
            MaplePacket packet2(0x33, 0x02, 0x99887766);
            autoRepeatUs = 1111;
            scheduler.add(priority,
                          txTime,
                          packet2,
                          expectResponse,
                          expectedResponseNumPayloadWords,
                          autoRepeatUs,
                          readTimeoutUs);
        }
};

TEST_F(TransmissionSchedulePopTestB, popTestAutoReload2)
{
    // Transmission 2 should be bumped up to be executed before 0 because 0 yeileded to 1
    std::shared_ptr<const PrioritizedTxScheduler::Transmission> item = scheduler.popNext(2);
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->transmissionId, 3);
    EXPECT_EQ(item->nextTxTimeUs, 1113);

    // Transmission should auto reload
    const std::list<std::shared_ptr<PrioritizedTxScheduler::Transmission>> schedule = scheduler.getSchedule();
    ASSERT_EQ(schedule.size(), 3);
}

class TransmissionScheduleCancelTest : public TransmissionSchedulePopTestA
{
    public:
        TransmissionScheduleCancelTest() {}
};

TEST_F(TransmissionScheduleCancelTest, cancelByIdNotFound)
{
    EXPECT_EQ(scheduler.cancelById(100), 0);

    const std::list<std::shared_ptr<PrioritizedTxScheduler::Transmission>> schedule = scheduler.getSchedule();

    ASSERT_EQ(schedule.size(), 3);
    std::list<std::shared_ptr<PrioritizedTxScheduler::Transmission>>::const_iterator iter = schedule.cbegin();
    EXPECT_EQ((*iter++)->transmissionId, 1);
    EXPECT_EQ((*iter++)->transmissionId, 2);
    EXPECT_EQ((*iter++)->transmissionId, 3);
}

TEST_F(TransmissionScheduleCancelTest, cancelByIdFound)
{
    EXPECT_EQ(scheduler.cancelById(2), 1);

    const std::list<std::shared_ptr<PrioritizedTxScheduler::Transmission>> schedule = scheduler.getSchedule();

    ASSERT_EQ(schedule.size(), 2);
    std::list<std::shared_ptr<PrioritizedTxScheduler::Transmission>>::const_iterator iter = schedule.cbegin();
    EXPECT_EQ((*iter++)->transmissionId, 1);
    EXPECT_EQ((*iter++)->transmissionId, 3);
}

TEST_F(TransmissionScheduleCancelTest, cancelByRecipientNotFound)
{
    EXPECT_EQ(scheduler.cancelByRecipient(100), 0);

    const std::list<std::shared_ptr<PrioritizedTxScheduler::Transmission>> schedule = scheduler.getSchedule();

    ASSERT_EQ(schedule.size(), 3);
    std::list<std::shared_ptr<PrioritizedTxScheduler::Transmission>>::const_iterator iter = schedule.cbegin();
    EXPECT_EQ((*iter++)->transmissionId, 1);
    EXPECT_EQ((*iter++)->transmissionId, 2);
    EXPECT_EQ((*iter++)->transmissionId, 3);
}

TEST_F(TransmissionScheduleCancelTest, cancelByRecipientFound)
{
    EXPECT_EQ(scheduler.cancelByRecipient(0x02), 2);

    const std::list<std::shared_ptr<PrioritizedTxScheduler::Transmission>> schedule = scheduler.getSchedule();

    ASSERT_EQ(schedule.size(), 1);
    std::list<std::shared_ptr<PrioritizedTxScheduler::Transmission>>::const_iterator iter = schedule.cbegin();
    EXPECT_EQ((*iter++)->transmissionId, 1);
}

TEST_F(TransmissionScheduleCancelTest, cancelAll)
{
    EXPECT_EQ(scheduler.cancelAll(), 3);

    const std::list<std::shared_ptr<PrioritizedTxScheduler::Transmission>> schedule = scheduler.getSchedule();

    ASSERT_EQ(schedule.size(), 0);
}
