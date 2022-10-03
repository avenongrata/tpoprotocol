#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "devapi.h"
#include "common.h"

//-----------------------------------------------------------------------------

namespace dev
{

//=============================================================================

DevApi::DevApi(std::string path)
    : m_path(path)
{
    // Получить разрешения файла.
    m_getFilePerms();
    m_opened = m_open();
}

//-----------------------------------------------------------------------------

DevApi::~DevApi()
{
    if (m_opened)
        fclose(m_fd);
}

//-----------------------------------------------------------------------------

bool DevApi::read(std::stringstream & data)
{
    ssize_t readBytes;

    // Когда файл не был открыт.
    if (!m_opened)
    {
        LOGGER_ERROR("File wasn't opened");
        return false;
    }

    // Проверить разрешения на чтение в файл.
    if (m_perms != "r+" && m_perms != "r")
    {
        LOGGER_ERROR("The file is opened without the possibility to read");
        return false;
    }

    // Прочитать данные из файла.
    readBytes = fread(m_buffer, 1, m_bufSize, m_fd);
    // Переместиться в начало файла.
    fseek(m_fd, 0, SEEK_SET);

    // Сохранить данные в пакет.
    for (ssize_t i = 0; i < readBytes - 1; i++)
        data << m_buffer[i];

    return true;
}

//-----------------------------------------------------------------------------

bool DevApi::write(std::string & apiVal)
{
    // Когда файл не был открыт.
    if (!m_opened)
    {
        LOGGER_ERROR("File wasn't opened");
        return false;
    }

    // Проверить разрешения на запись в файл.
    if (m_perms != "r+" && m_perms != "w")
    {
        LOGGER_ERROR("The file is opened without the possibility to write");
        return false;
    }

    // Записать данные в файл.
    fwrite(apiVal.c_str(), 1, apiVal.size(), m_fd);
    // Переместиться в начало файла.
    fseek(m_fd, 0, SEEK_SET);

    return true;
}

//-----------------------------------------------------------------------------

bool DevApi::m_open()
{
    std::stringstream msg;

    // Отслеживать ошибки отсутствия файла ?
    // Открыть символьное устройство на чтение/запись.
    m_fd = fopen(m_path.c_str(), m_perms.c_str());
    if (m_fd == NULL)
    {
        msg << "Error opening /dev/mem in \"" << m_perms << "\" mode ("
            << errno << ") : " << strerror(errno);
        LOGGER_ERROR(msg.str());
        return false;
    }
    msg << "Successfully opened /dev/mem in \"" << m_perms << "\" mode";
    LOGGER_INFO(msg.str());

    return true;
}

//-----------------------------------------------------------------------------

void DevApi::m_getFilePerms()
{
    namespace fs = std::filesystem;
    std::string tmpPerms;

    // Отслеживать ошибки отсутствия файла ?
    auto perms = fs::status(m_path.c_str()).permissions();
    if ((perms & fs::perms::owner_read) != fs::perms::none)
        tmpPerms += "r";
    if ((perms & fs::perms::owner_write) != fs::perms::none)
        tmpPerms += "w";

    m_perms = (tmpPerms == "rw") ? "r+" : tmpPerms;
}

//=============================================================================
//=============================================================================

DevsApi::DevsApi(file_t & fileName)
{
    m_add(fileName);
}

//-----------------------------------------------------------------------------

DevsApi::~DevsApi()
{
    m_deleteFiles();
}

//-----------------------------------------------------------------------------

bool DevsApi::read(std::stringstream & pkg)
{
    std::lock_guard<std::mutex> lock(m_apisMutex);
    for (auto it = m_apis.begin(); it != m_apis.end(); )
    {
        pkg << (*it)->first.second << sep::dataSep;
        if (!(*it)->second->read(pkg))
        {
            pkg.str(std::string());
            return false;
        }

        if (++it != m_apis.end())
            pkg << sep::dataSep;
    }

    return true;
}

//-----------------------------------------------------------------------------

bool DevsApi::add(file_t & fileName)
{
    std::lock_guard<std::mutex> lock(m_apisMutex);

    if (m_isExist(fileName))
        return false;

    m_add(fileName);
    return true;
}

//-----------------------------------------------------------------------------

bool DevsApi::remove(file_t & fileName)
{
    std::lock_guard<std::mutex> lock(m_apisMutex);
    for (auto it = m_apis.begin(); it != m_apis.end(); it++)
    {
        if (fileName == (*it)->first)
        {
            m_deleteFile(*it);
            m_apis.erase(it);
            return true;
        }
    }

    return false;
}

//-----------------------------------------------------------------------------

void DevsApi::removeAll()
{
    std::lock_guard<std::mutex> lock(m_apisMutex);

    for (auto it = m_apis.begin(); it != m_apis.end(); it++)
        m_deleteFile(*it);

    m_apis.resize(0);
}

//-----------------------------------------------------------------------------

files_t DevsApi::getActive()
{
    files_t data;

    std::lock_guard<std::mutex> lock(m_apisMutex);
    for (auto it = m_apis.begin(); it != m_apis.end(); it++)
        data.push_back((*it)->first);

    return data;
}

//-----------------------------------------------------------------------------

bool DevsApi::isExist(file_t & fileName)
{
    std::lock_guard<std::mutex> lock(m_apisMutex);
    return m_isExist(fileName);
}

//-----------------------------------------------------------------------------

bool DevsApi::isActive()
{
    std::lock_guard<std::mutex> lock(m_apisMutex);
    return bool(m_apis.size());
}

//-----------------------------------------------------------------------------

bool DevsApi::m_isExist(file_t & fileName)
{
    for (auto it = m_apis.begin(); it != m_apis.end(); it++)
    {
        if (fileName == (*it)->first)
            return true;
    }

    return false;
}

//-----------------------------------------------------------------------------

void DevsApi::m_add(file_t & fileName)
{
    api_t * api = new api_t;
    api->first = fileName;
    api->second = new DevApi(fileName.first);

    m_apis.push_back(api);
}

//-----------------------------------------------------------------------------

void DevsApi::m_deleteFile(api_t * api)
{
    delete api->second;
}

//-----------------------------------------------------------------------------

void DevsApi::m_deleteFiles()
{
    std::lock_guard<std::mutex> lock(m_apisMutex);
    auto apisCnt = m_apis.size();
    for (int i = (int) apisCnt - 1; i >= 0; i--)
    {
        m_deleteFile(m_apis[i]);
        m_apis.pop_back();
    }
}

//=============================================================================

} // namespace dev

