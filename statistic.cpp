#include <iostream>
#include <algorithm>
#include <iomanip>

#include "statistic.h"
#include "common.h"

//-----------------------------------------------------------------------------

namespace statistic
{

//=============================================================================

Statistic::Statistic(std::condition_variable * notify)
    : m_pkg(notify), m_activated(false)
{}

//-----------------------------------------------------------------------------

Statistic::~Statistic()
{
    m_activated = false;
    m_statThread.join();

    // Очистить память класса устройств.
    std::lock_guard<std::mutex> lock(m_dataMutex);
    for (auto it = m_devs.begin(); it != m_devs.end(); it++)
        m_deleteDevsMem(it->second);

    // Очистить память класса файлов API.
    for (auto it = m_apis.begin(); it != m_apis.end(); it++)
        m_deleteFilesMem(it->second);

    m_devs.resize(0);
}

//-----------------------------------------------------------------------------

bool Statistic::addDev(dev::devInfo_t & dev, timers::hz_t hz)
{
    // Преобразовать Гц в тики.
    auto ticks = m_timer.hzToTicks(hz);
    // Нельзя добавить устройство с таким количеством тиков.
    if (!ticks)
        return false;

    std::lock_guard<std::mutex> lock(m_dataMutex);
    // Проверить можно ли устройство добавить в пул.
    if (m_isDevExist(dev.first))
    {
        std::stringstream msg;
        msg << "Device " << m_getHexAddr(dev.first) << " is already exist";
        LOGGER_DEBUG(msg.str());
        return false;
    }

    // Добавить устройство в пул.
    m_addDev(dev, ticks);

    // Запустить поток сбора статистики, если еще не запущен.
    return m_startStat();
}

//-----------------------------------------------------------------------------

bool Statistic::addFile(dev::file_t & file, timers::hz_t hz)
{
    // Преобразовать Гц в тики.
    auto ticks = m_timer.hzToTicks(hz);
    // Нельзя добавить файл с таким количеством тиков.
    if (!ticks)
        return false;

    std::lock_guard<std::mutex> lock(m_dataMutex);
    // Проверить можно ли добавить файл API в пул.
    if (m_isFileExist(file))
    {
        std::stringstream msg;
        msg << "File " << file.second << " is already exist";
        LOGGER_DEBUG(msg.str());
        return false;
    }

    // Добавить файл API в пул.
    m_addFile(file, ticks);

    // Запустить поток сбора статистики, если еще не запущен.
    return  m_startStat();
}

//-----------------------------------------------------------------------------

bool Statistic::delDev(uint32_t & addr)
{
    // Заблокировать мьютекс для работы с устройствами. (Разблокируется в m_stopStat).
    m_dataMutex.lock();

    // Вернуть false, если нет такого устройства в списке активных.
    if (!m_isDevExist(addr))
    {
        m_dataMutex.unlock();
        return false;
    }

    // Удалить устройство из списка активных задач и списка активных устройств.
    m_delDevFromPool(addr);
    return true;
}

//-----------------------------------------------------------------------------

bool Statistic::delFile(dev::file_t & file)
{
    // Заблокировать мьютекс для работы с файлами API. (Разблокируется в m_stopStat).
    m_dataMutex.lock();

    // Вернуть false, если нет такого файла API в списке активных.
    if (!m_isFileExist(file))
    {
        m_dataMutex.unlock();
        return false;
    }

    // Удалить файл API из списка активных.
    m_delFileFromPool(file);
    return true;
}

//-----------------------------------------------------------------------------

void Statistic::getActiveDevs(std::stringstream & pkg)
{
    std::lock_guard<std::mutex> lock(m_dataQMutex);
    for (auto it = m_devs.begin(); it != m_devs.end(); )
    {
        auto devs = it->second->getActive();
        for (auto dev = devs.begin(); dev != devs.end(); )
        {
            pkg << m_getHexAddr(dev->first);
            pkg << sep::dataSep << std::dec << dev->second << sep::dataSep;
            // Добавить частоту считывания в пакет.
            pkg << m_getFreq(it->first);

            if (++dev != devs.end())
                pkg << sep::dataSep;
        }

        if (++it != m_devs.end())
            pkg << sep::dataSep;
    }
}

//-----------------------------------------------------------------------------

void Statistic::getActiveFiles(std::stringstream & pkg)
{
    std::lock_guard<std::mutex> lock(m_dataQMutex);
    for (auto it = m_apis.begin(); it != m_apis.end(); )
    {
        auto files = it->second->getActive();
        for (auto file = files.begin(); file != files.end(); )
        {
            pkg << file->second << sep::dataSep;
            // Добавить частоту считывания в пакет.
            pkg << m_getFreq(it->first);

            if (++file != files.end())
                pkg << sep::dataSep;
        }

        if (++it != m_apis.end())
            pkg << sep::dataSep;
    }
}

//-----------------------------------------------------------------------------

bool Statistic::readDevOnce(dev::devInfo_t & devInfo, std::stringstream & data)
{
    dev::Device dev {devInfo};
    dev::region_t region;

    // Задать размер региона.
    region.resize(devInfo.second);
    // Заполнить регион начальными базовыми адресами.
    dev.fillAddrs(region);

    // Прочитать данные устойства.
    if (!dev.read(region))
        return false;

    // Сформировать пакет.
    m_addReg(region, data);
    return true;
}

//-----------------------------------------------------------------------------

bool Statistic::readApiOnce(dev::file_t & fileInfo, std::stringstream & data)
{
    dev::DevApi api {fileInfo.first};

    data << fileInfo.second << sep::dataSep;
    // Прочитать файл API.
    if (api.read(data))
        return true;

    // Очистить пакет.
    data.str(std::string());
    return false;
}

//-----------------------------------------------------------------------------

bool Statistic::readStatistic(std::stringstream & pkg)
{
    std::lock_guard<std::mutex> lock(m_dataQMutex);

    // Вернуться, если очередь пуста.
    if (m_dataQ.size() == 0)
        return false;

    // Передать пакет читающему потоку.
    pkg = std::move(m_dataQ.front());
    // Удалить пакет из очереди пакетов.
    m_dataQ.pop();

    return true;
}

//-----------------------------------------------------------------------------

void Statistic::stopStatistic()
{
    m_dataMutex.lock();
    // Если поток уже остановлен.
    if (!m_activated)
    {
        m_dataMutex.unlock();
        return;
    }

    // Удалить все устройства.
    m_delAllDevs();
    // Удалить все файлы API.
    m_delAllApis();

    m_activated = false;
    m_dataMutex.unlock();
    m_statThread.join();
}

//-----------------------------------------------------------------------------

bool Statistic::m_isDevExist(uint32_t & addr)
{
    for (auto it = m_devs.begin(); it != m_devs.end(); it++)
    {
        if (it->second->isExist(addr))
            return true;
    }

    return false;
}

//-----------------------------------------------------------------------------

bool Statistic::m_isFileExist(dev::file_t & fileName)
{
    for (auto it = m_apis.begin(); it != m_apis.end(); it++)
    {
        if (it->second->isExist(fileName))
            return true;
    }

    return false;
}

//-----------------------------------------------------------------------------

void Statistic::m_addRegs(std::stringstream & data, dev::devsRegion_t * region)
{
    for (unsigned int i = 0; i < region->size(); i++)
    {
        // Добавить разделитель для устройства.
        if (data.str().size())
            data << sep::dataSep;

        // Добавить регион устройства в пакет.
        m_addReg((*region)[i], data);
    }
}

//-----------------------------------------------------------------------------

void Statistic::m_doStat(Statistic * stat)
{
    while (stat->m_activated.load())
    {
        // Заснуть на определенный срок.
        stat->m_timer.sleep();
        // Проверить список работ и сформировать данные, если есть.
        stat->m_parseData();
        // Увеличить количество пройденных тиков.
        ++stat->m_timer;
    }
    stat->log.trace(__FILE__, EP7TRACE_LEVEL_INFO, (tUINT16)__LINE__,
                    __FUNCTION__, "Statistics parsing thread stopped");
}

//-----------------------------------------------------------------------------

bool Statistic::m_startStat()
{
    // Поток уже запущен.
    if (m_activated.load())
        return true;

    m_activated = true;
    m_statThread = std::thread(m_doStat, this);
    if (!m_statThread.joinable())
    {
        LOGGER_ERROR("Can't start statistic parsing in thread");
        return false;
    }

    LOGGER_INFO("Statistic parsing started in thread");
    return true;
}

//-----------------------------------------------------------------------------

void Statistic::m_stopStat()
{
    if ((m_devs.size() != 0) || (m_apis.size() != 0))
    {
        m_dataMutex.unlock();
        return;
    }

    // Если нет актиных задач, то завершить цикл сбора статистики.
    m_activated = false;
    m_dataMutex.unlock();
    m_statThread.join();
}

//-----------------------------------------------------------------------------

void Statistic::m_deleteDevsMem(dev::Devices * devs)
{
    delete devs;
}

//-----------------------------------------------------------------------------

void Statistic::m_deleteFilesMem(dev::DevsApi * apis)
{
    delete apis;
}

//-----------------------------------------------------------------------------

void Statistic::m_delDevFromPool(uint32_t & addr)
{
    // Удалить устройство из списка активных задач.
    for (auto it = m_devs.begin(); it != m_devs.end(); it++)
    {
        // Удалить устройство из пула, если оно там присутсвует.
        if (!it->second->remove(addr))
            continue;
        // Если это последнее устройство - удалить пул для этого тика.
        if (!it->second->isActive())
        {
            // Удалить память, выделенную под класс устройства.
            m_deleteDevsMem(it->second);
            // Уменьшить список активных задач.
            m_devs.erase(it);
            break;
        }
    }
    // Остановить сбор статистики, если это последнее устройство в пуле.
    m_stopStat();
}

//-----------------------------------------------------------------------------

void Statistic::m_delFileFromPool(dev::file_t & file)
{
    // Удалить файл API из списка активных задач.
    for (auto it = m_apis.begin(); it != m_apis.end(); it++)
    {
        // Удалить файл API из пула чтения.
        if (!it->second->remove(file))
            continue;
        // Если это последний файл - удалить пул для этого тика.
        if (!it->second->isActive())
        {
            // Удалить память, выделенную под класс файлов API.
            m_deleteFilesMem(it->second);
            // Уменьшить список активных файлов чтения.
            m_apis.erase(it);
            break;
        }
    }
    // Остановить сбор статистики, если это последний файл API в пуле.
    m_stopStat();
}

//-----------------------------------------------------------------------------

std::list<Statistic::devJob_t>::const_iterator
Statistic::m_isDevTickExist(timers::ticks_t & ticks)
{
    for (auto it = m_devs.begin(); it != m_devs.end(); it++)
    {
        if (ticks == it->first)
            return it;
    }

    return m_devs.end();
}

//-----------------------------------------------------------------------------

std::list<Statistic::apiJob_t>::const_iterator
Statistic::m_isFileTickExist(timers::ticks_t & ticks)
{
    for (auto it = m_apis.begin(); it != m_apis.end(); it++)
    {
        if (ticks == it->first)
            return it;
    }

    return m_apis.end();
}

//-----------------------------------------------------------------------------

void Statistic::m_addDev(dev::devInfo_t & dev, timers::ticks_t & ticks)
{
    auto it = m_isDevTickExist(ticks);
    // Добавить устройство, если такая частота считывания уже есть.
    if (it != m_devs.end())
    {
        LOGGER_DEBUG("Dev read frequency exists");
        it->second->add(dev);
        return;
    }
    LOGGER_DEBUG("Dev read frequency isn't exist");

    // Создать класс, который будет обслуживать устройства с такой частотой считывания.
    dev::Devices * devs = new dev::Devices(dev);
    devJob_t pair{ticks, devs};
    m_devs.push_back(pair);
}

//-----------------------------------------------------------------------------

void Statistic::m_addFile(dev::file_t & file, timers::ticks_t & ticks)
{
    auto it = m_isFileTickExist(ticks);
    // Добавить файл API, если такая частота считывания уже есть.
    if (it != m_apis.end())
    {
        LOGGER_DEBUG("Api read frequency exists");
        it->second->add(file);
        return;
    }
    LOGGER_DEBUG("Api read frequency isn't exist");

    // Создать класс, который будет обслуживать файлы API с такой частотой считывания.
    dev::DevsApi * apis = new dev::DevsApi(file);
    apiJob_t pair{ticks, apis};
    m_apis.push_back(pair);
}

//-----------------------------------------------------------------------------

std::string Statistic::m_getFreq(timers::ticks_t & ticks)
{
    std::stringstream pkg;
    pkg << m_timer.ticksToHz(ticks);

    return pkg.str();
}

//-----------------------------------------------------------------------------

std::string Statistic::m_getHexAddr(uint32_t & addr)
{
    std::stringstream pkg;
    pkg << "0x" << std::setfill('0') << std::setw(8) << std::hex << addr;

    return pkg.str();
}

//-----------------------------------------------------------------------------

void Statistic::m_delAllDevs()
{
    // Удалить все устройства из списка активных задач.
    for (auto it = m_devs.begin(); it != m_devs.end(); it++)
    {
        // Удалить все устройства из пула.
        it->second->removeAll();
        // Удалить пул для этого тика.
        m_deleteDevsMem(it->second);
    }
    m_devs.resize(0);
}

//-----------------------------------------------------------------------------

void Statistic::m_delAllApis()
{
    for (auto it = m_apis.begin(); it != m_apis.end(); it++)
    {
        // Удалить все файлы API из пула чтения.
        it->second->removeAll();
        // Удалить пул для этого тика.
        m_deleteFilesMem(it->second);
    }
    m_apis.resize(0);
}

//-----------------------------------------------------------------------------

void Statistic::m_addData(std::stringstream & devsData,
                          std::stringstream & apisData)
{
    std::lock_guard<std::mutex> lock(m_dataMutex);

    // Пройти по списку устройств и если время считывания совпадает, то добавить данные в пакет.
    m_addDevsData(devsData);
    // Пройти по списку файлов API и если время считывания совпадает, то добавить данные в пакет.
    m_addApisData(apisData);
}

//-----------------------------------------------------------------------------

void Statistic::m_addPkgToQueue(std::stringstream & devsData,
                                std::stringstream & apisData)
{
    std::lock_guard<std::mutex> lock(m_dataQMutex);

    // СОВМЕСТИТЬ ДАННЫЕ ???

    // Добавить пакет с данными от устройств (если есть).
    if (devsData.str().size())
        m_dataQ.push(std::move(devsData));
    // Добавить пакет с данными от файлов API (если есть).
    if (apisData.str().size())
        m_dataQ.push(std::move(apisData));
}

//-----------------------------------------------------------------------------

void Statistic::m_addDevsData(std::stringstream & devsData)
{
    dev::devsRegion_t * region;

    for (auto it = m_devs.begin(); it != m_devs.end(); it++)
    {
        // Пропустить, если время считывания данных не настало.
        if (!m_timer.isNow(it->first))
            continue;

        // Прочитать данные из устройств.
        it->second->read(&region);
        // Добавить данные в пакет.
        m_addRegs(devsData, region);
    }
}

//-----------------------------------------------------------------------------

void Statistic::m_addApisData(std::stringstream & apisData)
{
    for (auto it = m_apis.begin(); it != m_apis.end(); it++)
    {
        // Пропустить, если время считывания данных не настало.
        if (!m_timer.isNow(it->first))
            continue;

        // Прочитать данные из устройств.
        it->second->read(apisData);
    }
}

//-----------------------------------------------------------------------------

void Statistic::m_parseData()
{
    std::stringstream devsData;
    std::stringstream apisData;

    // Добавить данные устройств/API, если они имеются.
    m_addData(devsData, apisData);

    // Когда не было считанных данных.
    if (!(devsData.str().size() || apisData.str().size()))
        return;

    // Добавить сформированный пакет в очередь пакетов.
    m_addPkgToQueue(devsData, apisData);

    // Оповестить ждущий поток, что есть данные для считывания.
    m_pkg->notify_one();
}

//-----------------------------------------------------------------------------

void Statistic::m_addReg(dev::region_t & reg, std::stringstream & data)
{
    for (auto it = reg.begin(); it != reg.end(); )
    {
        data << m_getHexAddr(it->first);
        data << sep::dataSep;
        data << m_getHexAddr(it->second);

        if (++it != reg.end())
            data << sep::dataSep;
    }
}

//=============================================================================

} // namespace stat
