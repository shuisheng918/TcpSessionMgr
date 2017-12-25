#ifdef ENABLE_SSL

#include "SslServerSession.h"
#include <assert.h>
#include <openssl/err.h>
#include <string>
#include <sw_event.h>
#include "TcpSessionMgr.h"

SslServerSession::~SslServerSession()
{
    if (m_pSSL != NULL)
    {
        SSL_free(m_pSSL);
        m_pSSL = NULL;
    }
}

void SslServerSession::Init(SSL_CTX *pSslCtx)
{
    m_pSSLCtx = pSslCtx;
    m_pSSL = SSL_new(m_pSSLCtx);
    SSL_set_fd(m_pSSL, m_socket);
    SSL_set_accept_state(m_pSSL);
    SSL_set_mode(m_pSSL, SSL_MODE_ENABLE_PARTIAL_WRITE);
}

void SslServerSession::OnRead()
{
    int ret = 0;
    if (m_state == INIT)
    {
        m_state = HANDSHAKING;
    }
    if (m_state == HANDSHAKING)
    {
        ret = SSL_do_handshake(m_pSSL);
        if (ret == 1)
        {
            m_state = ESTABLESED;
        }
        else
        {
            int err = SSL_get_error(m_pSSL, ret);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
            {
                return;
            }
            else
            {
                SSL_shutdown(m_pSSL);
                m_state = CLOSE;
                Close();
            }
        }
    }
    if (m_state == ESTABLESED)
    {
        char buf[4096];
        while ((ret = SSL_read(m_pSSL, buf, sizeof(buf))) > 0)
        {
            OnRecvData(buf, ret);
        }
        int err = SSL_get_error(m_pSSL, ret);
        assert(err != SSL_ERROR_WANT_WRITE);
        if (err != SSL_ERROR_WANT_READ)
        {
            SSL_shutdown(m_pSSL);
            m_state = CLOSE;
            Close();
        }
    }
}

void SslServerSession::OnWrite()
{
    int ret = 0;
    if (m_state == INIT)
    {
        m_state = HANDSHAKING;
    }
    if (m_state == HANDSHAKING)
    {
        ret = SSL_do_handshake(m_pSSL);
        if (ret == 1)
        {
            m_state = ESTABLESED;
        }
        else
        {
            int err = SSL_get_error(m_pSSL, ret);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
            {
                return;
            }
            else
            {
                SSL_shutdown(m_pSSL);
                m_state = CLOSE;
                Close();
            }
        }
    }
    if (m_state == ESTABLESED)
    {
        SendPendingData();
    }

}

void SslServerSession::OnRecvData(const char *data, int len)
{
    // echo
    SendData(data, len);
}

void SslServerSession::SendData(const char *data, int len)
{
    if (data == NULL || len <= 0)
    {
        return;
    }
    int sended = 0;
    if (m_pSendBufHead == NULL)
    {
        int ret, err;
        while ((ret = SSL_write(m_pSSL, &data[sended], len - sended)) > 0)
        {
            sended += ret;
            if (sended == len) //ok
            {
                if (m_sentClose)
                {
                    SSL_shutdown(m_pSSL);
                    Close();
                }
                return;
            }
        }
        err = SSL_get_error(m_pSSL, ret);
        assert(err != SSL_ERROR_WANT_READ);
        if (ret < 0 && err == SSL_ERROR_WANT_WRITE)
        {
            goto topending;
        }
        else
        {
            SSL_shutdown(m_pSSL);
            m_state = CLOSE;
            Close();
            return;
        }
    }
    else
    {
topending:
        SendBuf *pSendBuf = new SendBuf;
        assert(len - sended > 0);
        pSendBuf->m_data = new char [len - sended];
        pSendBuf->m_len = len - sended;
        memcpy(pSendBuf->m_data, &data[sended], len - sended);
        pSendBuf->m_sendOff = 0;
        pSendBuf->m_next = NULL;
        if (m_pSendBufTail == NULL)
        {
            m_pSendBufHead = m_pSendBufTail = pSendBuf;
        }
        else
        {
            m_pSendBufTail->m_next = pSendBuf;
            m_pSendBufTail = pSendBuf;
        }
        if (-1 == sw_ev_io_add(m_pSessionMgr->GetEventCtx(), m_socket, SW_EV_WRITE, TcpSession::OnSessionIOReady, this))
        {
            printf("sw_ev_io_add failed. At %s:%d\n", basename(__FILE__), __LINE__);
        }
    }
}


/** 
 * return value:
 *    -1    socket error or ssl error, session will be end
 *     0    all pending data sended
 *     1    partial pending data sended
 */
int SslServerSession::SendPendingData()
{
    SendBuf *pBuf = m_pSendBufHead;
    SendBuf *pTmp;
    int ret, err;
    while (pBuf != NULL)
    {
        assert(pBuf->m_data);
        assert(pBuf->m_len > 0);
        assert(pBuf->m_len - pBuf->m_sendOff > 0);
        ret = SSL_write(m_pSSL, &pBuf->m_data[pBuf->m_sendOff], pBuf->m_len - pBuf->m_sendOff);

        if (ret == pBuf->m_len - pBuf->m_sendOff) //current buf send ok
        {
            pTmp = pBuf;
            pBuf = pBuf->m_next;
            m_pSendBufHead = pBuf;
            delete [] pTmp->m_data;
            delete pTmp;
        }
        else if (ret > 0)
        {
            pBuf->m_sendOff += ret;
        }
        else
        {
            err = SSL_get_error(m_pSSL, ret);
            assert(err != SSL_ERROR_WANT_READ);
            if (ret < 0 && err == SSL_ERROR_WANT_WRITE)
            {
                return 1;
            }
            else
            {
                SSL_shutdown(m_pSSL);
                m_state = CLOSE;
                Close();
                return -1;
            }
        }
    }
    m_pSendBufHead = m_pSendBufTail = NULL;
    if (-1 == sw_ev_io_del(m_pSessionMgr->GetEventCtx(), m_socket, SW_EV_WRITE))
    {
        printf("sw_ev_io_del failed. At %s:%d\n", basename(__FILE__), __LINE__);
    }
    return 0; //all pending data sended
}

#endif

