#include <iostream>
#include <thread>

#include "timers.h"


namespace timers
{

//=============================================================================

Tick::Tick(sleepTime_t time)
    : m_sleepTime(time), m_curTick(1)
{}

//-----------------------------------------------------------------------------

Tick & Tick::operator++()
{
    m_curTick++;
    if (m_curTick > g_maxTicks)
        m_curTick = 1;

    return *this;
}

//-----------------------------------------------------------------------------

ticks_t Tick::getCurTick()
{
    return m_curTick;
}

//-----------------------------------------------------------------------------

bool Tick::isNow(ticks_t & jiffy)
{
    // Если по модулю равно 0, значит время считывания наступило.
    return !(m_curTick % jiffy);
}

//-----------------------------------------------------------------------------

ticks_t hzToTicks(hz_t & hz)
{
    if (hz < 0)
        return 0;

    if (hz <= g_maxTicks)
        return g_maxTicks / hz;
    else
        return 0;
}

//-----------------------------------------------------------------------------

void Tick::sleep()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(m_sleepTime));
}

//-----------------------------------------------------------------------------

hz_t ticksToHz(ticks_t & ticks)
{
    return g_maxTicks / ticks;
}

//=============================================================================

} // namespace timers
