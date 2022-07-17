#include "MockedMapleBus.hpp"
#include "MockedDreamcastControllerObserver.hpp"
#include "MockedDreamcastPeripheral.hpp"
#include "MockedMutex.hpp"
#include "MockedUsbController.hpp"

#include "DreamcastMainNode.hpp"
#include "DreamcastSubNode.hpp"
#include "DreamcastPeripheral.hpp"
#include "dreamcast_constants.h"

#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::_;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::DoAll;

class MockedDreamcastSubNode : public DreamcastSubNode
{
    public:
        MockedDreamcastSubNode(uint8_t addr, MapleBusInterface& bus, PlayerData playerData) :
            DreamcastSubNode(addr, bus, playerData)
        {}

        MOCK_METHOD(bool,
                    handleData,
                    (uint8_t len, uint8_t cmd, const uint32_t *payload),
                    (override));

        MOCK_METHOD(void, task, (uint64_t currentTimeUs), (override));

        MOCK_METHOD(void, mainPeripheralDisconnected, (), (override));

        MOCK_METHOD(void, setConnected, (bool connected), (override));
};

class DreamcastMainNodeOverride : public DreamcastMainNode
{
    public:
        DreamcastMainNodeOverride(MapleBusInterface& bus, PlayerData playerData) :
            DreamcastMainNode(bus, playerData, 0),
            mMockedSubNodes()
        {
            uint32_t numSubNodes = DreamcastPeripheral::MAX_SUB_PERIPHERALS;
            mSubNodes.reserve(numSubNodes);
            for (uint32_t i = 0; i < numSubNodes; ++i)
            {
                std::shared_ptr<MockedDreamcastSubNode> mockedSubNode =
                    std::make_shared<MockedDreamcastSubNode>(
                        DreamcastPeripheral::subPeripheralMask(i), mBus, mPlayerData);
                mMockedSubNodes.push_back(mockedSubNode);
                mSubNodes.push_back(mockedSubNode);
            }
        }

        MOCK_METHOD(void, mockMethodPeripheralFactory, (uint32_t functionCode));

        void peripheralFactory(uint32_t functionCode) override
        {
            mPeripherals = mPeripheralsToAdd;
            mockMethodPeripheralFactory(functionCode);
        }

        uint64_t getNextCheckTime() {return mNextCheckTime;}
        void setNextCheckTime(uint64_t t) {mNextCheckTime = t;}
        std::vector<std::shared_ptr<DreamcastPeripheral>>& getPeripherals()
        {
            return mPeripherals;
        }

        std::vector<std::shared_ptr<MockedDreamcastSubNode>> mMockedSubNodes;

        std::vector<std::shared_ptr<DreamcastPeripheral>> mPeripheralsToAdd;
};

class MainNodeTest : public ::testing::Test
{
    public:
        MainNodeTest() :
            mDreamcastControllerObserver(),
            mMutex(),
            mScreenData(mMutex),
            mPlayerData{0, mDreamcastControllerObserver, mScreenData},
            mMapleBus(),
            mDreamcastMainNode(mMapleBus, mPlayerData)
        {}

    protected:
        MockedDreamcastControllerObserver mDreamcastControllerObserver;
        MockedMutex mMutex;
        ScreenData mScreenData;
        PlayerData mPlayerData;
        MockedMapleBus mMapleBus;
        DreamcastMainNodeOverride mDreamcastMainNode;

        virtual void SetUp()
        {}

        virtual void TearDown()
        {}
};

TEST_F(MainNodeTest, successfulInfoRequest)
{
    EXPECT_CALL(mMapleBus, processEvents(1000000)).Times(1);
    EXPECT_CALL(mMapleBus, getReadData(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgReferee<0>((uint32_t)0), SetArgReferee<1>(false), Return((const uint32_t*)NULL)));
    EXPECT_CALL(mMapleBus,
                write((uint8_t)COMMAND_DEVICE_INFO_REQUEST,
                      (uint8_t)0x20,
                      (const uint32_t*)NULL,
                      (uint8_t)0,
                      true,
                      _))
        .Times(1)
        .WillOnce(Return(true));

    mDreamcastMainNode.task(1000000);

    // Next check will occur  in 16 ms
    EXPECT_EQ(mDreamcastMainNode.getNextCheckTime(), 1016000);
}

TEST_F(MainNodeTest, unsuccessfulInfoRequest)
{
    EXPECT_CALL(mMapleBus, processEvents(1000000)).Times(1);
    EXPECT_CALL(mMapleBus, getReadData(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgReferee<0>((uint32_t)0), SetArgReferee<1>(false), Return((const uint32_t*)NULL)));
    EXPECT_CALL(mMapleBus,
                write((uint8_t)COMMAND_DEVICE_INFO_REQUEST,
                      (uint8_t)0x20,
                      (const uint32_t*)NULL,
                      (uint8_t)0,
                      true,
                      _))
        .Times(1)
        .WillOnce(Return(false));

    mDreamcastMainNode.task(1000000);

    // The next time task is called, it is expected to try again
    EXPECT_LE(mDreamcastMainNode.getNextCheckTime(), 1000000);
}

TEST_F(MainNodeTest, peripheralConnect)
{
    mDreamcastMainNode.setNextCheckTime(1016000);
    std::shared_ptr<MockedDreamcastPeripheral> mockedDreamcastPeripheral =
        std::make_shared<MockedDreamcastPeripheral>(0x20, mMapleBus, mPlayerData.playerIndex);
    mDreamcastMainNode.mPeripheralsToAdd.push_back(mockedDreamcastPeripheral);
    EXPECT_CALL(mMapleBus, processEvents(1000000)).Times(1);
    uint32_t data[2] = {0x05002001, 0x00000001};
    EXPECT_CALL(mMapleBus, getReadData(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgReferee<0>((uint32_t)2), SetArgReferee<1>(true), Return((const uint32_t*)data)));
    EXPECT_CALL(mDreamcastMainNode, mockMethodPeripheralFactory(0x00000001)).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[0], setConnected(false)).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[1], setConnected(false)).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[2], setConnected(false)).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[3], setConnected(false)).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[4], setConnected(false)).Times(1);
    EXPECT_CALL(*mockedDreamcastPeripheral, task(1000000)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[0], task(1000000)).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[1], task(1000000)).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[2], task(1000000)).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[3], task(1000000)).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[4], task(1000000)).Times(1);
    EXPECT_CALL(mMapleBus, write(_, _, _, _, _, _)).Times(0);

    mDreamcastMainNode.task(1000000);

    // Next check time remains unchanged
    EXPECT_EQ(mDreamcastMainNode.getNextCheckTime(), 1016000);
}

TEST_F(MainNodeTest, peripheralDisconnect)
{
    mDreamcastMainNode.setNextCheckTime(1016000);
    std::shared_ptr<MockedDreamcastPeripheral> mockedDreamcastPeripheral =
        std::make_shared<MockedDreamcastPeripheral>(0x20, mMapleBus, mPlayerData.playerIndex);
    mDreamcastMainNode.getPeripherals().push_back(mockedDreamcastPeripheral);
    EXPECT_CALL(mMapleBus, processEvents(1000000)).Times(1);
    EXPECT_CALL(mMapleBus, getReadData(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgReferee<0>((uint32_t)0), SetArgReferee<1>(false), Return((const uint32_t*)NULL)));
    EXPECT_CALL(*mockedDreamcastPeripheral, task(1000000)).Times(1).WillOnce(Return(false));
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[0], mainPeripheralDisconnected()).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[1], mainPeripheralDisconnected()).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[2], mainPeripheralDisconnected()).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[3], mainPeripheralDisconnected()).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[4], mainPeripheralDisconnected()).Times(1);

    mDreamcastMainNode.task(1000000);

    // Next check time should be set to current time
    EXPECT_EQ(mDreamcastMainNode.getNextCheckTime(), 1000000);
    // All peripherals removed
    EXPECT_EQ(mDreamcastMainNode.getPeripherals().size(), 0);
}

TEST_F(MainNodeTest, subPeripheralConnect)
{
    mDreamcastMainNode.setNextCheckTime(1016000);
    std::shared_ptr<MockedDreamcastPeripheral> mockedDreamcastPeripheral =
        std::make_shared<MockedDreamcastPeripheral>(0x01, mMapleBus, mPlayerData.playerIndex);
    mDreamcastMainNode.getPeripherals().push_back(mockedDreamcastPeripheral);
    EXPECT_CALL(mMapleBus, processEvents(1000000)).Times(1);
    uint32_t data[2] = {0x05000101, 8675309};
    EXPECT_CALL(mMapleBus, getReadData(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgReferee<0>((uint32_t)2), SetArgReferee<1>(true), Return((const uint32_t*)data)));
    EXPECT_CALL(
        *mDreamcastMainNode.mMockedSubNodes[0],
        handleData((uint8_t)1, (uint8_t)5, &data[1])
    ).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mockedDreamcastPeripheral, task(1000000)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[0], task(1000000)).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[1], task(1000000)).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[2], task(1000000)).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[3], task(1000000)).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[4], task(1000000)).Times(1);

    mDreamcastMainNode.task(1000000);

    // Next check time remains unchanged
    EXPECT_EQ(mDreamcastMainNode.getNextCheckTime(), 1016000);
}