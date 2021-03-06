/**
 * 一个简单高效的TCP会话管理器。
 * 用session id 区分每一个tcp连接会话, 底层网络事件读写基于高性能 libswevent 库.
 * 几分钟的移植时间就可让你的程序拥有高效率处理网络会话功能，包括侦听客户端
 * 连接、主动连接其他服务器，并将tcp会话统一管理。
 * 
 * Copyright (c) 2017 ShuishengWu <shuisheng918@gmail.com>
 */

#include "tcp_session.h"
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sw_event.h>
#include "utils.h"
#include "tcp_session_mgr.h"


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
    if (m_socket != -1)
    {
        sw_ev_io_del(m_pSessionMgr->GetEventCtx(), m_socket, SW_EV_READ | SW_EV_WRITE);
        close(m_socket);
        m_socket = -1;
    }
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
    m_sessionId = -1;
    m_sessionType = 0;
    
}

void TcpSession::OnRead()
{
    int ret;
    bool bNeedEndSession = false;
    char buf[4096];
    ret = read(m_socket, buf, sizeof(buf));
    if (ret > 0)
    {
        OnRecvData(buf, ret);
    }
    else if (ret == 0)
    {// closed by peer host
        bNeedEndSession = true;
    }
    else if (ret < 0) // no need process EINTR error in non-block socket
    {
        if (errno != EAGAIN)
        {// socket occur exception
            bNeedEndSession = true;
        }
    }
    if (bNeedEndSession)
    {
        Close();
    }
}

void TcpSession::OnWrite()
{
    int ret = SendPendingData();
    if (ret == 0 && m_sentClose)
    {
        Close();
    }
}

void TcpSession::Close()
{
    m_pSessionMgr->EndSession(m_sessionId);
}

void TcpSession::ForceClose()
{
    struct linger ling = {1, 0};
    setsockopt(m_socket, SOL_SOCKET, SO_LINGER, (void*)&ling, sizeof(ling));
    m_pSessionMgr->EndSession(m_sessionId);
}

void TcpSession::SetSentClose(bool sentClose)
{
    m_sentClose = sentClose;
    if (m_sentClose && m_pSendBufHead == NULL)
    {
        Close();
    }
}

void TcpSession::SendData(const char *data, int len)
{
    if (data == NULL || len <= 0)
    {
        return;
    }
    int sended = 0;
    if (m_pSendBufTail == NULL)
    {
        int ret;
        while ((ret = write(m_socket, &data[sended], len - sended)))
        {
            assert(len - sended > 0);
            if (ret == len - sended) //ok
            {
                if (m_sentClose)
                {
                    Close();
                }
                return;
            }
            else if (ret >= 0)
            {
                sended += ret;
            }
            else // ret == -1
            {
                if (errno == EAGAIN)
                {
                    goto topending;
                }
                else //socket exception
                {
                    Close();
                    return;
                }
            }
        }
    }
    else //append data to pending data list tail
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
            logerror("sw_ev_io_add failed.");
        }
    }
}

/** 
 * return value:
 *    -1    socket error, session will be end
 *     0    all pending data sended
 *     1    partial pending data sended
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
            if (errno == EAGAIN)
            {
                return 1;  //sended some part
            }
            else //socket exception
            {
                Close();
                return -1;
            }
        }
    }
    m_pSendBufHead = m_pSendBufTail = NULL;
    if (-1 == sw_ev_io_del(m_pSessionMgr->GetEventCtx(), m_socket, SW_EV_WRITE))
    {
        logerror("sw_ev_io_del failed.");
    }
    return 0; //all pending data sended
}




