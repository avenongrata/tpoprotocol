#ifndef DEVTREE_H
#define DEVTREE_H

#include <iostream>
#include <filesystem>
#include <vector>
#include <unordered_set>

#include "device.h"
#include "logger-library/logger.h"
#include "config-library/iiniparams.h"
#include "config-library/ciniparser.h"

//-----------------------------------------------------------------------------

namespace devtree
{

// Для обозначения имени файла.
typedef std::string file_t;

//=============================================================================

// CPU (int(answer['CPU_temp']) * 503.975)/4096) - 273.15

// Определен ниже.
class Api;

//-----------------------------------------------------------------------------

class DevTree
{
public:

    DevTree();
    ~DevTree() {};

    //-------------------------------------------------------------------------

    // Получить устройства из дерева устройств с именами файлов и значениями.
    void getDtb(std::stringstream & pkg);
    // Получить функционал API для конкретного устройства.
    bool getApi(std::string & dev, std::stringstream & data);
    // Получить полный путь к файлу API.
    std::string getApiPath(std::string & api);

    //-------------------------------------------------------------------------

    // Проверить существует ли устройство в дереве устройств.
    bool checkDev(dev::devInfo_t & dev);

private:

    // Класс логирования.
    logger::Logger log;

    //-------------------------------------------------------------------------

    // Связка alias:path для системного устройства.
    typedef std::pair<std::string, std::string> m_sysDev_t;

    //-------------------------------------------------------------------------

    // Путь к загруженному дереву устройств.
    std::string m_path;

    //-------------------------------------------------------------------------

    // Содержит имя файла и его содержимое.
    typedef std::pair<std::string, std::string> m_value_t;
    // Содержит вектор файлов и их содержимое.
    typedef std::vector<m_value_t> m_values_t;

    // Структура устройства из DTB.
    struct m_devData
    {
        std::string dtbDevName;     // Имя устройства в DTB.
        m_values_t baseInfo;        // Файлы и их содержимое с базовой информацией.
        dev::devInfo_t dev;         // Базовый адрес устройства и количество регистров.
        Api * api;                  // Для взаимодействия с API устройства.
    };

    // Содержит вектор файлов с их значениями в дереве устройств.
    typedef std::vector<m_devData> m_devTree_t;
    // Вектор директорий, которые содержат данные об устройствах на шине AMBA.
    m_devTree_t m_ambaPl;

    //-------------------------------------------------------------------------

    // Заполнить вектор директорий информацией об устройствах на шине.
    void m_fillAmbaPlData();
    // Заполнить вектор директорий информацией о системных устройствах на шине.
    void m_fillSysData();
    // Заполнить базовый адрес устройства и его количество регистров.
    dev::devInfo_t m_fillDevsInfo(m_values_t & data);

    //-------------------------------------------------------------------------

    // Проверить существует ли такое устройство.
    m_devTree_t::const_iterator m_isDevExist(std::string & devName);
    // Взять данные только из нужных файлов.
    bool m_isFileNeeded(const std::string & fullPath, m_value_t & save);

    //-------------------------------------------------------------------------

    // Прочитать содержимое файла.
    std::string m_readFile(const std::string & fileName);
    // Побайтовое чтение данных.
    std::vector<char> m_lowLRead(const std::string & fullPath);
    // Прочитать compatible для устройства.
    std::string m_readCompatible(const std::string & fullPath);
    // Прочитать reg для устройства.
    std::string m_readReg(const std::string & fullPath);

    //-------------------------------------------------------------------------

    // Получить API для устройства.
    Api * m_getDevApi(m_values_t & data);
    // Получить API для системного устрйоства.
    Api * m_getSysApi(m_sysDev_t & data);
    // Получить базовый адрес устройства.
    std::string m_getBaseAddr(std::vector<char> & data);
    // Получить количество регистров устройства.
    std::string m_getRegCnt(std::vector<char> & data);
    // Получить имя файла из полного пути.
    std::string m_getFileName(const std::string & fullPath);
    // Получить все имена файлов и их содержимое в директории.
    m_values_t m_getAllInsideDir(const std::string & path);
    // Получить адрес устройства из DTB.
    std::string m_getAddr(std::string & reg);
    // Получить преобразованный compatible из DTB.
    std::string m_getCompatible(std::string & compatible);
    // Получить путь к API устройства.
    std::string m_getDevApiPath(std::string & compatible, std::string & addr);

    //-------------------------------------------------------------------------

    // Чтение параметров из конфига.
    bool m_readConfig(std::vector<m_sysDev_t> & data);
    // Создать связку alias:path для системных устройств.
    void m_fillSysDev(cfg::ini::iniParams_t & params,
                      std::vector<std::string> & keys,
                      std::vector<m_sysDev_t> & sysDevs);
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

class Api
{
public:

    // Путь к расположению API устройства.
    Api(file_t path);

    //-------------------------------------------------------------------------

    // Вернуть функционал API.
    std::vector<file_t> getApi();
    // Вернуть полный путь к файлу API.
    file_t getPath(file_t & apiName);

private:

    // Класс логирования.
    logger::Logger log;

    // Путь к расположению API файлов.
    file_t m_path;
    // Имя API файла в драйвере.
    std::unordered_set<file_t> m_files;

    //-------------------------------------------------------------------------

    // Получить все файлы API драйвера.
    void m_getApiFiles();

    //-------------------------------------------------------------------------

    // Проверить есть ли такой файл API.
    bool m_isExist(std::string & fileName);
};

//=============================================================================

} // namespace devtree

#endif // DEVTREE_H
