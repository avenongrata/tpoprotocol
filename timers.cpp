#include <iostream>
#include <thread>
#include <math.h>

#include "timers.h"


namespace timers
{

//=============================================================================

Tick::Tick()
    : m_curTick(1)
{}

//-----------------------------------------------------------------------------

Tick & Tick::operator++()
{
    m_curTick++;
    if (m_curTick > m_maxTickCnt)
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

ticks_t Tick::hzToTicks(hz_t & hz)
{
    if (hz < 0)
        return 0;

    if (hz <= m_maxTicks)
        return m_maxTicks / hz;
    else
        return 0;
}

//-----------------------------------------------------------------------------

hz_t Tick::ticksToHz(ticks_t & ticks)
{
    auto hz = hz_t(m_maxTicks) / ticks;
    // Максимальное количество знаков после запятой - 1.
    double f = pow(10, 1);
    return int(hz*f) / f;
}

//-----------------------------------------------------------------------------

void Tick::sleep()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(m_sleepTime));
}

//-----------------------------------------------------------------------------

hz_t strToHz(std::string & data)
{
    return stod(data);
}

//=============================================================================

} // namespace timers
