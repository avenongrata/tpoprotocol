#ifndef DEVAPI_H
#define DEVAPI_H

//-----------------------------------------------------------------------------

#include <filesystem>
#include <iostream>
#include <vector>
#include <mutex>
#include <cstdio>

#include "logger-library/logger.h"

//-----------------------------------------------------------------------------

namespace dev
{

//=============================================================================

// Полное имя файла API и его alias.
typedef std::pair<std::string, std::string> file_t;
// Вектор файлов.
typedef std::vector<file_t> files_t;

//-----------------------------------------------------------------------------

class DevApi
{
public:

    DevApi(std::string path);
    ~DevApi();

    //-------------------------------------------------------------------------

    // Прочитать файл.
    bool read(std::stringstream & data);
    // Записать значение.
    bool write(std::string & apiVal);

private:

    // Класс логирования.
    logger::Logger log;

    // Путь до файла.
    std::string m_path;
    // Разрешения файла.
    std::string m_perms;
    // Флаг открытия файла.
    bool m_opened;
    // Дескриптор файла устройства в виртуальной файловой системе.
    FILE * m_fd;
    // Максимальный размер чтения файла.
    static const size_t m_bufSize = 2048;
    // Для сохранения информации из файла.
    char m_buffer[m_bufSize];

    //-------------------------------------------------------------------------

    // Открыть файл виртуальной файловой системы.
    bool m_open();
    // Получить разрешения для файла.
    void m_getFilePerms();
};

//=============================================================================
//=============================================================================

class DevsApi
{
public:

    DevsApi(file_t & fileName);
    ~DevsApi();

    //-------------------------------------------------------------------------

    // Прочитать файлы устройства.
    bool read(std::stringstream & pkg);

    //-------------------------------------------------------------------------

    // Добавить файл к пулу чтения.
    bool add(file_t & fileName);
    // Удалить файлы API из пула чтения.
    bool remove(file_t & fileName);
    // Удалить все файлы API из пула чтения.
    void removeAll();
    // Получить список активных файлов.
    files_t getActive();

    //-------------------------------------------------------------------------

    // Проверить существует ли файл в пуле чтения.
    bool isExist(file_t & fileName);
    // Проверить есть ли активные файлы для чтения.
    bool isActive();

private:

    // Класс логирования.
    logger::Logger log;

    // Путь к файлу и указатель на класс файла устройства.
    typedef std::pair<file_t, DevApi *> api_t;
    // Вектор указателей на файлы устройств.
    typedef std::vector<api_t *> apis_t;

    //-------------------------------------------------------------------------

    // Вектор файлов.
    apis_t m_apis;
    // Мьютекс для работы с файлами.
    std::mutex m_apisMutex;

    //-------------------------------------------------------------------------

    // Проверить существует ли файл.
    bool m_isExist(file_t & fileName);

    //-------------------------------------------------------------------------

    // Добавить файл в пул чтения.
    void m_add(file_t & fileName);

    //-------------------------------------------------------------------------

    // Удалить память, выделенную под класс DevApi.
    void m_deleteFile(api_t * api);
    // Отчистить вектор файлов чтения и удалть выделенную память.
    void m_deleteFiles();
};

//=============================================================================

} // namepsace dev

#endif // DEVAPI_H
