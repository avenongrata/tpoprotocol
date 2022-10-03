#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <unordered_map>
#include <string_view>
#include <sstream>
#include <iomanip>

#include "p7/P7_Trace.h"

//=============================================================================

#define LOGGER_TRACE(msg) log.trace(__FILE__, EP7TRACE_LEVEL_TRACE, \
    (tUINT16)__LINE__, __FUNCTION__, msg)

#define LOGGER_DEBUG(msg) log.trace(__FILE__, EP7TRACE_LEVEL_DEBUG, \
    (tUINT16)__LINE__, __FUNCTION__, msg)

#define LOGGER_INFO(msg) log.trace(__FILE__, EP7TRACE_LEVEL_INFO, \
    (tUINT16)__LINE__, __FUNCTION__, msg)

#define LOGGER_WARNING(msg) log.trace(__FILE__, EP7TRACE_LEVEL_WARNING, \
    (tUINT16)__LINE__, __FUNCTION__, msg)

#define LOGGER_ERROR(msg) log.trace(__FILE__, EP7TRACE_LEVEL_ERROR, \
    (tUINT16)__LINE__, __FUNCTION__, msg)

#define LOGGER_CRITICAL(msg) log.trace(__FILE__, EP7TRACE_LEVEL_CRITICAL, \
    (tUINT16)__LINE__, __FUNCTION__, msg)

//-----------------------------------------------------------------------------

namespace logger
{

//=============================================================================

class LogTemplate
{
    /* Singleton for Logger. */

public:

    // Получить экземпляр класса (существует в единственном числе).
    static LogTemplate * GetInstance()
    {
        // Allocate with `new` in case Logger is not trivially destructible.
        static LogTemplate * logger = new LogTemplate();
        return logger;
    }

    //-------------------------------------------------------------------------

    // Проверить существует ли такой модуль для отслеживания.
    IP7_Trace::hModule isExist(const char * moduleName);
    // Зарегестрировать модуль для отслеживания.
    bool registerModule(const char * moduleName, IP7_Trace::hModule *);

    //-------------------------------------------------------------------------

    // Функция регистрации сообщений.
    void trace(IP7_Trace::hModule module, const char * file,
               enum eP7Trace_Level level, tUINT16 line, const char * fun,
               std::string msg);
private:

    bool m_isConsole;

    //-------------------------------------------------------------------------

    LogTemplate();

    // Delete copy constructor
    LogTemplate(const LogTemplate &) = delete;
    // Delete copy operator
    LogTemplate & operator=(const LogTemplate &) = delete;
    // Delete move constructor
    LogTemplate(LogTemplate &&) = delete;
    // Delete move operator
    LogTemplate & operator=(LogTemplate &&) = delete;

    //-------------------------------------------------------------------------

    // Добавить модуль в P7.
    bool m_addModule(const char * moduleName, IP7_Trace::hModule * module);
    // Конфигурирование логгера p7.
    bool m_configureP7();

    //-------------------------------------------------------------------------

    // P7 Client object.
    IP7_Client * m_pClient;
    // P7 Trace object.
    IP7_Trace * m_pTrace;

    //-------------------------------------------------------------------------

    // Имена модулей связаны с ID.
    std::unordered_map<const char *, IP7_Trace::hModule> m_modules;
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

class Logger
{
public:

    Logger();

    //-------------------------------------------------------------------------

    // Проверить существует ли такой модуль для отслеживания.
    bool isExist(const char * moduleName);
    // Зарегестрировать модуль для отслеживания.
    bool registerModule(const char * moduleName);

    //-------------------------------------------------------------------------

    // Функция регистрации сообщений.
    void trace(const char * file, enum eP7Trace_Level level, tUINT16 line,
               const char * fun, std::string msg);

private:

    LogTemplate * log;
    IP7_Trace::hModule m_p7Module;
};

//=============================================================================

} // namespace logger

#endif // LOGGER_H
