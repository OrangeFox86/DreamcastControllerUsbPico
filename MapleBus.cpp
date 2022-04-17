#include "MapleBus.hpp"

MapleBus::MapleBus(uint32_t pinA, uint32_t pinB) :
    mPinA(pinA),
    mPinB(pinB),
    mPinAMask(1 << pinA),
    mPinBMask(1 << pinB),
    mAllPinMask(mPinAMask | mPinBMask)
{
    // B must be the very next output from A
    assert((pinB - pinA) == 1);
    // Initialize the pins as inputs with pull ups so that idle line will always be high
    gpio_init(mPinA);
    gpio_init(mPinB);
    gpio_set_dir_in_masked(mAllPinMask);
    gpio_set_pulls(mPinA, true, false);
    gpio_set_pulls(mPinB, true, false);
}

bool MapleBus::writeInit() const
{
    // Only one pair may be output, so make all others input
    gpio_set_dir_in_masked(~mAllPinMask);
    // Ensure no one is pulling low
    if ((gpio_get_all() & mAllPinMask) == mAllPinMask)
    {
        // Set the two pins at output and keep HIGH
        gpio_put_all(mAllPinMask);
        gpio_set_dir_out_masked(mAllPinMask);
        gpio_put_all(mAllPinMask);
        return true;
    }
    else
    {
        return false;
    }
}

void MapleBus::writeComplete() const
{
    // Make everything input
    gpio_set_dir_in_masked(0xFFFFFFFF);
    // Pull up setting shouldn't have changed, but just for good measure...
    gpio_set_pulls(mPinA, true, false);
    gpio_set_pulls(mPinB, true, false);
}
