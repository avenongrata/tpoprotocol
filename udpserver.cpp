#include <arpa/inet.h>
#include <unistd.h>

#include "udpserver.h"
#include "netsock-library/csockaddr.h"
#include "config-library/iiniparams.h"
#include "config-library/ciniparser.h"

//-----------------------------------------------------------------------------

udpserver::UdpServer::UdpServer()
{
    // Стандартный ip адрес для приема.
    m_ip = "0.0.0.0";

    // Инициализация.
    m_init();
}

//-----------------------------------------------------------------------------

udpserver::UdpServer::~UdpServer()
{
    m_sockClose();

    m_recv  ->clear();
    m_send  ->clear();
    m_sender->clear();
}

//-----------------------------------------------------------------------------

void udpserver::UdpServer::m_init()
{
    // Конфигурация сервера.
    m_readConfig();
    // Структура адреса приёма.
    m_recv = std::make_unique<net::sock::CSockAddr>();
    // Структура адреса отправки.
    m_send = std::make_unique<net::sock::CSockAddr>();
    // Структура адреса отправителя.
    m_sender = std::make_unique<net::sock::CSockAddr>();
    // Установить данные для UDP сервера.
    m_setRecieverAddr();
    // Флаг установленного соединения.
    f_established = false;
}

//-----------------------------------------------------------------------------

bool udpserver::UdpServer::start()
{
    if (!m_sockCreate())
    {
        LOGGER_ERROR("Can't create socket");
        return false;
    }

    // Прослушка порта.
    if (!m_sockBind())
    {
        LOGGER_ERROR("Can't bind socket");
        return false;
    }

    std::stringstream msg;
    msg << "Server is listening on port: " << m_port;
    LOGGER_INFO(msg.str());

    return true;
}

//-----------------------------------------------------------------------------

int udpserver::UdpServer::recvData(byte_t * buf, size_t & size)
{
    // Если пакет укладывается в пределы.
    if (size <= s_udp)
    {
        auto ret = m_recvFrom(buf, size);
        // Keep-Alive вернулся по таймауту.
        if (ret == -1)
            return ret;

        size = size_t (ret);
        return (size > 0);
    }

    size = 0x00U;
    return (size > 0);
}

//-----------------------------------------------------------------------------

bool udpserver::UdpServer::sendData(const byte_t * buf, const size_t & size)
{
    auto res = m_sendTo(buf, size);
    return (res > 0);
}

//-----------------------------------------------------------------------------

bool udpserver::UdpServer::m_sockCreate()
{
    // Инициализация сокета.
    m_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_sock < 0)
    {
        LOGGER_ERROR("Read-socket initialization error");
        return false;
    }

    // Параметр переподключения при ошибке.
    if (!m_setReUse())
        return false;

    // Установить Keep-Alive для сокета.
    if (m_keepAlive.tv_sec > 0)
    {
        if (!m_setKA())
            return false;
    }
    else
    {
        LOGGER_INFO("Keep-Alive isn't installed");
    }

    return true;
}

//-----------------------------------------------------------------------------

void udpserver::UdpServer::m_sockClose()
{
    close(m_sock);
}

//-----------------------------------------------------------------------------

bool udpserver::UdpServer::m_sockBind()
{
    // Структура адреса.
    auto recv = m_recv->getBerkley();

    // Запуск прослушки.
    auto ret = bind(m_sock, recv.addr, recv.size);
    if (ret >= 0)
        return true;

    // Ошибка операции.
    LOGGER_ERROR("Socket listening error");
    return false;
}

//-----------------------------------------------------------------------------

void udpserver::UdpServer::m_setRecieverAddr()
{
    m_recv->set(net::sock::ISockAddr::type_t::INET, m_ip, m_port);
}

//-----------------------------------------------------------------------------

int udpserver::UdpServer::m_recvFrom(byte_t * buf, const size_t & size)
{
    if (buf == nullptr)
    {
        LOGGER_ERROR("Receive buffer is nullptr");
        return 0x00U;
    }

    // Структура адреса отправителя.
    auto sender = m_sender->getBerkley();

    // Запрос пакета.
    auto res = recvfrom(m_sock, buf, size, 0, sender.addr, &sender.size);
    if (res < 0)
    {
        m_sender->clear();
        // Keep-Alive вернулся по таймауту.
        if (errno == EAGAIN)
        {
            LOGGER_INFO("Keep-Alive timeout");
            return -1;
        }
        LOGGER_ERROR("There is some error in recvfrom function");
    }

    // Возврат количества прочтённого, при ошибке - 0.
    return (res > 0) ? uint16_t(res) : 0x00U;
}

//-----------------------------------------------------------------------------

uint16_t udpserver::UdpServer::m_sendTo(const byte_t * buf,
                                         const size_t & size)
{
    if (buf == nullptr)
    {
        LOGGER_ERROR("Send buffer is nullptr");
        return 0x00U;
    }

    // Структура адреса отправки.
    auto send = m_sender->getBerkley();

    // Отправка пакета.
    auto ret = sendto(m_sock, buf, size, 0, send.addr, send.size);
    return (ret < 0) ? 0x0000U : uint16_t(ret);
}

//-----------------------------------------------------------------------------

bool udpserver::UdpServer::m_setKA()
{
    // Установить Keep-Alive для сокета.
    auto ret = setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, &m_keepAlive,
                     sizeof(m_keepAlive));
    if (ret < 0)
    {
        LOGGER_ERROR("Read-socket use option (Keep-Alive) error");
        shutdown(m_sock, SHUT_RDWR);
        m_sockClose();
        return false;
    }
    std::stringstream msg;
    msg << "Keep-Alive is set to " << m_keepAlive.tv_sec << " seconds";
    LOGGER_INFO(msg.str());

    return true;
}

//-----------------------------------------------------------------------------

bool udpserver::UdpServer::m_setReUse()
{
    int opt = 1;

    // Параметр переподключения при ошибке.
    static constexpr auto mask = SO_REUSEADDR | SO_REUSEPORT;
    auto ret = setsockopt(m_sock, SOL_SOCKET, mask, &opt, sizeof(int));
    if (ret < 0)
    {
        LOGGER_ERROR("Read-socket use options error");
        shutdown(m_sock, SHUT_RDWR);
        m_sockClose();
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------

void udpserver::UdpServer::m_readConfig()
{
    auto params = cfg::ini::parseConfig("/opt/control/conf", "tpoprotocol.ini",
                                        "SERVER");
    if (!params)
    {
        LOGGER_ERROR("Can't find parameters for server configuration");
        exit(EXIT_FAILURE);
    }

    // Чтение значения Keep-Alive.
    m_keepAlive.tv_sec = params->getInt("keep-alive", -1);
    m_keepAlive.tv_usec = 0;

    // Чтение значения порта.
    m_port = params->getInt("port", 8889);
}

//-----------------------------------------------------------------------------
