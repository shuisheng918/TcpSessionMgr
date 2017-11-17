/**
 * 一个简单高效的TCP会话管理器。
 * 用session id 区分每一个tcp连接会话, 底层网络事件读写基于高性能 libswevent 库.
 * 几分钟的移植时间就可让你的程序拥有高效率处理网络会话功能，包括侦听客户端
 * 连接、主动连接其他服务器，并将tcp会话统一管理。
 * 
 * Copyright (c) 2017 ShuishengWu <shuisheng918@gmail.com>
 */

#include "TcpSession.h"
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <sw_event.h>

#include "TcpSessionMgr.h"

using namespace std;

void TcpSession::OnSessionIOReady(int fd, int events, void *arg)
{
    TcpSession *pSession = (TcpSession*)arg;
    if (pSession != NULL)
    {
        assert(fd == pSession->GetSocket());
        if (events & SW_EV_WRITE)
        {
            pSession->OnWrite();
        }
        if (events & SW_EV_READ)
        {
            pSession->OnRead();
        }
    }
}

TcpSession::~TcpSession()
{
    sw_ev_io_del(m_pSessionMgr->GetEventCtx(), m_socket, SW_EV_READ | SW_EV_WRITE);
    close(m_socket);
    m_socket = -1;
    if (m_pSendBufHead != NULL)
    {
        SendBuf *pTmp;
        while (m_pSendBufHead)
        {
            pTmp = m_pSendBufHead->m_next;
            if (m_pSendBufHead->m_data)
            {
                delete [] m_pSendBufHead->m_data;
            }
            delete m_pSendBufHead;
            m_pSendBufHead = pTmp;
        }
        m_pSendBufTail = NULL;
    }

    m_peerIp = 0;
    m_peerPort = 0;
    m_socket = -1;
    m_sessionId = -1;
    m_sessionType = 0;
    
}

void TcpSession::OnRead()
{
    // default implement
    int ret;
    bool bNeedEndSession = false;
    char buf[4096];
    while (true)
    {
        ret = read(m_socket, buf, sizeof(buf));
        if (ret > 0)
        {
            m_pSessionMgr->ProcessMessage(this, buf, ret);
        }
        else if (ret == 0)
        {// closed by peer host
            bNeedEndSession = true;
            break;
        }
        else if (ret < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            else
            {// socket occur exception
                bNeedEndSession = true;
                break;
            }
        }
    }
    
    if (bNeedEndSession)
    {
        m_pSessionMgr->EndSession(m_sessionId);
    }
}

void TcpSession::OnWrite()
{
    SendPendingData();
}

void TcpSession::Close()
{
    m_pSessionMgr->EndSession(m_sessionId);
}

void TcpSession::SendData(const char *data, int len)
{
    if (data == NULL || len <= 0)
    {
        return;
    }
    int retSendPending = SendPendingData();
    int sended = 0;
    if (retSendPending == 0)
    {
        int ret;
        while ((ret = write(m_socket, &data[sended], len - sended)))
        {
            assert(len - sended > 0);
            if (ret == len - sended) //ok
            {
                return;
            }
            else if (ret >= 0)
            {
                sended += ret;
            }
            else // ret == -1
            {
                if (errno == EINTR)
                {
                    continue;
                }
                else if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    //event_add(&m_wev, NULL);
                    goto topending;
                }
                else //socket exception
                {
                    m_pSessionMgr->EndSession(m_sessionId);
                    return;
                }
            }
        }
    }
    else if (retSendPending == 1) //append data to pending data list tail
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
    }
    else  //socket exception
    {
        return;
    }
}

/** 
 * return value:
 *    -1    socket error, session will be end
 *     0    all pending data sended
 *     1    patial pending data sended
 */
int TcpSession::SendPendingData()
{
    SendBuf *pBuf = m_pSendBufHead;
    SendBuf *pTmp;
    int ret;
    while (pBuf != NULL)
    {
        ret = write(m_socket, &pBuf->m_data[pBuf->m_sendOff], pBuf->m_len - pBuf->m_sendOff);
        assert(pBuf->m_data);
        assert(pBuf->m_len > 0);
        assert(pBuf->m_len - pBuf->m_sendOff > 0);
        if (ret == pBuf->m_len - pBuf->m_sendOff) //current buf send ok
        {
            pTmp = pBuf;
            pBuf = pBuf->m_next;
            m_pSendBufHead = pBuf;
            delete [] pTmp->m_data;
            delete pTmp;
        }
        else if (ret >= 0)
        {
            pBuf->m_sendOff += ret;
        }
        else // ret == -1
        {
            if (errno == EINTR)
            {
                continue;
            }
            else if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return 1;  //sended some part
            }
            else //socket exception
            {
                m_pSessionMgr->EndSession(m_sessionId);
                return -1;
            }
        }
    }
    m_pSendBufHead = m_pSendBufTail = NULL;
    return 0; //all pending data sended
}




