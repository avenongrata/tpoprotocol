#include <algorithm>
#include <cstring>
#include <iostream>
#include <sys/wait.h>
#include <algorithm>

#include "protocol.h"
#include "common.h"

namespace tpoprotocol
{

//=============================================================================

TpoProtocol::TpoProtocol()
{
    init();
}

//-----------------------------------------------------------------------------

void TpoProtocol::init()
{
    m_pkg.set("", m_pkgSize);
    // Создать сервер.
    m_udp.start();
}

//-----------------------------------------------------------------------------

void TpoProtocol::sendStatistic(std::stringstream & data)
{
    auto header = status["GET"];
    auto pkg = header + data.str();
    m_sendPkg(const_cast<char*>(pkg.c_str()));
}

//-----------------------------------------------------------------------------

void TpoProtocol::setPointerToStatistic(statistic::Statistic * stat)
{
    m_statistic = stat;
}

//-----------------------------------------------------------------------------

bool TpoProtocol::waitForCommand()
{
    auto data = m_pkg.data();
    auto size = m_pkg.size();

    auto ret = m_udp.recvData(data, size);
    // Вернулся из-за Keep-Alive.
    if (ret == -1)
        return false;

    // Преобразовать пакет в сообщение.
    m_msg = std::string(m_pkg.begin(), m_pkg.end());
    // Урезать сообщение до полученного размера.
    m_msg.resize(size);
    m_parsePkg();

    return true;
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_parsePkg()
{
    // Удалить все пробелы из сообщения.
    m_msg.erase(remove(m_msg.begin(), m_msg.end(), ' '), m_msg.end());

    std::string dMsg = "Request: " + m_msg;
    LOGGER_INFO(dMsg);

    // Разбить сообщения не команду и тело.
    m_splitMsg();
    // Проверить существует ли команда.
    if (!m_isCmdExist())
    {
        m_sendBadCmd();
        return;
    }

    // Обработать команду.
    m_parseCmd();
    // Очистить внутренние буферы.
    m_clearPkg();
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_handleGet()
{
    // Если запрос GET без параметров - отправить список активных устройств/API.
    if (!m_body.size())
    {
        m_sendActive();
        return;
    }

    auto data = splitString(m_body, sep::dataSep);
    // Проверить правильность команды GET.
    if (!m_isGetRequestCorrect(data))
    {
        m_sendBadCmd();
        return;
    }

    jobData_t tmp;
    for (unsigned long i = 0; i < data.size(); i+=2)
    {
        tmp = m_getJob(data[i], data[i+1]);
        if (tmp.device) // Добавить устройство.
            m_addDev(tmp);
        else            // Добавить файл API.
            m_addFile(tmp);
    }
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_handleDel()
{
    auto data = splitString(m_body, sep::dataSep);

    jobData_t tmp;
    for (auto it = data.begin(); it != data.end(); it++)
    {
        tmp = m_delJob(*it);
        if (tmp.device) // Удалить устройство.
            m_delDev(tmp);
        else            // Удалить файл API.
            m_delFile(tmp);
    }
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_handleSet()
{
    auto data = splitString(m_body, sep::dataSep);

    jobData_t tmp;
    for (unsigned long i = 0; i < data.size(); i+=2)
    {
        tmp = m_setJob(data[i], data[i+1]);
        if (tmp.device)     // Записать в устройство.
            m_setDev(tmp);
        else
            m_setFile(tmp); // Записать в файл API.
    }
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_handleDtb()
{
    // Если запрашивается дерево устройств целиком.
    if (!m_body.size())
    {
        std::stringstream pkg;
        m_devTree.getDtb(pkg);
        m_sendDtb(pkg);
        return;
    }

    auto data = splitString(m_body, sep::dataSep);
    jobData_t tmp;
    for (unsigned long i = 0; i < data.size(); i++)
    {
        tmp = m_dtb(data[i]);
        m_getDtbApi(tmp);
    }
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_handleKA()
{
    LOGGER_INFO("Keep-Alive");
    //m_sendSuccess();
}

//-----------------------------------------------------------------------------

bool TpoProtocol::m_isCmdExist()
{
    // Перевести в нижний регистр.
    std::transform(m_cmd.begin(), m_cmd.end(), m_cmd.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    return command.find(m_cmd) != command.end();
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_parseCmd()
{
    // Обработать пакет с Keep-Alive.
    if (m_cmd == "keep-alive")
    {
        m_handleKA();
    }
    // Добавить устройство(a)/API в пул сбора статистики.
    if (m_cmd == "get")
    {
        m_handleGet();
    }
    // Удалить устройство(a)/API из пула сбора статистики.
    if (m_cmd == "del")
    {
        m_handleDel();
    }
    // Записать значение в устройство(a)/API.
    if (m_cmd == "set")
    {
        m_handleSet();
    }
    // Получить устройства в дереве устройств.
    if (m_cmd == "dtb")
    {
        m_handleDtb();
    }
    //  Остановить сбор любой статистики
    if (m_cmd == "stop")
    {
        m_stopStatistic();
    }
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_sendBadCmd()
{
    m_response = status["BAD_REQUEST"];
    m_response += m_msg;
    m_sendResponse();

    LOGGER_ERROR(m_response);
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_sendActive()
{
    m_response = status["ACTIVE"];
    m_getActive();
    m_sendResponse();
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_sendResponse()
{
    auto msgLen = m_response.length();
    auto msg = (byte_t *)m_response.c_str();
    m_udp.sendData(msg, msgLen);
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_sendSuccess(jobData & data)
{
    m_response = status["SUCCESS"];
    m_response += m_preparePkgData(data);
    m_sendResponse();
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_stopStatistic()
{
    m_response = status["STOPPED"];
    m_getActive();
    m_statistic->stopStatistic();
    m_sendResponse();
}

//-----------------------------------------------------------------------------

bool TpoProtocol::m_writeDevVal(dev::devInfo_t & devInfo, uint32_t & val)
{
    dev::Device dev {devInfo};
    return dev.write(val);
}

//-----------------------------------------------------------------------------

bool TpoProtocol::m_writeApiVal(dev::file_t & fileInfo, std::string & val)
{
    dev::DevApi api {fileInfo.first};
    return api.write(val);
}

//-----------------------------------------------------------------------------

bool TpoProtocol::m_readDevOnce(dev::devInfo_t & devInfo)
{
    std::stringstream msg;
    if (!m_statistic->readDevOnce(devInfo, msg))
        return false;

    sendStatistic(msg);
    return true;
}

//-----------------------------------------------------------------------------

bool TpoProtocol::m_readApiOnce(dev::file_t & fileInfo)
{
    std::stringstream msg;
    if (!m_statistic->readApiOnce(fileInfo, msg))
        return false;

    sendStatistic(msg);
    return true;
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_addDev(jobData_t & data)
{
    // Информация об устройстве.
    dev::devInfo_t devInfo {data.addr, data.regCnt};

    // Проверить наличие устройства в дереве устройств.
    if (!m_checkDev(devInfo))
    {
        m_sendNotExist(data);
        return;
    }

    // Считать устройство один раз, если не указана частота считывания.
    if (!data.hz)
    {
        if (!m_readDevOnce(devInfo))
            m_sendError(data);
    }
    else    // Если пользователь хочет добавить устройство в пул.
    {
        // Добавить устройство к пулу сбора статистики.
        if (!m_statistic->addDev(devInfo, data.hz))
            m_sendError(data);
    }
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_addFile(jobData_t & data)
{
    auto apiName = std::string(data.dtbDev) + "/" + data.apiName;
    auto fullPath = m_devTree.getApiPath(apiName);
    dev::file_t fileInfo {fullPath, apiName};

    // Проверить наличие файла API в дереве устройств.
    if (!fullPath.size())
    {
        m_sendNotExist(data);
        return;
    }

    // Считать файл API один раз, если не указана частота считывания.
    if (!data.hz)
    {
        if (!m_readApiOnce(fileInfo))
            m_sendError(data);
    }
    else    // Если пользователь хочет добавить файл API в пул.
    {
        // Добавить файл API к пулу сбора статистики.
        if (!m_statistic->addFile(fileInfo, data.hz))
            m_sendError(data);
    }
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_delDev(jobData_t & data)
{
    // Удалить устройство из пула.
    if (!m_statistic->delDev(data.addr))
        m_sendNotActive(data);
    else
        m_sendDeleted(data);
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_delFile(jobData_t & data)
{
    auto apiName = std::string(data.dtbDev) + "/" + data.apiName;
    auto fullPath = m_devTree.getApiPath(apiName);
    dev::file_t fileInfo {fullPath, apiName};

    // Удалить файл из пула.
    if (!m_statistic->delFile(fileInfo))
        m_sendNotActive(data);
    else
        m_sendDeleted(data);
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_setDev(jobData_t & data)
{
    // Запись производится в один регистр.
    dev::devInfo_t devInfo {data.addr, 1};

    // Проверить наличие устройства в дереве устройств (можно убрать проверку).
    if (!m_checkDev(devInfo))
    {
        m_sendNotExist(data);
        return;
    }

    // Записать значения по адресу.
    if (!m_writeDevVal(devInfo, data.regVal))
        m_sendError(data);
    else
        m_sendSuccess(data);
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_setFile(jobData_t & data)
{
    auto apiName = std::string(data.dtbDev) + "/" + data.apiName;
    auto fullPath = m_devTree.getApiPath(apiName);
    dev::file_t fileInfo {fullPath, apiName};

    // Проверить наличие файла API в дереве устройств.
    if (!fullPath.size())
    {
        m_sendNotExist(data);
        return;
    }

    if (!m_writeApiVal(fileInfo, data.apiVal))
        m_sendError(data);
    else
        m_sendSuccess(data);
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_getActive()
{
    m_response += "Devs: ";
    m_getActiveDevs();
    m_response += " Files: ";
    m_getActiveFiles();
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_getActiveDevs()
{
    std::stringstream data;

    m_statistic->getActiveDevs(data);
    if (data.str().size() == 0)
    {
        m_response += "NULL";
        return;
    }

    m_response += data.str();
    return;
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_getActiveFiles()
{
    std::stringstream data;

    m_statistic->getActiveFiles(data);
    if (data.str().size() == 0)
    {
        m_response += "NULL";
        return;
    }

    m_response += data.str();
    return;
}

//-----------------------------------------------------------------------------

bool TpoProtocol::m_checkDev(dev::devInfo_t & dev)
{
    return m_devTree.checkDev(dev);
}

//-----------------------------------------------------------------------------

timers::hz_t TpoProtocol::m_getUpdateHz(std::string & data)
{
    return std::stol(data);
}

//-----------------------------------------------------------------------------

unsigned int TpoProtocol::m_getRegCnt(std::string & data)
{
    return std::stol(data);
}

//-----------------------------------------------------------------------------

uint32_t TpoProtocol::m_getDevValue(std::string & data)
{
    std::string prefix = std::string (data.begin(), data.begin() + 2);
    if (prefix == "0x")
        return strtoul(data.c_str(), NULL, 16);
    else
        return std::stol(data);
}

//-----------------------------------------------------------------------------

uint32_t TpoProtocol::m_getDevAddr(std::string & data)
{
    return strtoul(data.c_str(), NULL, 16);
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_sendDtb(std::stringstream & data)
{
    m_response = status["DEVICE_TREE"];
    m_response += data.str();
    m_sendResponse();
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_sendNotExist(jobData & data)
{
    m_response = status["NOT_EXIST"];
    m_response += m_preparePkgData(data);
    m_sendResponse();

    LOGGER_ERROR(m_response);
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_sendNotActive(jobData & data)
{
    m_response = status["NOT_ACTIVE"];
    m_response += m_preparePkgData(data);
    m_sendResponse();

    LOGGER_ERROR(m_response);
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_sendDeleted(jobData & data)
{
    m_response = status["DELETED"];
    m_response += m_preparePkgData(data);
    m_sendResponse();

    LOGGER_INFO(m_response);
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_sendError(jobData & data)
{
    m_response = status["ERROR"];
    m_response += m_preparePkgData(data);;
    m_sendResponse();

    LOGGER_ERROR(m_response);
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_getDtbApi(jobData_t & devInfo)
{
    std::stringstream data;

    if (!m_devTree.getApi(devInfo.dtbDev, data))
        m_sendNotExist(devInfo);
    else
        m_sendDtb(data);
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_clearPkg()
{
    // Команда, полученная из пакета.
    m_cmd.clear();
    // Тело сообщения (без команды), полученное из пакета.
    m_body.clear();
    // Входящее сообщение, полученное из пакета.
    m_msg.clear();
}

//-----------------------------------------------------------------------------

std::string TpoProtocol::m_preparePkgData(jobData_t & data)
{
    std::stringstream pkg;

    if (data.device)
    {
        pkg << "0x" << std::setfill('0') << std::setw(8)
            << std::hex << data.addr;
    }
    else
    {
        pkg << data.dtbDev;
        if (data.apiName.size())
            pkg << "/" << data.apiName;
    }

    return pkg.str();
}

//-----------------------------------------------------------------------------

bool TpoProtocol::m_isGetRequestCorrect(std::vector<std::string> & request)
{
    auto len = request.size();

    // Нечетное количество - ошибка.
    if (len % 2)
        return false;

    // Проверить каждый запрос на правильность.
    for (unsigned long i = 0; i < len; i+=2)
    {
        // Устройство/API должны содержать этот символ.
        if (request[i].find(sep::baseSep) == std::string::npos)
            return false;

        // Проверить является ли cледующий параметр числом.
        request[i+1].erase(remove_if(request[i+1].begin(),
                           request[i+1].end(), isspace), request[i+1].end());
        if (!std::all_of(request[i+1].begin(), request[i+1].end(), ::isdigit))
            return false;
    }

    return true;
}

//-----------------------------------------------------------------------------

TpoProtocol::jobData_t TpoProtocol::m_getJob(std::string & base,
                                             std::string & hz)
{
    jobData_t jobData;

    auto data = splitString(base, sep::baseSep);
    // Работа с API.
    if (data[0].find('@') != std::string::npos)
    {
        jobData.device = false;
        jobData.dtbDev = data[0];
        jobData.apiName = data[1];
    }
    else    // Работа с устройствами.
    {
        jobData.device = true;
        jobData.addr = m_getDevAddr(data[0]);
        jobData.regCnt = m_getRegCnt(data[1]);
    }

    jobData.hz = m_getUpdateHz(hz);
    return jobData;
}

//-----------------------------------------------------------------------------

TpoProtocol::jobData_t TpoProtocol::m_delJob(std::string & base)
{
    jobData_t jobData;

    // Работа с API.
    if (base.find('@') != std::string::npos)
    {
        jobData.device = false;
        auto data = splitString(base, sep::baseSep);
        jobData.dtbDev = data[0];
        jobData.apiName = data[1];
    }
    else    // Работа с устройствами.
    {
        jobData.device = true;
        jobData.addr = m_getDevAddr(base);
    }

    return jobData;
}

//-----------------------------------------------------------------------------

TpoProtocol::jobData_t TpoProtocol::m_setJob(std::string & base,
                                             std::string & value)
{
    jobData_t jobData;

    auto data = splitString(base, sep::baseSep);
    // Работа с API.
    if (data[0].find('@') != std::string::npos)
    {
        jobData.device = false;
        jobData.dtbDev = data[0];
        jobData.apiName = data[1];
        jobData.apiVal = value;
    }
    else    // Работа с устройствами.
    {
        jobData.device = true;
        jobData.addr = m_getDevAddr(data[0]);
        jobData.regVal = m_getDevValue(value);
    }

    return jobData;
}

//-----------------------------------------------------------------------------

TpoProtocol::jobData_t TpoProtocol::m_dtb(std::string & base)
{
    jobData_t jobData
    {
        .device = false,
        .dtbDev = base
    };

    return jobData;
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_sendPkg(char * msg)
{
    // Поменять ?
    m_udp.sendData(reinterpret_cast<byte_t *>(msg), strlen(msg));
}

//-----------------------------------------------------------------------------

void TpoProtocol::m_splitMsg()
{    
    // Получить конец команды.
    auto cmdEnd = std::find(m_msg.begin(), m_msg.end(), sep::dataSep);
    // Не для всех команд нужен разделитель.
    if (cmdEnd == m_msg.end())
    {
        m_cmd = m_msg;
        return;
    }
    // Получить команду.
    m_cmd = std::string(m_msg.begin(), cmdEnd);
    // Получить тело сообщения.
    m_body = std::string(cmdEnd + 1, m_msg.end());
}

//=============================================================================

} // namespace tpoprotocol

