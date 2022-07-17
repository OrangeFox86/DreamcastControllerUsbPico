#include "MockedMapleBus.hpp"
#include "MockedDreamcastControllerObserver.hpp"
#include "MockedMutex.hpp"
#include "MockedUsbController.hpp"

#include "DreamcastMainNode.hpp"
#include "DreamcastPeripheral.hpp"
#include "dreamcast_constants.h"

#include <gtest/gtest.h>

using ::testing::_;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::DoAll;

class MainPeripheralTest : public ::testing::Test
{
    public:
        MainPeripheralTest() :
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
        DreamcastMainNode mDreamcastMainNode;

        virtual void SetUp()
        {}

        virtual void TearDown()
        {}
};

TEST_F(MainPeripheralTest, infoRequest)
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
                      DEFAULT_MAPLE_READ_TIMEOUT_US))
        .Times(1)
        .WillOnce(Return(true));

    mDreamcastMainNode.task(1000000);
}