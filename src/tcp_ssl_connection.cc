#include "tcp_ssl_connection.h"

#include "logger.h"
#include "progress_bar.h"
#include "net_utils.h"
#include "config.h"

#include <cstring>
#include <stdexcept>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

#include <openssl/x509v3.h>

void TCPSSLConnection::init_ssl(const std::string& host)
{
    SSL_load_error_strings();
    SSL_library_init();

    m_ssl_context.context_new(SSLv23_client_method());
    if (!Config::instance()->use_sslv2())
        m_ssl_context.set_options(SSL_OP_NO_SSLv2);
    if (!Config::instance()->use_sslv3())
        m_ssl_context.set_options(SSL_OP_NO_SSLv3);

    m_ssl_context.set_cipher_list("HIGH:MEDIUM:!RC4:!SRP:!PSK:!MD5:!aNULL@STRENGTH");
    m_ssl_context.set_default_verify_paths();

    m_ssl_handle.ssl_new(m_ssl_context);
    m_ssl_handle.set_fd(m_sock);

    // verify certificate
    if (Config::instance()->verify_peer()) {
        X509_VERIFY_PARAM *param;
        param = SSL_get0_param(m_ssl_handle.handle());

        X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
        X509_VERIFY_PARAM_set1_host(param, host.c_str(), 0);
        m_ssl_handle.set_verify(SSL_VERIFY_PEER, nullptr);
    }
    m_ssl_handle.connect();
}

void TCPSSLConnection::connect(const std::string& host, int port)
{
    connect(host, std::to_string(port));
}

void TCPSSLConnection::connect(const std::string& host, const std::string& service)
{
    close();
    m_sock = NetUtils::tcp_connect(host.c_str(), service.c_str());
    init_ssl(host);
    m_connected = true;
}

void TCPSSLConnection::write(const std::string& toWrite) const
{
    int written = 0;
    auto len = toWrite.size();
    auto start = toWrite.c_str();

    if (!m_connected)
        EXCEPTION("Not connected!");

    while (static_cast<decltype(len)>(written) < len) {
        int tmp = SSL_write(m_ssl_handle.handle(), start + written, len - written);
        if (tmp <= 0)
            EXCEPTION("SSL_write() failed: " << strerror(errno));
        written += tmp;
    }
}

std::string TCPSSLConnection::read(std::size_t numBytes) const
{
    std::string result;
    char buffer[BUFFER_SIZE];
    int read = 0;

    if (!m_connected)
        EXCEPTION("Not connected!");

    while (static_cast<decltype(numBytes)>(read) < numBytes) {
        int tmp = SSL_read(m_ssl_handle.handle(), buffer, sizeof(buffer));
        if (tmp <= 0)
            EXCEPTION("SSL_read() failed");
        result.insert(read, buffer, tmp);
        read += tmp;
    }

    return result;
}

std::string TCPSSLConnection::read_until_eof(std::size_t fileSize) const
{
    std::string result;
    char buffer[BUFFER_SIZE];
    int read = 0;

    if (!m_connected)
        EXCEPTION("Not connected!");

    if (fileSize > 0)
        result.reserve(fileSize);
    while (42) {
        int tmp = SSL_read(m_ssl_handle.handle(), buffer, sizeof(buffer));
        if (tmp < 0)
            EXCEPTION("SSL_read() failed.");
        if (tmp == 0)
            break;
        result.insert(read, buffer, tmp);
        read += tmp;
    }

    return result;
}

std::string TCPSSLConnection::read_until_eof_with_pg(std::size_t fileSize) const
{
    ProgressBar pg(fileSize);
    std::string result;
    char buffer[BUFFER_SIZE];
    int read = 0;

    if (!m_connected)
        EXCEPTION("Not connected!");

    if (fileSize > 0)
        result.reserve(fileSize);
    while (42) {
        int tmp = SSL_read(m_ssl_handle.handle(), buffer, sizeof(buffer));
        if (tmp == -1)
            EXCEPTION("SSL_read() failed.");
        if (tmp == 0)
            break;
        result.insert(read, buffer, tmp);
        read += tmp;
        pg.update(tmp);
    }

    return result;
}

void TCPSSLConnection::read_until_eof_to_fstream(std::ofstream& ofs) const
{
    char buffer[BUFFER_SIZE];

    if (!m_connected)
        EXCEPTION("Not connected!");

    while (42) {
        int tmp = SSL_read(m_ssl_handle.handle(), buffer, sizeof(buffer));
        if (tmp < 0)
            EXCEPTION("SSL_read() failed.");
        if (tmp == 0)
            break;
        ofs.write(buffer, tmp);
    }
}

void TCPSSLConnection::read_until_eof_with_pg_to_fstream(std::ofstream& ofs, std::size_t fileSize) const
{
    ProgressBar pg(fileSize);
    char buffer[BUFFER_SIZE];

    if (!m_connected)
        EXCEPTION("Not connected!");

    while (42) {
        int tmp = SSL_read(m_ssl_handle.handle(), buffer, sizeof(buffer));
        if (tmp < 0)
            EXCEPTION("SSL_read() failed.");
        if (tmp == 0)
            break;
        ofs.write(buffer, tmp);
        pg.update(tmp);
    }
}

std::string TCPSSLConnection::read_ln() const
{
    std::string result;
    char buffer[1];
    int read = 0;

    if (!m_connected)
        EXCEPTION("Not connected!");

    // FIXME: This is kind of slow
    while (42) {
        int tmp = SSL_read(m_ssl_handle.handle(), buffer, 1);
        if (tmp == -1)
            EXCEPTION("SSL_read() failed.");
        result.insert(read, buffer, tmp);
        read += tmp;

        if (buffer[0] == '\n')
            break;
    }

    return result;
}