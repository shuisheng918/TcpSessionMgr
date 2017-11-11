/**
 * 一个简单高效的TCP会话管理器。
 * 用session id 区分每一个tcp连接会话, 底层网络事件读写基于高性能 libswevent 库.
 * 几分钟的移植时间就可让你的程序拥有高效率处理网络会话功能，包括侦听客户端
 * 连接、主动连接其他服务器，并将tcp会话统一管理。
 * 
 * Copyright (c) 2017 ShuishengWu <shuisheng918@gmail.com>
 */

#include "TcpSession.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

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

void TcpSessionManager::OnAcceptReady(const int listenFd, int events, void *arg)  
{
    (void)events;
    ListenInfo *pListenInfo = (ListenInfo *)arg;
    if (pListenInfo != NULL)
    {
        TcpSessionManager *pSessionMgr = pListenInfo->m_pSessionMgr;
        if (pSessionMgr != NULL)
        {
            assert(listenFd == pListenInfo->m_listenFd);
            pSessionMgr->OnAccept(pListenInfo->m_listenFd, pListenInfo->m_sessionType);
        }
    }
}

struct ConnectInfo
{
    TcpSessionManager *m_pSessionMgr;
    int m_peerIp;
    int m_peerPort;
    int m_sessionType;
    struct sw_ev_context *m_pEvCtx;
};

void TcpSessionManager::OnConnectReady(const int connFd, int events, void *arg)
{
    int sockErrno;
    socklen_t len = 0;
    if (-1 == getsockopt(connFd, SOL_SOCKET, SO_ERROR, &sockErrno, &len))
    {
        printf("getsockopt: %s. At %s:%d\n", strerror(errno), basename(__FILE__), __LINE__);
        return;
    }

    ConnectInfo *pConnInfo = (ConnectInfo*)arg;
    if (sockErrno == 0 && (events & SW_EV_WRITE)) //ok ,connect success
    {
        if (pConnInfo != NULL && pConnInfo->m_pSessionMgr != NULL)
        {
            pConnInfo->m_pSessionMgr->OnConnect(connFd, pConnInfo->m_peerIp, pConnInfo->m_peerPort,
                                                1, pConnInfo->m_sessionType);
        }
        return;
    }
    else
    {
        pConnInfo->m_pSessionMgr->OnConnect(connFd, pConnInfo->m_peerIp, pConnInfo->m_peerPort, 
                                            0, pConnInfo->m_sessionType);
        sw_ev_io_del(pConnInfo->m_pEvCtx, connFd, SW_EV_WRITE | SW_EV_READ);
        close(connFd);
    }

    if (pConnInfo != NULL)
    {
        delete pConnInfo;
    }
}

void TcpSessionManager::OnSessionGCTimer(void *arg)
{
    TcpSessionManager *pMgr = (TcpSessionManager *)arg;
    if (pMgr != NULL)
    {
        pMgr->OnSessionGC();
    }
}

void SetNonBlock(int sock)
{
    int opts;
    opts=fcntl(sock,F_GETFL);
    if(opts<0)
    {
        printf("fcntl:%s. At %s:%d\n", strerror(errno), basename(__FILE__), __LINE__);
        return;
    }
    opts = opts|O_NONBLOCK;
    if(fcntl(sock,F_SETFL,opts)<0)
    {
        printf("fcntl:%s. At %s:%d\n", strerror(errno), basename(__FILE__), __LINE__);
        return;
    }
}

static inline unsigned long GenSessionId() 
{
    //static unsigned long seed = 0xfffffffffffffffeUL;
    static unsigned long seed = 0;
    unsigned long id =  __sync_add_and_fetch (&seed, 1);
    if (id == INVALID_SESSION_ID)
    {
        return __sync_add_and_fetch (&seed, 1);
    }
    return id;
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
    if (m_pMsgDecoder != NULL)
    {
        delete m_pMsgDecoder;
        m_pMsgDecoder = NULL;
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
            m_pMsgDecoder->AppendData(buf, ret);
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
    
    const char *pMsg = NULL;
    int msgLen = 0;
    while (true)
    {
        ret = m_pMsgDecoder->GetMessage(&pMsg, &msgLen);
        if (0 == ret)
        {
            if (pMsg != NULL && msgLen > 0)
            {
                m_pSessionMgr->ProcessMessage(this, pMsg, msgLen);
            }
        }
        else if (1 == ret)
        {
            break;
        }
        else // -1
        {
            bNeedEndSession = true;
            break;
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

//return value:
//    -1    socket error, session will be end
//     0    all pending data sended
//     1    patial pending data sended
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

void TcpSessionManager::BindAndListen(const char *ip, unsigned short port, int sessionType)
{
    if (ip == NULL)
    {
        return;
    }
    ListenInfo *pListenInfo = new ListenInfo;
    int listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSock == -1)
    {
       printf("socket:%s. At %s:%d\n", strerror(errno), basename(__FILE__), __LINE__);
       exit(1);
    }
    int reuse = 1;
    if (setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) == -1)
    {
       perror("setsockopt");
       exit(1);
    }
    struct sockaddr_in serverAddr;  
    bzero(&serverAddr,sizeof(struct sockaddr_in));  
    serverAddr.sin_family = AF_INET;  
    serverAddr.sin_addr.s_addr = inet_addr(ip);
    serverAddr.sin_port = htons(port);

    if(bind(listenSock,(struct sockaddr *)(&serverAddr),sizeof(struct sockaddr)) == -1)  
    {  
       printf("bind:%s. At %s:%d\n", strerror(errno), basename(__FILE__), __LINE__);
       exit(1);
    }  
    if (listen(listenSock, 128) == -1)
    {  
       printf("listen:%s. At %s:%d\n", strerror(errno), basename(__FILE__), __LINE__);
       exit(1);
    }
    SetNonBlock(listenSock);

    pListenInfo->m_bindIp = ntohl(serverAddr.sin_addr.s_addr);
    pListenInfo->m_bindPort = port;
    pListenInfo->m_listenFd = listenSock;
    pListenInfo->m_pSessionMgr = this;
    pListenInfo->m_sessionType = sessionType;
    if (-1 == sw_ev_io_add(m_pEvCtx, listenSock, SW_EV_READ, OnAcceptReady, pListenInfo))
    {
        printf("sw_ev_io_add failed. At %s:%d\n", basename(__FILE__), __LINE__);
        close(listenSock);
        delete pListenInfo;
        return;
    }
    m_listens.push_back(pListenInfo);

}

void TcpSessionManager::OnAccept(int listenFd, int sessionType)
{
    struct sockaddr_in addr;  
    socklen_t addrlen = sizeof(addr);  
    int client;
    
    while ( true )
    {
        client = accept(listenFd, (struct sockaddr *) &addr, &addrlen);
        if (client >= 0)
        {
            if (-1 == BeginSession(client, ntohl(addr.sin_addr.s_addr), ntohs(addr.sin_port), sessionType))
            {
                sw_ev_io_del(m_pEvCtx, client, SW_EV_READ | SW_EV_WRITE);
                close(client);
            }
        }
        else if (errno == EINTR)
        {
            continue;
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            break;
        }
        else // listen socket occur error
        {
            printf("listen socket exception, exit\n");
            exit(1);
        }
    }
}

void TcpSessionManager::OnSessionGC()
{
    TcpSession *pSession = NULL;
    for (auto it = m_sessionGC.begin(); it != m_sessionGC.end(); ++it)
    {
        pSession = *it;
        if (pSession != NULL)
        {
            OnSessionWillEnd(pSession);
            delete pSession;
        }
    }
    m_sessionGC.clear();
}

/*
return
    -1   connect error
    0    request connect ok. 
*/
int TcpSessionManager::Connect(const char *ip, unsigned short port, int sessionType)
{
    int connFd = socket(AF_INET, SOCK_STREAM, 0);
    if (connFd == -1)
    {
       printf("socket:%s. At %s:%d\n", strerror(errno), basename(__FILE__), __LINE__);
       exit(1);
    }
    struct sockaddr_in serverAddr;  
    bzero(&serverAddr, sizeof(struct sockaddr_in));
    serverAddr.sin_family = AF_INET;  
    serverAddr.sin_addr.s_addr = inet_addr(ip);
    serverAddr.sin_port = htons(port);

    SetNonBlock(connFd);
reconnect:
    int ret = connect(connFd, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr));
    if (ret != 0)
    {
        if (errno == EINTR || errno == EWOULDBLOCK)
        {
            goto reconnect;
        }
        else if (errno != EINPROGRESS)  // occour error
        {
            close(connFd);
            printf("connect %s:%d failed : %s. At %s:%d\n", ip, port, strerror(errno), basename(__FILE__), __LINE__);
            return -1;
        }
        // connect in progress
        ConnectInfo *pConnInfo = new ConnectInfo;
        pConnInfo->m_pSessionMgr = this;
        pConnInfo->m_peerIp = ntohl(serverAddr.sin_addr.s_addr);
        pConnInfo->m_peerPort = port;
        pConnInfo->m_sessionType = sessionType;
        pConnInfo->m_pEvCtx = m_pEvCtx;
        if (-1 == sw_ev_io_add(m_pEvCtx, connFd, SW_EV_WRITE | SW_EV_READ, OnConnectReady, pConnInfo))
        {
            printf("sw_ev_io_add failed. At %s:%d\n", basename(__FILE__), __LINE__);
            close(connFd);
            delete pConnInfo;
            return -1;
        }
        return 0;
    }
    else //connect success imediately
    {
        OnConnect(connFd, ntohl(serverAddr.sin_addr.s_addr), ntohs(serverAddr.sin_port), 1, sessionType);
        return 0;
    }
}

void TcpSessionManager::OnConnect(int connFd, int peerIp, int peerPort, int success, int sessionType)
{
    if (success)
    {
        if (-1 == BeginSession(connFd, peerIp, peerPort, sessionType))
        {
            sw_ev_io_del(m_pEvCtx, connFd, SW_EV_READ | SW_EV_WRITE);
            close(connFd);
        }
    }
    else
    {
        char addrBuf[16];
        unsigned char part1 = (((unsigned)peerIp && 0xff000000) >> 24);
        unsigned char part2 = (((unsigned)peerIp && 0xff0000) >> 16);
        unsigned char part3 = (((unsigned)peerIp && 0xff00) >> 8);
        unsigned char part4 = (((unsigned)peerIp && 0xff));
        snprintf(addrBuf, sizeof(addrBuf), "%d.%d.%d.%d", 
                 part1, part2, part3, part4);
        printf("connect %s:%d failed, sessionType=%d\n", addrBuf, peerPort, sessionType);
    }
}

int TcpSessionManager::BeginSession(int connFd, int peerIp, int peerPort, int sessionType)
{
    SetNonBlock(connFd);
    TcpSession *pSession = CreateSession(sessionType);
    if (pSession == NULL)
    {
        printf("CreateSession failed\n");
        return -1;
    }
    pSession->m_sessionId = GenSessionId();
    pSession->m_peerIp = peerIp;
    pSession->m_peerPort = peerPort;
    pSession->m_socket = connFd;
    pSession->m_sessionType = sessionType;
    pSession->m_pSessionMgr = this;
    if (-1 == sw_ev_io_add(m_pEvCtx, pSession->m_socket, SW_EV_READ | SW_EV_WRITE, TcpSession::OnSessionIOReady, pSession))
    {
        printf("sw_ev_io_add failed. At %s:%d\n", basename(__FILE__), __LINE__);
        delete pSession;
        return -1;
    }
    while (!pSession->m_pSessionMgr->AddSession(pSession))
    {
        pSession->m_sessionId = GenSessionId();
    }

    if (m_pSessionGCTimer == NULL)
    {
        m_pSessionGCTimer = sw_ev_timer_add(m_pEvCtx, 500, OnSessionGCTimer, this);
    }
    OnSessionHasBegin(pSession);
    
    return 0;
}

void TcpSessionManager::EndSession(unsigned long sessionId)
{
    TcpSession *pSession = GetSession(sessionId);
    if (pSession != NULL)
    {
        //OnSessionWillEnd(pSession);
        RemoveSession(sessionId);
        m_sessionGC.insert(pSession);
    }
}

bool TcpSessionManager::AddSession(TcpSession *pSession)
{
    if (pSession != NULL)
    {
        auto ret = m_sessions.insert(pair<unsigned int, TcpSession*>(pSession->m_sessionId, pSession));
        return ret.second;
    }
    return false;
}

void TcpSessionManager::RemoveSession(unsigned long sessionId)
{
    m_sessions.erase(sessionId);
}

TcpSession * TcpSessionManager::GetSession(unsigned long sessionId)
{
    auto iter = m_sessions.find(sessionId);
    if (iter != m_sessions.end())
    {
        return iter->second;
    }
    return NULL;
}

void TcpSessionManager::SendData(unsigned long sessionId, const char * data, int len)
{
    TcpSession *pSession = GetSession(sessionId);
    if (pSession != NULL)
    {
        pSession->SendData(data, len);
    }
}

TcpSession * TcpSessionManager::CreateSession(int sessionType)
{
    TcpSession *pNewSession = NULL;
    switch (sessionType)
    {
        case 0:
            pNewSession = new TcpSession(NULL);
        default:
            pNewSession = new TcpSession(NULL);
    }
    return pNewSession;
}

TcpSessionManager::~TcpSessionManager()
{
    auto iter = m_sessions.begin(), end = m_sessions.end();
    for (; iter != end; ++iter)
    {
        if (iter->second)
        {
            delete iter->second;
            iter->second = NULL;
        }
    }
    for (int i = 0; i < (int)m_listens.size(); ++i)
    {
        if (m_listens[i] != NULL)
        {
            sw_ev_io_del(m_pEvCtx, m_listens[i]->m_listenFd, SW_EV_READ);
            close(m_listens[i]->m_listenFd);
            delete m_listens[i];
            m_listens[i] = NULL;
        }
    }
    if (m_pSessionGCTimer)
    {
        sw_ev_timer_del(m_pEvCtx, m_pSessionGCTimer);
        m_pSessionGCTimer = NULL;
    }
    m_pEvCtx = NULL;
}



