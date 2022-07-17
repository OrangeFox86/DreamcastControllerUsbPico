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
            DreamcastMainNode(bus, playerData),
            mMockedSubNodes()
        {
            // Swap out the real sub nodes with mocked sub nodes
            mSubNodes.clear();
            uint32_t numSubNodes = DreamcastPeripheral::MAX_SUB_PERIPHERALS;
            mMockedSubNodes.reserve(numSubNodes);
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

        //! Called from peripheralFactory below so we can test what function code it was called with
        MOCK_METHOD(void, mockMethodPeripheralFactory, (uint32_t functionCode));

        //! This function overrides the real peripheral factory so that mock peripherals may be
        //! created.
        void peripheralFactory(uint32_t functionCode) override
        {
            mPeripherals = mPeripheralsToAdd;
            mockMethodPeripheralFactory(functionCode);
        }

        //! Allows the test to check the value of mNextCheckTime
        uint64_t getNextCheckTime() {return mNextCheckTime;}

        //! Allows the test to initialize the value of mNextCheckTime
        void setNextCheckTime(uint64_t t) {mNextCheckTime = t;}

        //! Allows the test to check what peripherals the node has
        std::vector<std::shared_ptr<DreamcastPeripheral>>& getPeripherals()
        {
            return mPeripherals;
        }

        //! The mocked nodes set in the constructor
        std::vector<std::shared_ptr<MockedDreamcastSubNode>> mMockedSubNodes;

        //! Allows the test to set what peripherals to add on next call to peripheralFactory()
        std::vector<std::shared_ptr<DreamcastPeripheral>> mPeripheralsToAdd;
};

class MainNodeTest : public ::testing::Test
{
    public:
        //! Sets up the DreamcastMainNode with mocked interfaces
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
    // --- MOCKING ---
    // The task should always first process events on the maple bus
    EXPECT_CALL(mMapleBus, processEvents(1000000)).Times(1);
    // The task will then read data from the bus, and nothing will be returned
    EXPECT_CALL(mMapleBus, getReadData(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgReferee<0>((uint32_t)0), SetArgReferee<1>(false), Return((const uint32_t*)NULL)));
    // Since no peripherals are detected, the main node should do a info request, and it will be successful
    EXPECT_CALL(mMapleBus,
                write((uint8_t)COMMAND_DEVICE_INFO_REQUEST,
                      (uint8_t)0x20,
                      (const uint32_t*)NULL,
                      (uint8_t)0,
                      true,
                      _))
        .Times(1)
        .WillOnce(Return(true));

    // --- TEST EXECUTION ---
    mDreamcastMainNode.task(1000000);

    // --- EXPECTATIONS ---
    // Next check will occur  in 16 ms
    EXPECT_EQ(mDreamcastMainNode.getNextCheckTime(), 1016000);
}

TEST_F(MainNodeTest, unsuccessfulInfoRequest)
{
    // --- MOCKING ---
    // The task should always first process events on the maple bus
    EXPECT_CALL(mMapleBus, processEvents(1000000)).Times(1);
    // The task will then read data from the bus, and nothing will be returned
    EXPECT_CALL(mMapleBus, getReadData(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgReferee<0>((uint32_t)0), SetArgReferee<1>(false), Return((const uint32_t*)NULL)));
    // Since no peripherals are detected, the main node should do a info request, and it will be unsuccessful
    EXPECT_CALL(mMapleBus,
                write((uint8_t)COMMAND_DEVICE_INFO_REQUEST,
                      (uint8_t)0x20,
                      (const uint32_t*)NULL,
                      (uint8_t)0,
                      true,
                      _))
        .Times(1)
        .WillOnce(Return(false));

    // --- TEST EXECUTION ---
    mDreamcastMainNode.task(1000000);

    // --- EXPECTATIONS ---
    // The next time task is called, it is expected to try again
    EXPECT_LE(mDreamcastMainNode.getNextCheckTime(), 1000000);
}

TEST_F(MainNodeTest, peripheralConnect)
{
    // --- SETUP ---
    // The next info request is set sometime in the future
    mDreamcastMainNode.setNextCheckTime(1016000);
    // The mocked factory will add a mocked peripheral
    std::shared_ptr<MockedDreamcastPeripheral> mockedDreamcastPeripheral =
        std::make_shared<MockedDreamcastPeripheral>(0x20, mMapleBus, mPlayerData.playerIndex);
    mDreamcastMainNode.mPeripheralsToAdd.push_back(mockedDreamcastPeripheral);

    // --- MOCKING ---
    // The task should always first process events on the maple bus
    EXPECT_CALL(mMapleBus, processEvents(1000000)).Times(1);
    // The task will then read data from the bus, and peripheral status will be returned
    uint32_t data[2] = {0x05002001, 0x00000001};
    EXPECT_CALL(mMapleBus, getReadData(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgReferee<0>((uint32_t)2), SetArgReferee<1>(true), Return((const uint32_t*)data)));
    // The peripheralFactory method should be called with function code 0x00000001
    EXPECT_CALL(mDreamcastMainNode, mockMethodPeripheralFactory(0x00000001)).Times(1);
    // No sub peripherals detected (addr value is 0x20 - 0 in the last 5 bits)
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[0], setConnected(false)).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[1], setConnected(false)).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[2], setConnected(false)).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[3], setConnected(false)).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[4], setConnected(false)).Times(1);
    // The peripheral's task function will be called with the current time
    EXPECT_CALL(*mockedDreamcastPeripheral, task(1000000)).Times(1).WillOnce(Return(true));
    // All sub node's task functions will be called with the current time
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[0], task(1000000)).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[1], task(1000000)).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[2], task(1000000)).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[3], task(1000000)).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[4], task(1000000)).Times(1);
    // No write operation should be called but the main node
    EXPECT_CALL(mMapleBus, write(_, _, _, _, _, _)).Times(0);

    // --- TEST EXECUTION ---
    mDreamcastMainNode.task(1000000);

    // --- EXPECTATIONS ---
    // Next check time remains unchanged
    EXPECT_EQ(mDreamcastMainNode.getNextCheckTime(), 1016000);
}

TEST_F(MainNodeTest, peripheralDisconnect)
{
    // --- SETUP ---
    // The next info request is set sometime in the future
    mDreamcastMainNode.setNextCheckTime(1016000);
    // A main peripheral is currently connected
    std::shared_ptr<MockedDreamcastPeripheral> mockedDreamcastPeripheral =
        std::make_shared<MockedDreamcastPeripheral>(0x20, mMapleBus, mPlayerData.playerIndex);
    mDreamcastMainNode.getPeripherals().push_back(mockedDreamcastPeripheral);

    // --- MOCKING ---
    // The task should always first process events on the maple bus
    EXPECT_CALL(mMapleBus, processEvents(1000000)).Times(1);
    // The task will then read data from the bus, and nothing will be returned
    EXPECT_CALL(mMapleBus, getReadData(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgReferee<0>((uint32_t)0), SetArgReferee<1>(false), Return((const uint32_t*)NULL)));
    // The peripheral's task() function will be called with the current time, and it will return
    // false, signifying that communication has failed too many times. The peripheral must
    // be disconnected.
    EXPECT_CALL(*mockedDreamcastPeripheral, task(1000000)).Times(1).WillOnce(Return(false));
    // All sub node's task functions will be called with the current time
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[0], mainPeripheralDisconnected()).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[1], mainPeripheralDisconnected()).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[2], mainPeripheralDisconnected()).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[3], mainPeripheralDisconnected()).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[4], mainPeripheralDisconnected()).Times(1);

    // --- TEST EXECUTION ---
    mDreamcastMainNode.task(1000000);

    // --- EXPECTATIONS ---
    // Next check time should be set to current time
    EXPECT_EQ(mDreamcastMainNode.getNextCheckTime(), 1000000);
    // All peripherals removed
    EXPECT_EQ(mDreamcastMainNode.getPeripherals().size(), 0);
}

TEST_F(MainNodeTest, subPeripheralConnect)
{
    // --- SETUP ---
    // The next info request is set sometime in the future
    mDreamcastMainNode.setNextCheckTime(1016000);
    // A main peripheral is currently connected
    std::shared_ptr<MockedDreamcastPeripheral> mockedDreamcastPeripheral =
        std::make_shared<MockedDreamcastPeripheral>(0x01, mMapleBus, mPlayerData.playerIndex);
    mDreamcastMainNode.getPeripherals().push_back(mockedDreamcastPeripheral);

    // --- MOCKING ---
    // The task should always first process events on the maple bus
    EXPECT_CALL(mMapleBus, processEvents(1000000)).Times(1);
    // The task will then read data from the bus, and a sub peripheral's info is returned
    uint32_t data[2] = {0x05000101, 8675309};
    EXPECT_CALL(mMapleBus, getReadData(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgReferee<0>((uint32_t)2), SetArgReferee<1>(true), Return((const uint32_t*)data)));
    // The sub node that data is addressed to (0x01) should handle the info
    EXPECT_CALL(
        *mDreamcastMainNode.mMockedSubNodes[0],
        handleData((uint8_t)1, (uint8_t)5, &data[1])
    ).Times(1).WillOnce(Return(true));
    // The peripheral's task() function will be called with the current time
    EXPECT_CALL(*mockedDreamcastPeripheral, task(1000000)).Times(1).WillOnce(Return(true));
    // All sub node's task functions will be called with the current time
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[0], task(1000000)).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[1], task(1000000)).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[2], task(1000000)).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[3], task(1000000)).Times(1);
    EXPECT_CALL(*mDreamcastMainNode.mMockedSubNodes[4], task(1000000)).Times(1);

    // --- TEST EXECUTION ---
    mDreamcastMainNode.task(1000000);

    // --- EXPECTATIONS ---
    // Next check time remains unchanged
    EXPECT_EQ(mDreamcastMainNode.getNextCheckTime(), 1016000);
}