#ifndef STATISTIC_H
#define STATISTIC_H

//-----------------------------------------------------------------------------

#include <list>
#include <queue>
#include <condition_variable>
#include <thread>
#include <atomic>

#include "device.h"
#include "timers.h"
#include "devapi.h"

namespace statistic
{

//=============================================================================

class Statistic
{
public:

    Statistic(std::condition_variable * notify);
    ~Statistic();

    //-------------------------------------------------------------------------

    // Добавить устройство к пулу.
    bool addDev(dev::devInfo_t & dev, timers::hz_t hz);
    // Добавить файл API к пулу чтения.
    bool addFile(dev::file_t & file, timers::hz_t hz);
    // Удалить устройство из пула.
    bool delDev(uint32_t & addr);
    // Удалить файл API из пула.
    bool delFile(dev::file_t & file);
    // Вернуть список устройств, для которых собирается статистика.
    void getActiveDevs(std::stringstream & pkg);
    // Вернуть список файлов API, для которых собирается статистика.
    void getActiveFiles(std::stringstream & pkg);
    // Прочитать устройство один раз.
    bool readDevOnce(dev::devInfo_t & devInfo, std::stringstream & data);
    // Прочитать файл API один раз.
    bool readApiOnce(dev::file_t & fileInfo, std::stringstream & data);

    //-------------------------------------------------------------------------

    // Прочитать статистику ждущим потоком.
    bool readStatistic(std::stringstream & pkg);
    // Остановить сбор статистики.
    void stopStatistic();

    //-------------------------------------------------------------------------

private:

    // Класс логирования.
    logger::Logger log;

    //-------------------------------------------------------------------------

    // Задача для устройств (частота считывания и указатель на класс устройств).
    typedef std::pair<timers::ticks_t, dev::Devices *> devJob_t;
    // Список устройств, для которых собирается статистика.
    std::list<devJob_t> m_devs;

    //-------------------------------------------------------------------------

    // Задача для файлов API (частота считывания и указатель на класс файлов API).
    typedef std::pair<timers::ticks_t, dev::DevsApi *> apiJob_t;
    // Список файлов API, для которых собирается статистика.
    std::list<apiJob_t> m_apis;

    //-------------------------------------------------------------------------

    // Для работа с любыми типами данных, которые связаны с устройствами и файлами API.
    std::mutex m_dataMutex;

    //-------------------------------------------------------------------------

    // Проверить есть ли такое устройство в списке активных.
    bool m_isDevExist(uint32_t & addr);
    // Проверить существует ли такой файл API в списке активных.
    bool m_isFileExist(dev::file_t & fileName);

    //-------------------------------------------------------------------------

    // Очередь сформированных пакетов для отправки пользователю.
    std::queue<std::stringstream> m_dataQ;
    // Мьютекс для работы с очередью пакетов.
    std::mutex m_dataQMutex;
    // Для оповещения ждущего сервера, что есть готовый пакет для отправки.
    std::shared_ptr<std::condition_variable> m_pkg;

    //-------------------------------------------------------------------------

    // Добавить данные из устройств.
    void m_addRegs(std::stringstream & data, dev::devsRegion_t * region);
    // Добавить регион устройства.
    void m_addReg(dev::region_t & reg, std::stringstream & data);
    // Добавить частоту считывания.
    std::string m_getFreq(timers::ticks_t & ticks);
    // Добавить адрес в HEX формате в пакет.
    std::string m_getHexAddr(uint32_t & addr);
    // Добавить прочитанные данные.
    void m_addData(std::stringstream & devsData, std::stringstream & apisData);
    // Добавить данные устройств.
    void m_addDevsData(std::stringstream & devsData);
    // Добавить данные файлов API.
    void m_addApisData(std::stringstream & apisData);

    //-------------------------------------------------------------------------

    // Таймер для работы со статистикой.
    timers::Tick m_timer;
    // Поток сбора статистики.
    std::thread m_statThread;
    // Переменная, обозначающая, что поток запущен.
    std::atomic<bool> m_activated;
    // Функция сбора статистики.
    static void m_doStat(Statistic * stat);
    // Функция запуска сбора статистики.
    bool m_startStat();
    // Функция остановки сбора статистики.
    void m_stopStat();

    //-------------------------------------------------------------------------

    // Очистить память класса устройства.
    void m_deleteDevsMem(dev::Devices * devs);
    // Очистить память класса файлов API.
    void m_deleteFilesMem(dev::DevsApi * apis);

    //-------------------------------------------------------------------------

    // Удалить устройство из пула задач.
    void m_delDevFromPool(uint32_t & addr);
    // Удалить файл API из пула чтения.
    void m_delFileFromPool(dev::file_t & file);
    // Удалить все устройства из всех списков.
    void m_delAllDevs();
    // Удалить все файлы API из всех списков.
    void m_delAllApis();

    //-------------------------------------------------------------------------

    // Проверить существует ли такая частота считывания для устройств.
    std::list<devJob_t>::const_iterator
    m_isDevTickExist(timers::ticks_t & ticks);
    // Проверить существует ли такая частота считывания для файлов API.
    std::list<apiJob_t>::const_iterator
    m_isFileTickExist(timers::ticks_t & ticks);

    //-------------------------------------------------------------------------

    // Добавить устройство в пул сбора статистики.
    void m_addDev(dev::devInfo_t & dev, timers::ticks_t & ticks);
    // Добавить файл API в пул сбора статистики.
    void m_addFile(dev::file_t & file, timers::ticks_t & ticks);

    //-------------------------------------------------------------------------

    // Сформировать пакет для отправки из полученных данных от устройств/API.
    void m_parseData();
    // Добавить пакет в очередь.
    void m_addPkgToQueue(std::stringstream & devsData,
                         std::stringstream & apisData);
};

//=============================================================================

}

#endif // STATISTIC_H
