/**
 * 一个简单高效的TCP会话管理器。
 * 用session id 区分每一个tcp连接会话, 底层网络事件读写基于高性能 libswevent 库.
 * 几分钟的移植时间就可让你的程序拥有高效率处理网络会话功能，包括侦听客户端
 * 连接、主动连接其他服务器，并将tcp会话统一管理。
 * 
 * Copyright (c) 2017 ShuishengWu <shuisheng918@gmail.com>
 */

#pragma once

#include "MsgDecoder.h"

class TcpSession;
class TcpSessionManager;

struct SendBuf
{
    char * m_data;
    int    m_len;
    int    m_sendOff;
    SendBuf * m_next;
};

class TcpSession
{
    friend class TcpSessionManager;
public:
    TcpSession(MsgDecoderBase *pMsgDecoder) : m_sessionId(-1), m_peerIp(0), m_peerPort(0), m_socket(-1), 
        m_sessionType(0), m_pUserObj(NULL), m_pSendBufHead(NULL), m_pSendBufTail(NULL), 
        m_pMsgDecoder(pMsgDecoder), m_pSessionMgr(NULL)
    {
        if (m_pMsgDecoder == NULL)
        {
            m_pMsgDecoder = new DefaultMsgDecoder;
        }
    }
    virtual ~TcpSession();
    virtual void OnRead();
    virtual void OnWrite();
    void SendData(const char *data, int len);
    int  SendPendingData();
    unsigned long GetSessionID()
    {
        return m_sessionId;
    }
    int GetPeerIP()
    {
        return m_peerIp;
    }
    int GetPeerPort()
    {
        return m_peerPort;
    }
    int GetSessionType()
    {
        return m_sessionType;
    }
    int GetSocket()
    {
        return m_socket;
    }
    void SetUserObj(void *pObj)
    {
        m_pUserObj = pObj;
    }
    void * GetUserObj()
    {
        return m_pUserObj;
    }
private:
    static void OnSessionIOReady(int fd, int events, void *arg);
    
    unsigned long m_sessionId;
    int  m_peerIp;
    int  m_peerPort;
    int  m_socket;
    int  m_sessionType;
    void * m_pUserObj;
    SendBuf * m_pSendBufHead;
    SendBuf * m_pSendBufTail;
    MsgDecoderBase *m_pMsgDecoder;
    TcpSessionManager * m_pSessionMgr;
};



