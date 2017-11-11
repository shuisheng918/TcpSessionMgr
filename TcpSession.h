/**
 * 一个简单高效的TCP会话管理器。
 * 用session id 区分每一个tcp连接会话, 底层网络事件读写基于高性能 libswevent 库.
 * 几分钟的移植时间就可让你的程序拥有高效率处理网络会话功能，包括侦听客户端
 * 连接、主动连接其他服务器，并将tcp会话统一管理。
 * 
 * Copyright (c) 2017 ShuishengWu <shuisheng918@gmail.com>
 */

#pragma once

#include <sw_event.h>
#include <unordered_map>
#include <vector>
#include <list>
#include <string>
#include <set>

#define INVALID_SESSION_ID  (-1UL)

class TcpSession;
class TcpSessionManager;

void SetNonBlock(int sock);

struct SendBuf
{
    char * m_data;
    int    m_len;
    int    m_sendOff;
    SendBuf * m_next;
};


class MsgDecoderBase
{
public:
    virtual void AppendData(const char *pData, int len) = 0;
    /*
     *  return:
     *      -1  error, such as illegal message, illegal length...
     *       0  success, have got a message.
     *       1  success, but message data is not enough.
     *  args:
     *      ppMsg    *ppMsg is data pointer have got.
     *      pMsgLen  *pMsgLen is the message's length.
     */
    virtual int  GetMessage(const char **ppMsg, int *pMsgLen) = 0;
};


/*
 *  message format:
 *  |--head(4B)--|------------body--------------|
 *  head is unsigned int(big endian), store the total length of message(include head self). 
 */
class DefaultMsgDecoder : public MsgDecoderBase
{
public:
    DefaultMsgDecoder() : m_nMaxMsgLen(4096), m_nPos(0) {         }

    virtual void AppendData(const char *pData, int len)
    {
        if (pData && len > 0)
        {
            if (m_nPos > 0)
            {
                m_data.erase(0, m_nPos);
                m_nPos = 0;
            }
            m_data.append(pData, len);
        }
    }
    
    virtual int  GetMessage(const char **ppMsg, int *pMsgLen)
    {
        if (m_nPos > 0)
        {
            m_data.erase(0, m_nPos);
            m_nPos = 0;
        }
        uint32_t nTotalLen = (uint32_t)m_data.size();
        if (nTotalLen >= sizeof(uint32_t))
        {
            uint32_t nMsgLen = ((uint8_t)m_data[0] << 24)
                              | ((uint8_t)m_data[1] << 16)
                              | ((uint8_t)m_data[2] << 8)
                              | ((uint8_t)m_data[3]);
            if (nMsgLen < 4 || nMsgLen > m_nMaxMsgLen)
            {
                return -1;
            }
            if (nTotalLen >=  nMsgLen)
            {
                *ppMsg = m_data.data();
                *pMsgLen = nMsgLen;
                m_nPos = nMsgLen;
                return 0;
            }
            else
            {
                return 1;
            }
        }
        else
        {
            return 1;
        }
    }
private:
    unsigned int m_nMaxMsgLen;
    std::string m_data;
    unsigned int m_nPos;
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


struct ListenInfo
{
    int m_bindIp;
    int m_bindPort;
    int m_listenFd;
    int m_sessionType; // new created sessionn use this type
    TcpSessionManager * m_pSessionMgr;
};


class TcpSessionManager
{
public:
    TcpSessionManager() : m_pEvCtx(NULL), m_pSessionGCTimer(NULL)
    {
    }
    virtual ~TcpSessionManager();
    
    void SetEventCtx(struct sw_ev_context *ctx) { m_pEvCtx = ctx; }
    struct sw_ev_context* GetEventCtx() { return m_pEvCtx; }
    void BindAndListen(const char *ip, unsigned short port, int sessionType);

    /*
     * connect asynchronous, return 0 success, -1 error.
     */
    int Connect(const char *ip, unsigned short port, int sessionType);

    /*
     * return 0 success, -1 failed. 
     * arguments:
     * connFd       can be tcp socket fd, pipe or unix socket fd.
     * sessionType  used to create sesionn object
     */
    int  BeginSession(int connFd, int peerIp, int peerPort, int sessionType);
    void EndSession(unsigned long sessionId);
    TcpSession * GetSession(unsigned long sessionId);
    void SendData(unsigned long sessionId, const char * data, int len);

    /*
     * New a different TcpSession instances according sessionType, You can overide this function
     * and create yourself TcpSession's subclass objects. You needn't delete it. It will be deleted
     * automatically.
     */
    virtual TcpSession * CreateSession(int sessionType);
    virtual void OnConnect(int connFd, int peerIp, int peerPort, int success, int sessionType);
    /*
     * Overide this function to map sessionid and user object.
     */
    virtual void OnSessionHasBegin(TcpSession *pSession) {   }
    /*
     * Overide this function to unmap sessionid and user object.
     */
    virtual void OnSessionWillEnd(TcpSession *pSession) {   }
    /*
     * Overide this function to implement business logic.
     */
    virtual void ProcessMessage(TcpSession *pSession, const char * data, unsigned len) {   }

private:
    bool AddSession(TcpSession *pSession);
    void RemoveSession(unsigned long sessionId);
    void OnAccept(int listenFd, int sessionType);
    void OnSessionGC();
    static void OnAcceptReady(const int listenFd, int events, void *arg);
    static void OnConnectReady(const int connFd, int events, void *arg);
    static void OnSessionGCTimer(void *arg);
private:
    struct sw_ev_context *m_pEvCtx;
    std::unordered_map<unsigned long, TcpSession*> m_sessions; // key is sessionid
    std::vector<ListenInfo*> m_listens;
    std::set<TcpSession*> m_sessionGC;  // session objects garbage collection
    struct sw_ev_timer *  m_pSessionGCTimer;
};

