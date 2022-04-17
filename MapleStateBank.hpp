#ifndef __MAPLE_STATE_BANK_H__
#define __MAPLE_STATE_BANK_H__

#include <stdint.h>
#include "configuration.h"
#include "MapleBus.hpp"

//! Contains encoded Maple Bus states
class MapleStateBank
{
    public:
        //! Constructor
        MapleStateBank() :
            mData(),
            mCurrent(mData),
            mEnd(mData),
            mCapacityEnd(mData + ELEMENTS_PER_BANK),
            mBitShift(0),
            mSendMask(MASK_AB)
        {}

        //! Resets pointers back to 0
        inline void reset()
        {
            mEnd = mData;
            mCurrent = mData;
        }

        //! Writes the next datum to the bank
        inline void write(uint_fast8_t datum)
        {
            // WARNING: no checks done here in order to speed up processing
            *mEnd++ = (datum << mBitShift);
        }

        //! Reads the next byte (FIFO) and increments read pointer
        //! @returns next bus state
        inline uint_fast32_t readNext()
        {
            return *mCurrent++;
        }

        //! @returns the remaining capacity left in this bank
        inline int_fast32_t remainingCapacity()
        {
            return (mCapacityEnd - mEnd);
        }

        //! @returns true iff no states are left in this bank
        inline bool empty()
        {
            return (mEnd == mCurrent);
        }

        //! Sets the bus this bank writes to
        //! @param[in] mapleBus  The bus this state bank must conform to
        inline void setMapleBus(const MapleBus& mapleBus)
        {
            assert((mapleBus.mPinB - mapleBus.mPinA) == 1);
            mBitShift = mapleBus.mPinA;
            mSendMask = MASK_AB << mBitShift;
        }

        //! @returns gpio mask this bank is setup for
        inline uint_fast32_t getSendMask()
        {
            return mSendMask;
        }

    public:
        //! Bit mask for "A" output
        static const uint_fast8_t MASK_A = 0x01;
        //! Bit mask for "B" output
        static const uint_fast8_t MASK_B = 0x02;
        //! Bit mask for both "A" and "B" outputs
        static const uint_fast8_t MASK_AB = (MASK_A | MASK_B);

    private:
        //! Contains this bank's data
        uint_fast32_t mData[ELEMENTS_PER_BANK];
        //! Pointer within mData which represents the next byte to read
        uint_fast32_t* mCurrent;
        //! Pointer within mData which represents the next byte to write
        uint_fast32_t* mEnd;
        //! Pointer to then end (last + 1) of mData
        uint_fast32_t* const mCapacityEnd;
        //! Number of bits to shift left when writing data
        uint_fast32_t mBitShift;
        //! The gpio mask this bank is setup for
        uint_fast32_t mSendMask;
};

#endif // __MAPLE_STATE_BANK_H__
