#pragma once

#ifdef ENABLE_SSL

#include <openssl/ssl.h>
#include "tcp_session.h"

class SslSession : public TcpSession
{
public:
    enum ESslServerSessionState
    {
        INIT,
        HANDSHAKING,
        ESTABLESED,
        CLOSE
    };
public:
    SslSession() : m_pSSLCtx(NULL), m_pSSL(NULL), m_state(INIT)
    {
    }
    void Init(SSL_CTX *pSslCtx, bool bServer);
    virtual ~SslSession();
    
    virtual void OnRead();
    virtual void OnWrite();
    virtual void OnRecvData(const char *data, int len);
    virtual void SendData(const char *data, int len);
    virtual int  SendPendingData();
private:
    SSL_CTX  * m_pSSLCtx;
    SSL      * m_pSSL;
    ESslServerSessionState  m_state;
};

#endif