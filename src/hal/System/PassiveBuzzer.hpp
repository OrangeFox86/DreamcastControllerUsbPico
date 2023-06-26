#pragma once

#include <stdint.h>
#include <list>
#include "pico/time.h"

class PassiveBuzzer
{
public:
    PassiveBuzzer(uint32_t gpio,
                  double systemFreqHz,
                  double baseFreqHz = DEFAULT_BASE_FREQUENCY);
    void setSystemFreq(double systemFreqHz);
    void setBaseFreq(double baseFreqHz);
    inline double getBaseFreq() { return mBaseFreqHz; }
    void stop();
    void buzz(double freq, double dutyCycle = 0.5, double seconds=-1);
    void buzzRaw(uint16_t wrapCount, uint16_t highCount, double seconds=-1);
private:
    int64_t stopAlarmCallback(alarm_id_t id);
    static int64_t stopAlarmCallback(alarm_id_t id, void *passiveBuzzer);
public:
    static const double DEFAULT_BASE_FREQUENCY;
private:
    const uint32_t mGpio;
    const uint32_t mPwmSlice;
    const uint32_t mPwmChan;
    double mSystemFreqHz;
    double mBaseFreqHz;
    double mDivider;
    alarm_id_t mLastAlarm;

};
