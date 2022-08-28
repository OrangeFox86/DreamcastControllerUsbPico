#include "MockMapleBus.hpp"
#include "MockDreamcastControllerObserver.hpp"
#include "MockDreamcastPeripheral.hpp"
#include "MockMutex.hpp"
#include "MockUsbController.hpp"

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

class DreamcastSubNodeOverride : public DreamcastSubNode
{
    public:
        DreamcastSubNodeOverride(uint8_t addr, std::shared_ptr<PrioritizedTxScheduler> scheduler, PlayerData playerData) :
            DreamcastSubNode(addr, scheduler, playerData)
        {
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

        //! Allows the test to check what peripherals the node has
        std::vector<std::shared_ptr<DreamcastPeripheral>>& getPeripherals()
        {
            return mPeripherals;
        }

        //! Allows the test to check connection state
        bool isConnected()
        {
            return mConnected;
        }

        //! Allows the test to set connection state
        void mockSetConnected(bool value)
        {
            mConnected = value;
        }

        //! Allows the test to set what peripherals to add on next call to peripheralFactory()
        std::vector<std::shared_ptr<DreamcastPeripheral>> mPeripheralsToAdd;
};

class SubNodeTest : public ::testing::Test
{
    public:
        //! Sets up the DreamcastMainNode with mocked interfaces
        SubNodeTest() :
            mDreamcastControllerObserver(),
            mMutex(),
            mScreenData(mMutex),
            mPlayerData{1, mDreamcastControllerObserver, mScreenData},
            mMapleBus(),
            mDreamcastSubNode(0x01, mMapleBus, mPlayerData)
        {}

    protected:
        MockDreamcastControllerObserver mDreamcastControllerObserver;
        MockMutex mMutex;
        ScreenData mScreenData;
        PlayerData mPlayerData;
        MockMapleBus mMapleBus;
        DreamcastSubNodeOverride mDreamcastSubNode;
        std::shared_ptr<MockDreamcastPeripheral> mockDreamcastPeripheral1;
        std::shared_ptr<MockDreamcastPeripheral> mockDreamcastPeripheral2;

        virtual void SetUp()
        {}

        virtual void TearDown()
        {}

        void addTwoMockedPeripherals()
        {
            std::vector<std::shared_ptr<DreamcastPeripheral>>& peripherals =
                mDreamcastSubNode.getPeripherals();
            mockDreamcastPeripheral1 =
                std::make_shared<MockDreamcastPeripheral>(0x01, mMapleBus, mPlayerData.playerIndex);
            peripherals.push_back(mockDreamcastPeripheral1);
            mockDreamcastPeripheral2 =
                std::make_shared<MockDreamcastPeripheral>(0x01, mMapleBus, mPlayerData.playerIndex);
            peripherals.push_back(mockDreamcastPeripheral2);
        }
};

TEST_F(SubNodeTest, handleDataCommandNoPeripheralsAdded)
{
    EXPECT_CALL(mDreamcastSubNode, mockMethodPeripheralFactory(1234567)).Times(1);
    uint32_t payload[1] = {1234567};
    EXPECT_FALSE(mDreamcastSubNode.handleData(1, 5, payload));
    EXPECT_TRUE(mDreamcastSubNode.getPeripherals().empty());
}

TEST_F(SubNodeTest, handleDataCommandPeripheralAdded)
{
    // The mocked factory will add a mocked peripheral
    std::shared_ptr<MockDreamcastPeripheral> mockedDreamcastPeripheral =
        std::make_shared<MockDreamcastPeripheral>(0x01, mMapleBus, mPlayerData.playerIndex);
    mDreamcastSubNode.mPeripheralsToAdd.push_back(mockedDreamcastPeripheral);

    EXPECT_CALL(mDreamcastSubNode, mockMethodPeripheralFactory(1234567)).Times(1);
    uint32_t payload[1] = {1234567};
    EXPECT_TRUE(mDreamcastSubNode.handleData(1, 5, payload));
    EXPECT_EQ(mDreamcastSubNode.getPeripherals().size(), 1);
}

TEST_F(SubNodeTest, handleDataCommandInvalidPayloadSize)
{
    // The mocked factory will add a mocked peripheral
    std::shared_ptr<MockDreamcastPeripheral> mockedDreamcastPeripheral =
        std::make_shared<MockDreamcastPeripheral>(0x01, mMapleBus, mPlayerData.playerIndex);
    mDreamcastSubNode.mPeripheralsToAdd.push_back(mockedDreamcastPeripheral);

    EXPECT_CALL(mDreamcastSubNode, mockMethodPeripheralFactory(_)).Times(0);
    EXPECT_FALSE(mDreamcastSubNode.handleData(0, 5, NULL));
    EXPECT_TRUE(mDreamcastSubNode.getPeripherals().empty());
}

TEST_F(SubNodeTest, handleDataPeripheralDataNoneHandled)
{
    // --- MOCKING ---
    // Add 2 mock peripherals to the sub node
    addTwoMockedPeripherals();
    // Setup the payload
    uint32_t payload[3] = {1234567, 987654, 8088};
    // Neither of the peripherals will handle data
    EXPECT_CALL(*mockDreamcastPeripheral1, handleData(3, 0xAB, payload)).Times(1).WillOnce(Return(false));
    EXPECT_CALL(*mockDreamcastPeripheral2, handleData(3, 0xAB, payload)).Times(1).WillOnce(Return(false));

    // --- TEST EXECUTION ---
    EXPECT_FALSE(mDreamcastSubNode.handleData(3, 0xAB, payload));
}

TEST_F(SubNodeTest, handleDataPeripheralDataFirstHandled)
{
    // --- MOCKING ---
    // Add 2 mock peripherals to the sub node
    addTwoMockedPeripherals();
    // Setup the payload
    uint32_t payload[3] = {1234567, 987654, 8088};
    // The first peripheral will handle data
    EXPECT_CALL(*mockDreamcastPeripheral1, handleData(3, 0xAB, payload)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mockDreamcastPeripheral2, handleData(_, _, _)).Times(0);

    // --- TEST EXECUTION ---
    EXPECT_TRUE(mDreamcastSubNode.handleData(3, 0xAB, payload));
}

TEST_F(SubNodeTest, taskNoPeripheralsNotConnected)
{
    // --- MOCKING ---
    EXPECT_CALL(mMapleBus, write(_, _, _)).Times(0);

    // --- TEST EXECUTION ---
    mDreamcastSubNode.task(999999999);
}

TEST_F(SubNodeTest, taskNoPeripheralsConnectedNotTime)
{
    // --- MOCKING ---
    EXPECT_CALL(mMapleBus, write(_, _, _)).Times(0);
    mDreamcastSubNode.mockSetConnected(true);
    mDreamcastSubNode.setNextCheckTime(1000000000);

    // --- TEST EXECUTION ---
    mDreamcastSubNode.task(999999999);
}

TEST_F(SubNodeTest, taskNoPeripheralsConnectedAndTimeSuccessful)
{
    // --- MOCKING ---
    EXPECT_CALL(mMapleBus, write(MaplePacket((uint8_t)COMMAND_DEVICE_INFO_REQUEST,
                                             (uint8_t)0x41,
                                             (const uint32_t*)NULL,
                                             (uint8_t)0),
                                 true,
                                 _))
        .Times(1)
        .WillOnce(Return(true));
    mDreamcastSubNode.mockSetConnected(true);

    // --- TEST EXECUTION ---
    mDreamcastSubNode.task(999999999);

    // --- EXPECTATIONS ---
    // Next check will occur in 16 ms
    EXPECT_EQ(mDreamcastSubNode.getNextCheckTime(), 1000015999);
}

TEST_F(SubNodeTest, taskNoPeripheralsConnectedAndTimeUnsuccessful)
{
    // --- MOCKING ---
    EXPECT_CALL(mMapleBus, write(MaplePacket((uint8_t)COMMAND_DEVICE_INFO_REQUEST,
                                             (uint8_t)0x41,
                                             (const uint32_t*)NULL,
                                             (uint8_t)0),
                                 true,
                                 _))
        .Times(1)
        .WillOnce(Return(false));
    mDreamcastSubNode.mockSetConnected(true);

    // --- TEST EXECUTION ---
    mDreamcastSubNode.task(999999999);

    // --- EXPECTATIONS ---
    // Next check time will remain the default
    EXPECT_EQ(mDreamcastSubNode.getNextCheckTime(), 0);
}

TEST_F(SubNodeTest, taskPeripheralsConnectedSuccessfulTask)
{
    // --- MOCKING ---
    mDreamcastSubNode.mockSetConnected(true);
    mDreamcastSubNode.setNextCheckTime(1000000000);
    // Add 2 mock peripherals to the sub node
    addTwoMockedPeripherals();
    // Both peripherals will successfully handle their task
    EXPECT_CALL(*mockDreamcastPeripheral1, task(554433)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mockDreamcastPeripheral2, task(554433)).Times(1).WillOnce(Return(true));

    // --- TEST EXECUTION ---
    mDreamcastSubNode.task(554433);

    // --- EXPECTATIONS ---
    EXPECT_EQ(mDreamcastSubNode.getNextCheckTime(), 1000000000);
    EXPECT_EQ(mDreamcastSubNode.getPeripherals().size(), 2);
}

TEST_F(SubNodeTest, taskPeripheralsConnectedFailureTask)
{
    // --- MOCKING ---
    mDreamcastSubNode.mockSetConnected(true);
    // Add 2 mock peripherals to the sub node
    addTwoMockedPeripherals();
    // Both peripherals will successfully handle their task
    EXPECT_CALL(*mockDreamcastPeripheral1, task(554433)).Times(1).WillOnce(Return(false));
    EXPECT_CALL(*mockDreamcastPeripheral2, task(_)).Times(0);

    // --- TEST EXECUTION ---
    mDreamcastSubNode.task(554433);

    // --- EXPECTATIONS ---
    EXPECT_EQ(mDreamcastSubNode.getNextCheckTime(), 554433);
    EXPECT_TRUE(mDreamcastSubNode.getPeripherals().empty());
}

TEST_F(SubNodeTest, mainPeripheralDisconnected)
{
    // --- MOCKING ---
    // Add 2 mock peripherals to the sub node
    addTwoMockedPeripherals();

    // --- TEST EXECUTION ---
    mDreamcastSubNode.mainPeripheralDisconnected();

    // --- EXPECTATIONS ---
    EXPECT_TRUE(mDreamcastSubNode.getPeripherals().empty());
}

TEST_F(SubNodeTest, setConnectedDisconnectedToConnected)
{
    // --- MOCKING ---
    // Add 2 mock peripherals to the sub node
    addTwoMockedPeripherals();

    // --- TEST EXECUTION ---
    mDreamcastSubNode.setConnected(true);

    // --- EXPECTATIONS ---
    EXPECT_TRUE(mDreamcastSubNode.isConnected());
    EXPECT_EQ(mDreamcastSubNode.getPeripherals().size(), 2);
}

TEST_F(SubNodeTest, setConnectedConnectedToDisconnected)
{
    // --- MOCKING ---
    mDreamcastSubNode.mockSetConnected(true);
    // Add 2 mock peripherals to the sub node
    addTwoMockedPeripherals();

    // --- TEST EXECUTION ---
    mDreamcastSubNode.setConnected(false);

    // --- EXPECTATIONS ---
    EXPECT_FALSE(mDreamcastSubNode.isConnected());
    EXPECT_TRUE(mDreamcastSubNode.getPeripherals().empty());
}

TEST_F(SubNodeTest, setConnectedDisconnectedToDisconnected)
{
    // --- MOCKING ---
    mDreamcastSubNode.mockSetConnected(false);
    // Add 2 mock peripherals to the sub node
    addTwoMockedPeripherals();

    // --- TEST EXECUTION ---
    mDreamcastSubNode.setConnected(false);

    // --- EXPECTATIONS ---
    EXPECT_FALSE(mDreamcastSubNode.isConnected());
    EXPECT_EQ(mDreamcastSubNode.getPeripherals().size(), 2);
}
