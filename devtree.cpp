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
    return m_readFile(fullPath);
}

//-----------------------------------------------------------------------------

std::string DevTree::m_readReg(const std::string & fullPath)
{
    std::string data;

    // Прочитать содержимое файла.
    auto bytes = m_lowLRead(fullPath);
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
    std::string comp;

    // Прочитать содержимое файла compatible.
    if (fileName == "compatible")
    {
        save.first = fileName;
        comp = m_readCompatible(fullPath);
        comp.pop_back();    // Удалить последний символ (\00).
        save.second = comp;
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

    return "";
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

DriverApi * DevTree::m_getDevApi(m_values_t & data)
{
    DriverApi * api;
    std::string compatible;
    std::string addr;

    // Получить необходимые данные из данных устройства.
    for (auto it = data.begin(); it != data.end(); it++)
    {
        if (it->first == "compatible")
            compatible = it->second;

        if (it->first == "reg")
        {
            std::vector<std::string> tmp;
            tmp = splitString(it->second, ',');
            // Удалить префикс 0x из адреса.
            addr = tmp[0].substr(2, 10);
        }
    }

    api = new DriverApi {compatible, addr};
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

DriverApi::DriverApi(std::string compatible, std::string addr)
{
    compatible = m_createAvailableCompatible(compatible);
    m_apiPath = "/sys/class/" + compatible + "/" + addr + "/control";

    // Получить все имена файлов API в катологе для взаимодействия с драйвером.
    m_getApiFiles();
}

//-----------------------------------------------------------------------------

void DriverApi::m_getApiFiles()
{
    // Удалить прошлые сохраненные данные.
    m_apiFiles.clear();

    // Если путь не существует, то не считывать API.
    if (!std::filesystem::exists(m_apiPath))
        return;

    // Получить весь функционал API в директории.
    try
    {
        for (const auto & entry :
             std::filesystem::directory_iterator(m_apiPath))
        {
            if (entry.is_regular_file())
            {
                auto vec = splitString(entry.path(), '/');
                std::string apiName = vec[vec.size() - 1];
                m_apiFiles.insert(apiName);
            }
        }
    }
    catch (...)
    {
        m_apiFiles.clear();
        return;
    }

}

//-----------------------------------------------------------------------------

std::string DriverApi::m_createAvailableCompatible(std::string compatible)
{
    // Удалить из compatible префикс xlnx,...
    auto vec = splitString(compatible, ',');
    auto idx = (vec.size() > 1) ? 1 : 0;
    compatible = vec[idx];

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

bool DriverApi::m_isExist(std::string & fileName)
{
    // Обновить функционал API.
    m_getApiFiles();
    return m_apiFiles.find(fileName) != m_apiFiles.end();
}

//-----------------------------------------------------------------------------

std::vector<std::string> DriverApi::getApi()
{
    std::vector<std::string> api;

    // Обновить функционал API.
    m_getApiFiles();
    for (auto it = m_apiFiles.begin(); it != m_apiFiles.end(); it++)
        api.push_back(*it);

    return api;
}

//-----------------------------------------------------------------------------

std::string DriverApi::getPath(std::string & apiName)
{
    if (!m_isExist(apiName))
        return "";

    return m_apiPath + "/" + apiName;
}

//=============================================================================

} // namespace devtree
