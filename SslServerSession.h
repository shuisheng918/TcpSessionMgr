#pragma once

#ifdef ENABLE_SSL

#include <openssl/ssl.h>
#include "TcpSession.h"

class SslServerSession : public TcpSession
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
    SslServerSession() : m_pSSLCtx(NULL), m_pSSL(NULL), m_state(INIT)
    {
    }
    void Init(SSL_CTX *pSslCtx);
    virtual ~SslServerSession();
    
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