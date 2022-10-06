#ifndef PROTOCOL_H
#define PROTOCOL_H

//-----------------------------------------------------------------------------

#include <vector>
#include <unordered_set>
#include <map>

#include "global-module/cbytearray.h"
#include "udpserver.h"
#include "statistic.h"
#include "devtree.h"
#include "logger-library/logger.h"

//-----------------------------------------------------------------------------

namespace tpoprotocol
{

//=============================================================================

class TpoProtocol
{
public:

    TpoProtocol();
    ~TpoProtocol() {};

    //-------------------------------------------------------------------------

    // Базовая инициализация и запуск сервера.
    void init();

    //-------------------------------------------------------------------------

    // Отправить пакет со статистикой.
    void sendStatistic(std::stringstream & data);
    // Установить указатель на внутреннюю переменную-класс, отвечающую за статистику.
    void setPointerToStatistic(statistic::Statistic * stat);
    // Ожидание пакета с командой от пользователя.
    bool waitForCommand();

private:

    // Класс логирования.
    logger::Logger log;

    // Возможные команды от клиента.
    typedef std::unordered_set<std::string> commands_t;
    commands_t command =
    {
        "keep-alive",       // Для поддержания Keep-Alive.
        "get",              // Получить статистику устройства/устройств.
        "del",              // Удалить устройство/устройства из пула сбора статистики.
        "set",              // Записать значение по адресу.
        "dtb",              // Получить дерево устройств.
        "stop"              // Остановить сбор статистики для всех устройств и файлов.
    };

    //-------------------------------------------------------------------------

    // Ответные статусы для клиента.
    typedef std::map<std::string, std::string> status_t;
    status_t status =
    {
        {"BAD_REQUEST", "BAD_REQUEST,"},   // Заголовок для обозначения неправильного запроса.
        {"SUCCESS", "SUCCESS,"},           // Заголовок для обозначения выполнения команды.
        {"ACTIVE", "ACTIVE,"},             // Заголовок для списка активных устройств/API.
        {"DEVICE_TREE", "DEVICE_TREE,"},   // Заголовок для дерева устройств.
        {"NOT_EXIST", "NOT_EXIST,"},       // Заголовок для обозначения несуществующего устройства.
        {"NOT_ACTIVE", "NOT_ACTIVE,"},     // Заголовок для обозначения неактивного устройства.
        {"ERROR", "ERROR,"},               // Заголовок для обозначения ошибки (не удалось выполнить какую-то команду)
        {"GET", "GET,"},                   // Заголовок для обозначения пакетов статистики.
        {"STOPPED", "STOPPED,"},           // Заголовок для обозначения остановки сбора статистики.
        {"DELETED", "DELETED,"}            // Заголовок для обозначения удаления из пула устройства/API.
    };

    //-------------------------------------------------------------------------

    // Структура для хранения ифнормации о запросе клиента на выполнение какой-либо работы.
    typedef struct jobData
    {
        bool device;            // Если true, то это устройство.

        uint32_t addr;          // Содержит адрес устройства.
        unsigned int regCnt;    // Количество регистров для считывания.
        uint32_t regVal;        // Значение для записи в регистр.

        std::string dtbDev;     // Для именя IP-Core в DTB.
        std::string apiName;    // Имя файла API.
        std::string apiVal;     // Значение для записи в API.

        timers::hz_t hz;        // Частота обновления.
    } jobData_t;

    //-------------------------------------------------------------------------

    // Входящее сообщение, полученное из пакета.
    std::string m_msg;
    // Команда, полученная из пакета.
    std::string m_cmd;
    // Тело сообщения (без команды), полученное из пакета.
    std::string m_body;

    //-------------------------------------------------------------------------

    // UDP сервер.
    udpserver::UdpServer m_udp;
    // Буфер для входящего пакета.
    global::CByteArray m_pkg;
    // Размер входящего пакета.
    const int m_pkgSize = 8192;
    // Ответный пакет.
    std::string m_response;

    //-------------------------------------------------------------------------

    // Для взаимодействия с классом сбора статистики.
    statistic::Statistic * m_statistic;
    // Для взаимодействия с деревом устройств.
    devtree::DevTree m_devTree;

    //-------------------------------------------------------------------------

    // Получить из пакета необходимые данные.
    void m_parsePkg();
    // Проверить существует ли команда.
    bool m_isCmdExist();
    // Разбить сообщение на команду и тело сообщения.
    void m_splitMsg();
    // Обработать команду.
    void m_parseCmd();

    //-------------------------------------------------------------------------

    // Обработать команду KEEP-ALIVE.
    void m_handleKA();
    // Обработать команду GET.
    void m_handleGet();
    // Обработать команду DEL.
    void m_handleDel();
    // Обработать команду SET.
    void m_handleSet();
    // Обработать команду DTB.
    void m_handleDtb();

    //-------------------------------------------------------------------------

    // Получить информацию из запроса GET об устройствах/API.
    jobData_t m_getJob(std::string & base, std::string & hz);
    // Получить информацию из запроса DEL об устройствах/API.
    jobData_t m_delJob(std::string & base);
    // Получить информацию из запроса SET об устройствах/API.
    jobData_t m_setJob(std::string & base, std::string & value);
    // Получить информацию из запроса DTB об устройстве.
    jobData_t m_dtb(std::string & base);

    //-------------------------------------------------------------------------

    // Проверить правильность запроса GET.
    bool m_isGetRequestCorrect(std::vector<std::string> & request);
    // Проверить существует ли устройство в дереве устройств.
    bool m_checkDev(dev::devInfo_t & dev);

    //-------------------------------------------------------------------------

    // Получить частоту обновления для чтения устройств/API.
    timers::hz_t m_getUpdateHz(std::string & data);
    // Получить количество регистров для чтения.
    unsigned int m_getRegCnt(std::string & data);
    // Получить значение для записи в устройство.
    uint32_t m_getDevValue(std::string & data);
    // Получить адрес устройства.
    uint32_t m_getDevAddr(std::string & data);

    //-------------------------------------------------------------------------

    // Отправить список устройств из дерева устройств.
    void m_sendDtb(std::stringstream & data);
    // Отправить подтверждение выполнения команды
    void m_sendSuccess(jobData & data);
    // Отправить команду, что устройство не существует.
    void m_sendNotExist(jobData & data);
    // Отправить команду, что устройство неактивно.
    void m_sendNotActive(jobData & data);
    // Отправить пакет, что устройство/API удалены.
    void m_sendDeleted(jobData & data);
    // Отправить команду, что не удалось выполнить какую-либо команду.
    void m_sendError(jobData & data);
    // Отправить ответный пакет на неправильную команду.
    void m_sendBadCmd();
    // Отправить список активных устройств и API.
    void m_sendActive();
    // Отправить сформированный пакет обратно пользователю.
    void m_sendResponse();
    // Отправить сырой пакет пользователю.
    void m_sendPkg(char * msg);

    //-------------------------------------------------------------------------

    // Для добавления устройства в пул.
    void m_addDev(jobData_t & data);
    // Для добавления файла API в пул.
    void m_addFile(jobData_t & data);
    // Для удаления устройства из пула.
    void m_delDev(jobData_t & data);
    // Для удаления файла API из пула.
    void m_delFile(jobData_t & data);
    // Для записи значения в устройство.
    void m_setDev(jobData_t & data);
    // Для записи значения в файл API.
    void m_setFile(jobData_t & data);
    // Получить функционал API для устройства.
    void m_getDtbApi(jobData_t & devInfo);
    // Записать значение по адресу.
    bool m_writeDevVal(dev::devInfo_t & devInfo, uint32_t & val);
    // Записать значение в файл API.
    bool m_writeApiVal(dev::file_t & fileInfo, std::string & val);
    // Прочитать устройство один раз.
    bool m_readDevOnce(dev::devInfo_t & devInfo);
    // Прочитать файл API один раз.
    bool m_readApiOnce(dev::file_t & fileInfo);
    // Сформировать список активных устройств/API.
    void m_getActive();
    // Получить список активных устройств.
    void m_getActiveDevs();
    // Получить список активных файлов API.
    void m_getActiveFiles();
    // Остановить сбор статистики и отправить пакет об остановленных устройствах/API.
    void m_stopStatistic();

    //-------------------------------------------------------------------------

    // Подготовить данные об устройстве/API для отправки.
    std::string m_preparePkgData(jobData_t & data);
    // Очистка внутренних буферов.
    void m_clearPkg();
};

//=============================================================================

} // namespace tpoprotocol

#endif // PROTOCOL_H
