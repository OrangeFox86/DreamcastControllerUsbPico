#include "MockMapleBus.hpp"
#include "MockDreamcastControllerObserver.hpp"
#include "MockDreamcastPeripheral.hpp"
#include "MockMutex.hpp"
#include "MockUsbController.hpp"

#include "DreamcastMainNode.hpp"
#include "DreamcastSubNode.hpp"
#include "DreamcastPeripheral.hpp"
#include "dreamcast_constants.h"
#include "EndpointTxSchedulerInterface.hpp"
#include "EndpointTxScheduler.hpp"

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
        DreamcastSubNodeOverride(uint8_t addr,
                                 std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                                 PlayerData playerData) :
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
            mPrioritizedTxScheduler(std::make_shared<PrioritizedTxScheduler>()),
            mEndpointTxScheduler(std::make_shared<EndpointTxScheduler>(
                mPrioritizedTxScheduler, 0, DreamcastPeripheral::getRecipientAddress(1, 0x01))),
            mDreamcastSubNode(0x01, mEndpointTxScheduler, mPlayerData)
        {}

    protected:
        MockDreamcastControllerObserver mDreamcastControllerObserver;
        MockMutex mMutex;
        ScreenData mScreenData;
        PlayerData mPlayerData;
        std::shared_ptr<PrioritizedTxScheduler> mPrioritizedTxScheduler;
        std::shared_ptr<EndpointTxScheduler> mEndpointTxScheduler;
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
                std::make_shared<MockDreamcastPeripheral>(0x01, mEndpointTxScheduler, mPlayerData.playerIndex);
            peripherals.push_back(mockDreamcastPeripheral1);
            mockDreamcastPeripheral2 =
                std::make_shared<MockDreamcastPeripheral>(0x01, mEndpointTxScheduler, mPlayerData.playerIndex);
            peripherals.push_back(mockDreamcastPeripheral2);
        }
};

TEST_F(SubNodeTest, handleDataCommandNoPeripheralsAdded)
{
    // --- MOCKING ---
    EXPECT_CALL(mDreamcastSubNode, mockMethodPeripheralFactory(1234567)).Times(1);
    std::shared_ptr<MaplePacket> packet = std::make_shared<MaplePacket>(5, 0, 1234567);
    std::shared_ptr<MaplePacket> txPacket = std::make_shared<MaplePacket>(4, 1, 7654321);
    std::shared_ptr<const Transmission> tx =
        std::make_shared<Transmission>(0, 0, true, 0, 123, 0, txPacket, nullptr);

    // --- TEST EXECUTION ---
    mDreamcastSubNode.txComplete(packet, tx);

    // --- EXPECTATIONS ---
    EXPECT_TRUE(mDreamcastSubNode.getPeripherals().empty());
}

TEST_F(SubNodeTest, handleDataCommandPeripheralAdded)
{
    // --- MOCKING ---
    // The mocked factory will add a mocked peripheral
    std::shared_ptr<MockDreamcastPeripheral> mockedDreamcastPeripheral =
        std::make_shared<MockDreamcastPeripheral>(0x01, mEndpointTxScheduler, mPlayerData.playerIndex);
    mDreamcastSubNode.mPeripheralsToAdd.push_back(mockedDreamcastPeripheral);
    EXPECT_CALL(mDreamcastSubNode, mockMethodPeripheralFactory(1234567)).Times(1);
    std::shared_ptr<MaplePacket> packet = std::make_shared<MaplePacket>(5, 0, 1234567);
    std::shared_ptr<MaplePacket> txPacket = std::make_shared<MaplePacket>(4, 1, 7654321);
    std::shared_ptr<const Transmission> tx =
        std::make_shared<Transmission>(0, 0, true, 0, 123, 0, txPacket, nullptr);

    // --- TEST EXECUTION ---
    mDreamcastSubNode.txComplete(packet, tx);

    // --- EXPECTATIONS ---
    EXPECT_EQ(mDreamcastSubNode.getPeripherals().size(), 1);
}

TEST_F(SubNodeTest, handleDataCommandInvalidPayloadSize)
{
    // --- MOCKING ---
    // The mocked factory will add a mocked peripheral
    std::shared_ptr<MockDreamcastPeripheral> mockedDreamcastPeripheral =
        std::make_shared<MockDreamcastPeripheral>(0x01, mEndpointTxScheduler, mPlayerData.playerIndex);
    mDreamcastSubNode.mPeripheralsToAdd.push_back(mockedDreamcastPeripheral);
    std::shared_ptr<MaplePacket> packet = std::make_shared<MaplePacket>(5, 0, (uint32_t*)NULL, 0);
    std::shared_ptr<MaplePacket> txPacket = std::make_shared<MaplePacket>(4, 1, 7654321);
    std::shared_ptr<const Transmission> tx =
        std::make_shared<Transmission>(0, 0, true, 0, 123, 0, txPacket, nullptr);
    EXPECT_CALL(mDreamcastSubNode, mockMethodPeripheralFactory(_)).Times(0);

    // --- TEST EXECUTION ---
    mDreamcastSubNode.txComplete(packet, tx);

    // --- EXPECTATIONS ---
    EXPECT_TRUE(mDreamcastSubNode.getPeripherals().empty());
}

TEST_F(SubNodeTest, taskPeripheralsConnectedSuccessfulTask)
{
    // --- MOCKING ---
    mDreamcastSubNode.mockSetConnected(true);
    // Add 2 mock peripherals to the sub node
    addTwoMockedPeripherals();
    // Both peripherals will successfully handle their task
    EXPECT_CALL(*mockDreamcastPeripheral1, task(554433)).Times(1);
    EXPECT_CALL(*mockDreamcastPeripheral2, task(554433)).Times(1);

    // --- TEST EXECUTION ---
    mDreamcastSubNode.task(554433);

    // --- EXPECTATIONS ---
    EXPECT_EQ(mDreamcastSubNode.getPeripherals().size(), 2);
}

TEST_F(SubNodeTest, mainPeripheralDisconnected)
{
    // --- MOCKING ---
    mDreamcastSubNode.mockSetConnected(true);
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
    EXPECT_EQ(mDreamcastSubNode.getPeripherals().size(), 0);
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
