#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>

#include "device.h"
#include <atomic>

//-----------------------------------------------------------------------------

namespace dev
{

//=============================================================================

Device::Device(devInfo_t & dev)
    : m_fd(-1), m_dev(dev), m_mapBase(nullptr), m_mappedFlag(false)
{
    // Получить смещение на странице памяти.
    m_offset = (unsigned int)(m_dev.first & (m_pagesize - 1));

    //-------------------------------------------------------------------------

    // Отобразить регион.
    m_mappedFlag = m_mapRegion();
    if (!m_mappedFlag)
        LOGGER_ERROR("Cannot map device");
}

//-----------------------------------------------------------------------------

Device::~Device()
{
    // Прекратить отображение региона.
    if (m_mapBase != MAP_FAILED)
    {
        if (munmap(m_mapBase, m_pagesize) != 0)
            LOGGER_ERROR("Cannot unmap device registers");
    }

    //-------------------------------------------------------------------------

    // Закрыть символьное устройство.
    if (m_fd != -1)
        close(m_fd);
}

//-----------------------------------------------------------------------------

void Device::fillAddrs(region_t & region)
{
    // Заполнить регион адресами.
    for (unsigned int i = 0; i < m_dev.second; i++)
        region[i].first = m_dev.first + i * m_step;
}

//-----------------------------------------------------------------------------

bool Device::read(region_t & region)
{
    // Проверить отображено ли устройство.
    if (!m_mappedFlag)
    {
        LOGGER_ERROR("Device isn't remapped");
        return false;
    }

    // Прочитать регион.
    m_readRegion(region);
    return true;
}

//-----------------------------------------------------------------------------

bool Device::write(uint32_t & value)
{
    void * virt_addr;

    // Проверить отображено ли устройство.
    if (!m_mappedFlag)
    {
        LOGGER_ERROR("Device isn't remapped");
        return false;
    }

    auto tmpOffset = m_offset;
    // Записать значение по адресам.
    for (unsigned int i = 0; i < m_dev.second; i++)
    {
        virt_addr = m_mapBase + tmpOffset;
        *((uint32_t *) virt_addr) = value;
        tmpOffset += m_step;
    }

    return true;
}

//-----------------------------------------------------------------------------

bool Device::m_mapRegion()
{
    if (!m_dev.second)
    {
        LOGGER_ERROR("Register count isn't specified");
        return false;
    }

    // Открыть символьное устройство.
    if (!m_openChrdev())
        return false;

    // Отобразить устройство в физическую память.
    if (!m_mmap())
        return false;

    return true;
}

//-----------------------------------------------------------------------------

void Device::m_readRegion(region_t & region)
{
    void * virt_addr;

    auto tmpOffset = m_offset;
    // Прочитать значения регистров в устройстве.
    for (unsigned int i = 0; i < m_dev.second; i++)
    {
        virt_addr = m_mapBase + tmpOffset;
        region[i].second = *((uint32_t *) virt_addr);
        tmpOffset += m_step;
    }
}

//-----------------------------------------------------------------------------

bool Device::m_openChrdev()
{
    m_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (m_fd == -1)
    {
        std::stringstream msg;
        msg << "Error opening /dev/mem (" << errno << ") : "
            << strerror(errno);
        LOGGER_ERROR(msg.str());
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------

bool Device::m_mmap()
{
    m_mapBase = mmap(0, m_pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd,
                    m_dev.first & ~((typeof(m_dev.first))m_pagesize - 1));

    if (m_mapBase == MAP_FAILED)
    {
        std::stringstream msg;
        msg << "Error mapping (" << errno << ") : "
            << strerror(errno);
        LOGGER_ERROR(msg.str());
        return false;
    }

    return true;
}

//=============================================================================
//=============================================================================

Devices::Devices(devInfo_t & dev)
{
    // Создать устройство.
    m_createDev(dev);
}

//-----------------------------------------------------------------------------

Devices::~Devices()
{
    // Удалить устройства.
    m_deleteDevs();
    // Удалить регионы устройств.
    m_deleteRegions();
}

//-----------------------------------------------------------------------------

bool Devices::read(devsRegion_t ** regions)
{
    // Прочитать регионы устройств.
    auto ret = m_readRegions();
    if (!ret)
        return ret;

    // Получить регионы устройств.
    m_getRegions();

    // Установить указатель на прочитанные регионы устройств.
    *regions = &m_regions;
    return true;
}

//-----------------------------------------------------------------------------

bool Devices::m_isExist(uint32_t & addr)
{
    for (unsigned int i = 0; i < m_devs.size(); i++)
    {
        if (addr == m_devs[i]->devInfo.first)
            return true;
    }

    return false;
}

//-----------------------------------------------------------------------------

bool Devices::add(devInfo_t & devInfo)
{
    std::lock_guard<std::mutex> lock(m_devsMutex);

    // Проверить есть ли такое устройство в пуле.
    if (m_isExist(devInfo.first))
    {
        LOGGER_DEBUG("Device exists");
        return false;
    }
    LOGGER_DEBUG("Device ins't exist");

    // Создать и добавить устройство к общему пулу.
    m_createDev(devInfo);
    return true;
}

//-----------------------------------------------------------------------------

bool Devices::remove(uint32_t & addr)
{
    std::lock_guard<std::mutex> lock(m_devsMutex);

    for (auto it = std::begin(m_devs); it != std::end(m_devs); it++)
    {
        if (addr == (*it)->devInfo.first)
        {
            m_deleteDev(*it);
            m_devs.erase(it);
            return true;
        }
    }

    return false;
}

//-----------------------------------------------------------------------------

void Devices::removeAll()
{
    std::lock_guard<std::mutex> lock(m_devsMutex);

    for (auto it = std::begin(m_devs); it != std::end(m_devs); it++)
        m_deleteDev(*it);

    m_devs.resize(0);
}

//-----------------------------------------------------------------------------

devsInfo_t Devices::getActive()
{
    devInfo_t devInfo;
    // Список активных устройств.
    devsInfo_t devs;

    std::lock_guard<std::mutex> lock(m_devsMutex);
    for (auto it = m_devs.begin(); it != m_devs.end(); it++)
    {
        devInfo.first = (*it)->devInfo.first;
        devInfo.second = (*it)->devInfo.second;
        devs.push_back(devInfo);
    }

    return devs;
}

//-----------------------------------------------------------------------------

bool Devices::isExist(uint32_t & addr)
{
    std::lock_guard<std::mutex> lock(m_devsMutex);
    return m_isExist(addr);
}

//-----------------------------------------------------------------------------

bool Devices::isActive()
{
    std::lock_guard<std::mutex> lock(m_devsMutex);
    return bool(m_devs.size());
}

//-----------------------------------------------------------------------------

void Devices::m_createDev(devInfo_t & devInfo)
{
    m_dev_t * devData = new m_dev_t;
    devData->dev = new Device(devInfo);
    devData->region = new region_t;
    // Задать размер региона для сохранения прочитанных данных.
    devData->region->resize(devInfo.second);
    devData->devInfo = devInfo;
    // Заполнить регион начальными базовыми адресами.
    region_t * reg = devData->region;
    devData->dev->fillAddrs(*reg);
    m_devs.push_back(devData);
}

//-----------------------------------------------------------------------------

void Devices::m_deleteDev(m_dev_t * devInfo)
{
    delete devInfo->dev;
    delete devInfo->region;
    delete devInfo;
}

//-----------------------------------------------------------------------------

void Devices::m_deleteDevs()
{
    std::lock_guard<std::mutex> lock(m_devsMutex);

    auto devsCnt = m_devs.size();
    // Удалить устройства, начиная с конца.
    for (int i = (int) devsCnt - 1; i >= 0; i--)
    {
        m_deleteDev(m_devs[i]);
        m_devs.pop_back();
    }
}

//-----------------------------------------------------------------------------

void Devices::m_deleteRegions()
{
    m_regions.resize(0);
}

//-----------------------------------------------------------------------------

bool Devices::m_readRegions()
{
    bool ret;

    std::lock_guard<std::mutex> lock(m_devsMutex);
    for (unsigned int i = 0; i < m_devs.size(); i++)
    {
        ret = m_devs[i]->dev->read(*m_devs[i]->region);
        if (!ret)
            return false;
    }

    return true;
}

//-----------------------------------------------------------------------------

void Devices::m_getRegions()
{
    m_regions.resize(0);

    std::lock_guard<std::mutex> lock(m_devsMutex);
    for (unsigned int i = 0; i < m_devs.size(); i++)
    {
        region_t tmpRegion = *m_devs[i]->region;
        m_regions.push_back(tmpRegion);
    }
}

//=============================================================================

} // namepsace dev

