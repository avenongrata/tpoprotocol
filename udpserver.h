#ifndef UDPSERVER_H
#define UDPSERVER_H

//-----------------------------------------------------------------------------

#include <iostream>
#include <string>
#include <memory>
#include <time.h>
#include <sys/time.h>

#include "global-module/types.h"
#include "netsock-library/isockaddr.h"
#include "logger-library/logger.h"

//-----------------------------------------------------------------------------

namespace udpserver
{

//=============================================================================

class UdpServer
{
public:

    UdpServer();
    ~UdpServer();

    //-------------------------------------------------------------------------

    // Запустить UDP server.
    bool start();
    // Остановить UDP server.
    void stop();

    //-------------------------------------------------------------------------

    // Чтение данных пакета.
    int recvData(byte_t * buf, size_t & size);
    // Отправка данных пакета.
    bool sendData(const byte_t * buf, const size_t & size);

private:

    // Класс логирования.
    logger::Logger log;

    // Максимальный размер пакета UDP.
    DEF_CONST size_t s_udp = 65535;
    // Keep-Alive в секундах.
    struct timeval m_keepAlive;

    //-------------------------------------------------------------------------

    // Структура адреса приёма.
    std::unique_ptr<net::sock::ISockAddr> m_recv;
    // Структура адреса отправки.
    std::unique_ptr<net::sock::ISockAddr> m_send;
    // Структура адреса отправителя.
    std::unique_ptr<net::sock::ISockAddr> m_sender;

    //-------------------------------------------------------------------------

    // Дескриптор сокета.
    int m_sock;
    // Порт на котором сервер будет принимать пакеты.
    int m_port;
    // IP-адрес для создания сервера.
    std::string m_ip;
    // Флаг установленного соединения.
    bool f_established;

    //-------------------------------------------------------------------------

    // Инициализация.
    void m_init();
    // Чтение конфигурационных данных из файла.
    void m_readConfig();

    //-------------------------------------------------------------------------

    // Создать сокет.
    bool m_sockCreate();
    // Закрыть сокет.
    void m_sockClose();
    // Прослушать порт.
    bool m_sockBind();
    // Установить адрес приема.
    void m_setRecieverAddr();

    //-------------------------------------------------------------------------

    // Прочитать из сокета.
    int m_recvFrom(byte_t * buf, const size_t & size);
    // Прочитать в сокет.
    uint16_t m_sendTo(const byte_t * buf, const size_t & size);

    //-------------------------------------------------------------------------

    // Установить Keep-Alive.
    bool m_setKA();
    // Установить параметр переподключения при ошибке.
    bool m_setReUse();
};

//=============================================================================

} // namespace udpserver

#endif // UDPSERVER_H
