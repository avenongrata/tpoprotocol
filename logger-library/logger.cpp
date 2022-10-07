#include "logger.h"
#include <filesystem>
#include <iostream>

#include "config-library/ciniparser.h"
#include "config-library/iiniparams.h"

//-----------------------------------------------------------------------------

namespace logger
{

//=============================================================================

LogTemplate::LogTemplate()
    : m_isConsole{false}
{
    m_configureP7();
}

//-----------------------------------------------------------------------------

bool LogTemplate::m_addModule(const char * moduleName,
                              IP7_Trace::hModule * module)
{
    return m_pTrace->Register_Module(TM(moduleName), module);
}

//-----------------------------------------------------------------------------

bool LogTemplate::m_configureP7()
{
    cfg::ini::iniParams_t params = cfg::ini::parseConfig("/opt/control/conf",
                                               "logger.ini", "LOGGER");
    if (!params)
        return false;

    std::string p7Iface = "/P7.Sink=";
    std::string sink = params->get("sink");
    std::string ip = params->get("server_ip", "192.168.11.203");

    p7Iface += sink;

    if (sink == "Console")
        m_isConsole = true;
    else if (sink == "Baical")
        p7Iface = p7Iface + " /P7.Addr=" + ip + " /P7.Pool=4096";

    m_pClient = P7_Create_Client(TM(p7Iface.c_str()));
    if (!m_pClient)
    {
        std::cout << "Can't create P7 Client object\n";
        exit(EXIT_FAILURE);
    }

    m_pTrace = P7_Create_Trace(m_pClient, TM("Trace channel"));
    if (!m_pTrace)
    {
        std::cout << "Can't create P7 Trace object\n";
        exit(EXIT_FAILURE);
    }

    bool ret;
    ret = m_pTrace->Register_Thread(TM("Application"), 0);
    if (!ret)
    {
        std::cout << "Can't register thread for P7 Trace object\n";
        exit(EXIT_FAILURE);
    }

    return true;
}

//-----------------------------------------------------------------------------

IP7_Trace::hModule LogTemplate::isExist(const char * moduleName)
{
    if (m_modules.find(moduleName) != m_modules.end())
        return m_modules[moduleName];
    else
        return nullptr;
}

//-----------------------------------------------------------------------------

bool LogTemplate::registerModule(const char * moduleName,
                                 IP7_Trace::hModule * module)
{
    if (!m_addModule(moduleName, module))
    {
        std::cout << "Can't register module " << moduleName << " for trace"
                  << std::endl;
        return false;
    }

    m_modules[moduleName] = *module;
    return true;
}

//-----------------------------------------------------------------------------

void LogTemplate::trace(IP7_Trace::hModule module, const char * file,
                   enum eP7Trace_Level level, tUINT16 line, const char * fun,
                   std::string msg)
{
    std::string color;

    if (m_isConsole)
    {
        if (level == EP7TRACE_LEVEL_DEBUG)
            color = "\033[96m";
        if (level == EP7TRACE_LEVEL_INFO)
            color = "\033[92m";
        if (level == EP7TRACE_LEVEL_WARNING)
            color = "\033[93m";
        if (level == EP7TRACE_LEVEL_ERROR || level == EP7TRACE_LEVEL_CRITICAL)
            color = "\033[91m";
    }
    msg = color + msg;
    if (m_isConsole)
        msg += "\033[0m";

    m_pTrace->Trace(0, level, module, line, file, fun,
                    TM("%s"), msg.c_str());
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

Logger::Logger()
    : log{LogTemplate::GetInstance()}, m_p7Module{NULL}
{}

//-----------------------------------------------------------------------------

bool Logger::isExist(const char * moduleName)
{
    IP7_Trace::hModule hModule = log->isExist(moduleName);
    if (hModule == nullptr)
        return false;

    /*
     * Когда в одном файле используется несколько классов, то каждый класс
     * имеет свой собственный id для логгера p7. Идентификация внутри логгера
     * привязыввется к названию файла, а перед каждым вызовом макроса
     * происходит проверка на регистрацию модуля. Из-за такого подхода название
     * файла связывается с тем из 2(и более) классов в одном файле, из которого
     * в первый раз вызывался макрос, соответсвенно теряется идентификация
     * для остальных классов.
     *
     * Этот хак позволяет избежать данной проблемы. Если макрос вызывался из
     * класса1, то при вызове из класса2 произойдет присваивание переменных
     * id, по которой идентифицирует логгер p7.
     *
     * Даже если объект класса был удален, то идентификация все равно
     * не исчезает, так-как в логгере хранится словарь, который связывает
     * имена файлов и id.
     */
    m_p7Module = hModule;
    return true;
}

//-----------------------------------------------------------------------------

bool Logger::registerModule(const char * moduleName)
{
    return log->registerModule(moduleName, &m_p7Module);
}

//-----------------------------------------------------------------------------

void Logger::trace(const char * file, eP7Trace_Level level, tUINT16 line,
                   const char * fun, std::string msg)
{
    m_p7Module = log->isExist(file);
    if (!m_p7Module)
        log->registerModule(file, &m_p7Module);

    log->trace(m_p7Module, file, level, line, fun, msg);
}

//=============================================================================

} // namespace logger
