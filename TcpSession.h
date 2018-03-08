

#pragma once

#include <stdlib.h>

class TcpSessionMgr;

struct SendBuf
{
    char * m_data;
    int    m_len;
    int    m_sendOff;
    SendBuf * m_next;
};

class TcpSession
{
    friend class TcpSessionMgr;
public:
    TcpSession() : m_sessionId(-1), m_peerIp(0), m_peerPort(0), m_socket(-1), 
        m_sessionType(0), m_pUserObj(NULL), m_pSendBufHead(NULL), m_pSendBufTail(NULL), m_pSessionMgr(NULL), m_sentClose(false)
    {
    }
    
    virtual void OnRead();
    virtual void OnWrite();
    virtual void OnRecvData(const char *data, int len) = 0;

    void Close();
	// close socket with RESET
    void ForceClose();
    
    /**
     * When sentClose is true, this session will be closed after sended all pending data.
     */
    void SetSentClose(bool sentClose);
    virtual void SendData(const char *data, int len);
    virtual int  SendPendingData();
    unsigned long GetSessionID() { return m_sessionId; }
    int    GetPeerIP() { return m_peerIp; }
    int    GetPeerPort() { return m_peerPort; }
    int    GetSessionType() { return m_sessionType; }
    int    GetSocket() { return m_socket; }
    void   SetUserObj(void *pObj) { m_pUserObj = pObj; }
    void * GetUserObj() { return m_pUserObj; }
    TcpSessionMgr * GetSessionManager() { return m_pSessionMgr; }
    
protected:
    /**
     * All tcp sessions is deleted by TcpSessionMgr automaticly, user code needn't delete it directly.
     * So destructor function is not public.
     */
    virtual ~TcpSession();    
    
    static void OnSessionIOReady(int fd, int events, void *arg);
    
    unsigned long m_sessionId;
    int  m_peerIp;
    int  m_peerPort;
    int  m_socket;
    int  m_sessionType;
    void * m_pUserObj;
    SendBuf * m_pSendBufHead;
    SendBuf * m_pSendBufTail;
    TcpSessionMgr * m_pSessionMgr;
    bool m_sentClose;
};



