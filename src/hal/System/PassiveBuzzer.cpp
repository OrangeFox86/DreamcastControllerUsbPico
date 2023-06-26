#include "PassiveBuzzer.hpp"

#include "hardware/gpio.h"
#include "hardware/pwm.h"

#include <cmath>

const double PassiveBuzzer::DEFAULT_BASE_FREQUENCY = 546134.73143824581524;

PassiveBuzzer::PassiveBuzzer(uint32_t gpio, double systemFreqHz, double baseFreqHz) :
    mGpio(gpio),
    mPwmSlice(pwm_gpio_to_slice_num(gpio)),
    mPwmChan(pwm_gpio_to_channel(gpio)),
    mSystemFreqHz(systemFreqHz),
    mBaseFreqHz(baseFreqHz),
    mDivider(),
    mLastAlarm(0)
{
    gpio_init(mGpio);
    gpio_put(mGpio, false);
    gpio_set_dir(mGpio, true);
    gpio_set_function(mGpio, GPIO_FUNC_PWM);


    setBaseFreq(baseFreqHz);
}

void PassiveBuzzer::setSystemFreq(double systemFreqHz)
{
    mSystemFreqHz = systemFreqHz;
    setBaseFreq(mBaseFreqHz);
}

void PassiveBuzzer::setBaseFreq(double baseFreqHz)
{
    mBaseFreqHz = baseFreqHz;
    mDivider = mSystemFreqHz / mBaseFreqHz;

    stop();
    pwm_set_clkdiv(mPwmSlice, mDivider);

}

void PassiveBuzzer::stop()
{
    pwm_set_enabled(mPwmSlice, false);
}

void PassiveBuzzer::buzz(double freq, double dutyCycle, double seconds)
{
    uint16_t wrap = std::ceil(mBaseFreqHz / freq) - 1;
    buzzRaw(wrap, wrap * dutyCycle, seconds);
}

void PassiveBuzzer::buzzRaw(uint16_t wrapCount, uint16_t highCount, double seconds)
{
    if (mLastAlarm > 0)
    {
        // Still waiting on previous stop - stop it now
        cancel_alarm(mLastAlarm);
        stop();
    }

    if (seconds != 0)
    {
        pwm_set_wrap(mPwmSlice, wrapCount);
        pwm_set_chan_level(mPwmSlice, mPwmChan, highCount);
        pwm_set_enabled(mPwmSlice, true);

        if (seconds > 0)
        {
            // schedule off
            alarm_id_t id = add_alarm_in_us(seconds * 1000000, stopAlarmCallback, this, false);
            if (id <= 0)
            {
                // Failure
                stop();
            }
        }
        // else: buzz until manually stopped
    }
    // else: seconds was set to 0, so don't do anything
}

int64_t PassiveBuzzer::stopAlarmCallback(alarm_id_t id)
{
    mLastAlarm = 0;
    stop();
    return 0; // Don't repeat
}

int64_t PassiveBuzzer::stopAlarmCallback(alarm_id_t id, void *passiveBuzzer)
{
    return static_cast<PassiveBuzzer*>(passiveBuzzer)->stopAlarmCallback(id);
}
