#ifndef TIMERS_H
#define TIMERS_H

#include "logger-library/logger.h"

//-----------------------------------------------------------------------------

namespace timers
{

//=============================================================================

// Для обозначения частоты обновления.
typedef long hz_t;
// Для обозначения времени сна основного цикла.
typedef long sleepTime_t;
// Для обозначения тиков цикла.
typedef long ticks_t;

//-----------------------------------------------------------------------------

// Максимальная частота обновления устройств - 100 раз в секунду.
const ticks_t g_maxTicks = 100;
// Сколько миллисекунд спит цикл.
const sleepTime_t g_sleepTime = 1000 / g_maxTicks;

//-----------------------------------------------------------------------------

// Класс для работы со временем сна цикла.
class Tick
{
public:

    Tick(sleepTime_t time);
    ~Tick() {}

    Tick & operator++();

    //-------------------------------------------------------------------------

    // Получить текущий номер тика.
    ticks_t getCurTick();
    // Проверить наступило ли время считывания конкретного пула.
    bool isNow(ticks_t & ticks);

    //-------------------------------------------------------------------------

    // Заснуть на определенный срок.
    void sleep();

    //-------------------------------------------------------------------------

private:

    // Класс логирования.
    logger::Logger log;

    // Для обозначения сколько времени должен спать цикл.
    sleepTime_t m_sleepTime;
    // Для обозначения текущего тика.
    ticks_t m_curTick;
};

//-----------------------------------------------------------------------------

// Перевести Гц в количество тиков.
ticks_t hzToTicks(hz_t & hz);
// Перевести тики в Гц.
hz_t ticksToHz(ticks_t & ticks);

//=============================================================================

} // namespace timers

#endif // TIMERS_H