#include <iterator>
#include <fstream>
#include <iostream>
#include <vector>

#include "devtree.h"
#include "common.h"

namespace devtree
{

//=============================================================================

DevTree::DevTree()
    : m_path{"/proc/device-tree/amba_pl/"}
{
    m_fillAmbaPlData();
    m_fillSysData();
}

//-----------------------------------------------------------------------------

void DevTree::getDtb(std::stringstream & pkg)
{
    for (auto it = m_ambaPl.begin(); it != m_ambaPl.end(); )
    {
        // Записать имя устройства.
        pkg << it->dtbDevName << sep::dataSep;
        // Записать в пакет имена файлов и их содрежимое для конкретного устройства.
        for (auto fd = it->baseInfo.begin(); fd != it->baseInfo.end(); )
        {
            pkg << fd->first << sep::dataSep << fd->second;
            if (++fd != it->baseInfo.end())
                pkg << sep::dataSep;
        }

        if (++it != m_ambaPl.end())
            pkg << sep::dataSep;
    }
}

//-----------------------------------------------------------------------------

bool DevTree::getApi(std::string & dev, std::stringstream & data)
{
    auto devIter = m_isDevExist(dev);
    // Нет такого устройства.
    if (devIter == m_ambaPl.end())
        return false;

    // Получить функционал API.
    auto api = devIter->api->getApi();
    if (!api.size())    // Нет API.
        return false;

    // Сохранить имя устройства.
    data << devIter->dtbDevName;
    data << sep::dataSep;
    // Сохранить файлы API.
    for (auto apiFd = api.begin(); apiFd != api.end(); )
    {
        data << *apiFd;
        if (++apiFd != api.end())
            data << sep::dataSep;
    }

    return true;
}

//-----------------------------------------------------------------------------

bool DevTree::checkDev(dev::devInfo_t & dev)
{
    for (auto it = m_ambaPl.begin(); it != m_ambaPl.end(); it++)
    {
        if (dev.first >= it->dev.first &&
                dev.first + dev.second * 4 <= it->dev.first + it->dev.second)
        {
            return true;
        }
    }

    return false;
}

//-----------------------------------------------------------------------------

std::string DevTree::getApiPath(std::string & api)
{
    // Разбить на имя и файл устройства.
    auto data = splitString(api, sep::baseSep);

    for (auto it = m_ambaPl.begin(); it != m_ambaPl.end(); it++)
    {
        if (it->dtbDevName == data[0])
            return it->api->getPath(data[1]);
    }

    return "";
}

//-----------------------------------------------------------------------------

void DevTree::m_fillAmbaPlData()
{
    m_devData pushedVal;

    for (const auto & entry : std::filesystem::directory_iterator(m_path))
    {
        // Если это директория и имя директории содержит ...@some_addr.
        if (entry.is_directory() &&
                (entry.path().string().find("@") != std::string::npos))
        {
            // Получить имя директории в /proc/device-tree/...
            pushedVal.dtbDevName = m_getFileName(entry.path());;
            pushedVal.baseInfo = m_getAllInsideDir(entry.path());
            pushedVal.api = m_getDevApi(pushedVal.baseInfo);
            pushedVal.dev = m_fillDevsInfo(pushedVal.baseInfo);
            m_ambaPl.push_back(pushedVal);
        }
    }
}

//-----------------------------------------------------------------------------

void DevTree::m_fillSysData()
{
    std::vector<m_sysDev_t> data;

    // Получить данные из конфига
    if (!m_readConfig(data))
        return;

    // Не было данных.
    if (!data.size())
        return;

    m_devData pushedVal;
    // Заполнить данные для устройств из конфига.
    for (auto it = data.begin(); it != data.end(); it++)
    {
        pushedVal.dtbDevName = it->first;
        pushedVal.baseInfo = m_getAllInsideDir(it->second + "/of_node");
        pushedVal.api = m_getSysApi(*it);
        pushedVal.dev = m_fillDevsInfo(pushedVal.baseInfo);
        m_ambaPl.push_back(pushedVal);
    }
}

//-----------------------------------------------------------------------------

DevTree::m_values_t DevTree::m_getAllInsideDir(const std::string & path)
{
    m_values_t data;
    m_value_t tmp;

    for (const auto & entry : std::filesystem::directory_iterator(path))
    {
        if (entry.is_regular_file())
        {
            // Прочитать данные только из нужных файлов.
            if (!m_isFileNeeded(entry.path(), tmp))
                continue;
            data.push_back(tmp);
        }
    }

    return data;
}

//-----------------------------------------------------------------------------

std::string DevTree::m_getAddr(std::string & reg)
{
    auto data = splitString(reg, sep::dataSep);
    // Удалить префикс 0x из адреса.
    auto addr = data[0].substr(2, 10);

    return addr;
}

//-----------------------------------------------------------------------------

std::string DevTree::m_getCompatible(std::string & compatible)
{
    // Замена всех символов (кроме цифр и букв) в compatible.
    std::string availableCompatible;
    for (auto it = compatible.begin(); it != compatible.end(); it++)
    {
        if (!std::isalnum(*it))
            availableCompatible += "_";
        else
            availableCompatible += *it;
    }

    return availableCompatible;
}

//-----------------------------------------------------------------------------

std::string DevTree::m_getDevApiPath(std::string & compatible,
                                     std::string & addr)
{
    return "/sys/class/" + compatible + "/" + addr + "/control";
}

//-----------------------------------------------------------------------------

bool DevTree::m_readConfig(std::vector<m_sysDev_t> & data)
{
    auto params = cfg::ini::parseConfig("/opt/control/conf", "tpoprotocol.ini",
                                        "DEVICES");
    if (!params)
    {
        LOGGER_ERROR("Can't find parameters for device configuration");
        return false;
    }

    // Получить alias-ы для устройств из файла конфига.
    auto keys = params->getKeys();
    if (!keys.size())
    {
        LOGGER_INFO("Can't find system devices in configuration file");
        return false;
    }

    // Заполнить базовую информацию о системных устройствах.
    m_fillSysDev(params, keys, data);
    return true;
}

//-----------------------------------------------------------------------------

void DevTree::m_fillSysDev(cfg::ini::iniParams_t & params,
                           std::vector<std::string> & keys,
                           std::vector<m_sysDev_t> & sysDevs)
{
    m_sysDev_t sysDev;
    for (auto it = keys.begin(); it != keys.end(); it++)
    {
        // Получить путь для системного устройства.
        auto path = params->get(*it);
        if (!path.size())
            continue;

        sysDev.first = *it;
        sysDev.second = path;
        sysDevs.push_back(sysDev);
    }
}

//-----------------------------------------------------------------------------

std::string DevTree::m_readFile(const std::string & fileName)
{
    std::ifstream f(fileName);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

//-----------------------------------------------------------------------------

std::string DevTree::m_readCompatible(const std::string & fullPath)
{
    auto comp = m_readFile(fullPath);
    comp.pop_back();    // Удалить последний символ (\00).
    // Удалить из compatible префикс.
    auto vec = splitString(comp, ',');
    auto idx = (vec.size() > 1) ? 1 : 0;

    return vec[idx];
}

//-----------------------------------------------------------------------------

std::string DevTree::m_readReg(const std::string & fullPath)
{
    std::string data;

    // Прочитать содержимое файла.
    auto bytes = m_lowLRead(fullPath);
    // Заполнить массив нулями, если он меньше 8 символов.
    bytes.resize(8);
    // Получить базовый адрес устройства.
    data += m_getBaseAddr(bytes);
    // Добавить разделитель между базовым адресом и кол-ом регистров.
    data += sep::dataSep;
    // Получить количество регистров.
    data += m_getRegCnt(bytes);

    return data;
}

//-----------------------------------------------------------------------------

bool DevTree::m_isFileNeeded(const std::string & fullPath, m_value_t & save)
{
    auto fileName = m_getFileName(fullPath);

    // Прочитать содержимое файла compatible.
    if (fileName == "compatible")
    {
        save.first = fileName;
        save.second = m_readCompatible(fullPath);
        return true;
    }
    // Прочитать содержимое файла reg.
    if (fileName == "reg")
    {
        save.first = fileName;
        save.second = m_readReg(fullPath);
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------

std::vector<char> DevTree::m_lowLRead(const std::string & fullPath)
{
    std::vector<char> bytes;
    char byte;

    // Прочитать данные из файла.
    std::ifstream input_file(fullPath);
    while (input_file.get(byte))
        bytes.push_back(byte);
    input_file.close();

    return bytes;
}

//-----------------------------------------------------------------------------

std::string DevTree::m_getBaseAddr(std::vector<char> & data)
{
    std::stringstream tmp;

    // Сформировать базовый адрес.
    tmp << "0x";
    for (int i = 0; i < 4; i++)
    {
        tmp << std::setfill('0') << std::hex << std::setw(2)
            << (uint16_t) data[i];
    }

    return tmp.str();
}

//-----------------------------------------------------------------------------

std::string DevTree::m_getRegCnt(std::vector<char> & data)
{
    std::stringstream tmp;
    std::string regs;

    // Сформировать количество регистров.
    for (int i = 4; i < 8; i++)
    {
        tmp << std::hex << std::setfill('0') << std::setw(2)
            << (uint16_t) data[i];
    }

    // Взять только значащие нули.
    regs = tmp.str();
    for (unsigned i = 0; i != regs.size(); i++)
    {
        if (regs[i] == '0')
            continue;

        return regs.substr(i);
    }

    return "0";
}

//-----------------------------------------------------------------------------

DevTree::m_devTree_t::const_iterator
DevTree::m_isDevExist(std::string & devName)
{
    for (auto it = m_ambaPl.begin(); it != m_ambaPl.end(); it++)
    {
        if (devName == it->dtbDevName)
            return it;
    }

    return m_ambaPl.end();
}

//-----------------------------------------------------------------------------

dev::devInfo_t DevTree::m_fillDevsInfo(m_values_t & data)
{
    dev::devInfo_t devInfo;

    // Найти информацию о базовом адресе и количестве регистров.
    for (auto it = data.begin(); it != data.end(); it++)
    {
        if (it->first != "reg")
            continue;

        auto data = splitString(it->second, sep::dataSep);
        // Получить базовый адрес устройства.
        devInfo.first = strtoul(data[0].c_str(), NULL, 16);
        // Получить количество регистров устройства (в hex).
        devInfo.second = strtoul(data[1].c_str(), NULL, 16);
        break;
    }

    return devInfo;
}

//-----------------------------------------------------------------------------

Api * DevTree::m_getDevApi(m_values_t & data)
{
    std::string compatible;
    std::string addr;

    // Получить необходимые данные из данных устройства.
    for (auto it = data.begin(); it != data.end(); it++)
    {
        if (it->first == "compatible")
            compatible = m_getCompatible(it->second);
        if (it->first == "reg")
            addr = m_getAddr(it->second);
    }

    // Путь к расположению API устройства.
    auto path = m_getDevApiPath(compatible, addr);
    auto api = new Api {path};

    return api;
}

//-----------------------------------------------------------------------------

Api * DevTree::m_getSysApi(m_sysDev_t & data)
{
    auto api = new Api {data.second};
    return api;
}

//-----------------------------------------------------------------------------

std::string DevTree::m_getFileName(const std::string & fullPath)
{
    auto names = splitString(fullPath, '/');
    return names.back();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

Api::Api(file_t path)
    : m_path{path}
{}

//-----------------------------------------------------------------------------

std::vector<file_t> Api::getApi()
{
    std::vector<file_t> api;

    // Обновить функционал API.
    m_getApiFiles();
    for (auto it = m_files.begin(); it != m_files.end(); it++)
        api.push_back(*it);

    return api;
}

//-----------------------------------------------------------------------------

file_t Api::getPath(file_t & apiName)
{
    if (!m_isExist(apiName))
        return "";

    return m_path + "/" + apiName;
}

//-----------------------------------------------------------------------------

void Api::m_getApiFiles()
{
    // Удалить прошлые сохраненные данные.
    m_files.clear();

    // Если путь не существует, то не считывать API.
    if (!std::filesystem::exists(m_path))
        return;

    // Получить весь функционал API в директории.
    try
    {
        for (const auto & entry :
             std::filesystem::directory_iterator(m_path))
        {
            if (entry.is_regular_file())
            {
                auto vec = splitString(entry.path(), '/');
                std::string apiName = vec[vec.size() - 1];
                m_files.insert(apiName);
            }
        }
    }
    catch (...)
    {
        m_files.clear();
        return;
    }
}

//-----------------------------------------------------------------------------

bool Api::m_isExist(std::string & fileName)
{
    // Обновить функционал API.
    m_getApiFiles();
    return m_files.find(fileName) != m_files.end();
}

//=============================================================================

} // namespace devtree
