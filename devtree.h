#ifndef DEVTREE_H
#define DEVTREE_H

#include <iostream>
#include <filesystem>
#include <vector>
#include <unordered_set>

#include "device.h"
#include "logger-library/logger.h"

//-----------------------------------------------------------------------------

namespace devtree
{

//=============================================================================

// CPU (int(answer['CPU_temp']) * 503.975)/4096) - 273.15

// Определен ниже.
class DriverApi;

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
        devtree::DriverApi * api;   // Для взаимодействия с API устройства.
    };

    // Содержит вектор файлов с их значениями в дереве устройств.
    typedef std::vector<m_devData> m_devTree_t;
    // Вектор директорий, которые содержат данные об устройствах на шине AMBA.
    m_devTree_t m_ambaPl;

    //-------------------------------------------------------------------------

    // Заполнить вектор директорий информацией об устройствах на шине.
    void m_fillAmbaPlData();
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
    DriverApi * m_getDevApi(m_values_t & data);
    // Получить базовый адрес устройства.
    std::string m_getBaseAddr(std::vector<char> & data);
    // Получить количество регистров устройства.
    std::string m_getRegCnt(std::vector<char> & data);
    // Получить имя файла из полного пути.
    std::string m_getFileName(const std::string & fullPath);
    // Получить все имена файлов и их содержимое в директории.
    m_values_t m_getAllInsideDir(const std::string & path);
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

class DriverApi
{
public:

    DriverApi(std::string compatible, std::string addr);

    //-------------------------------------------------------------------------

    // Вернуть функционал API.
    std::vector<std::string> getApi();
    // Вернуть полный путь к файлу API.
    std::string getPath(std::string & apiName);

private:

    // Класс логирования.
    logger::Logger log;

    // Имя файла.
    typedef std::string m_file_t;

    // Путь к расположению файлов.
    std::string m_apiPath;
    // Имя файла в API драйвера.
    std::unordered_set<m_file_t> m_apiFiles;

    //-------------------------------------------------------------------------

    // Получить все файлы API устройства.
    void m_getApiFiles();

    //-------------------------------------------------------------------------

    // Преобразовать compatible в используемый вид.
    std::string m_createAvailableCompatible(std::string compatible);

    //-------------------------------------------------------------------------

    // Проверить есть ли такой файл API.
    bool m_isExist(std::string & fileName);
};

//=============================================================================

} // namespace devtree

#endif // DEVTREE_H
