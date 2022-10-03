#include "core.h"

namespace core
{

//=============================================================================

Core::Core():
    m_devStat{&m_pkgCV}
{}

//-----------------------------------------------------------------------------

bool Core::start()
{
    // Запустить поток приема команд от ТПО.
    if (!m_startThread())
        return false;

    // Передать указатель на класс сбора статистики в класс протокола.
    m_proto.setPointerToStatistic(&m_devStat);

    // Ожидать оповещения о готовых пакетах. Блокирующая функция (поток не вернется до завершения программы).
    m_waitForPkg();

    // Функция никогда не достигнет этого места !
    return true;
}

//-----------------------------------------------------------------------------

bool Core::m_startThread()
{
    m_recvThread = std::thread(m_recvCmd, this);
    if (!m_recvThread.joinable())
    {
        LOGGER_ERROR("Can't start receiving command from TPO in thread");
        return false;
    }
    else
    {
        LOGGER_INFO("Receiving commands from TPO started in thread");
        return true;
    }
}

//-----------------------------------------------------------------------------

void Core::m_recvCmd(Core * core)
{    
    while (true)
    {
        // Если Keep-Alive вернулся по таймауту, то приостановить сбор статистики.
        if (!core->m_proto.waitForCommand())
            core->m_devStat.stopStatistic();
    }
}

//-----------------------------------------------------------------------------

void Core::m_waitForPkg()
{
    std::unique_lock<std::mutex> lk(m_pkgCVMutex);
    while (true)
    {
        m_pkgCV.wait(lk, [this]
        {
            while (this->m_devStat.readStatistic(this->m_pkg))
            {
                // Отправить пакет со статистикой.
                this->m_proto.sendStatistic(this->m_pkg);
            }
            return false;
        });
        lk.unlock(); // needs ?
    }
}

//=============================================================================

} // namespace core


