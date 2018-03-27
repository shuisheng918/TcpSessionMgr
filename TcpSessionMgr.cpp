#include "TcpSessionMgr.h"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "utils.h"
#include "DefaultTcpSession.h"
#include "HttpServerSession.h"
#include "HttpClientSession.h"

#ifdef ENABLE_SSL
#include "SslSession.h"
#include "HttpsServerSession.h"
#include "HttpsClientSession.h"
#endif

using namespace std;

void SetNonBlock(int sock)
{
    int opts;
    opts=fcntl(sock,F_GETFL);
    if(opts<0)
    {
        logerror("fcntl: %s.", strerror(errno));
        return;
    }
    opts = opts|O_NONBLOCK;
    if(fcntl(sock,F_SETFL,opts)<0)
    {
        logerror("fcntl: %s.", strerror(errno));
        return;
    }
}

static inline unsigned long GenSessionId() 
{
    static unsigned long seed = 0;
    unsigned long id =  __sync_add_and_fetch (&seed, 1);
    if (id == INVALID_SESSION_ID)
    {
        return __sync_add_and_fetch (&seed, 1);
    }
    return id;
}


void TcpSessionMgr::OnAcceptReady(const int listenFd, int events, void *arg)  
{
    (void)events;
    ListenInfo *pListenInfo = (ListenInfo *)arg;
    if (pListenInfo != NULL)
    {
        TcpSessionMgr *pSessionMgr = pListenInfo->m_pSessionMgr;
        if (pSessionMgr != NULL)
        {
            assert(listenFd == pListenInfo->m_listenFd);
            pSessionMgr->OnAccept(pListenInfo->m_listenFd, pListenInfo->m_sessionType);
        }
    }
}

struct ConnectInfo
{
    TcpSessionMgr *m_pSessionMgr;
    int m_peerIp;
    int m_peerPort;
    int m_sessionType;
    struct sw_ev_context *m_pEvCtx;
};

void TcpSessionMgr::OnConnectReady(const int connFd, int events, void *arg)
{
    int sockErrno = 0;
    socklen_t len = 0;
    if (-1 == getsockopt(connFd, SOL_SOCKET, SO_ERROR, &sockErrno, &len))
    {
        logerror("getsockopt: %s.", strerror(errno));
        return;
    }
    ConnectInfo *pConnInfo = (ConnectInfo*)arg;
    assert(pConnInfo);
    assert(pConnInfo->m_pSessionMgr);
    if (sockErrno == 0 && (events & SW_EV_WRITE)) //ok ,connect success
    {
        sw_ev_io_del(pConnInfo->m_pEvCtx, connFd, SW_EV_WRITE | SW_EV_READ);
        pConnInfo->m_pSessionMgr->OnConnect(connFd, pConnInfo->m_peerIp, pConnInfo->m_peerPort, 1, pConnInfo->m_sessionType);
    }
    else // connect fail
    {
        pConnInfo->m_pSessionMgr->OnConnect(connFd, pConnInfo->m_peerIp, pConnInfo->m_peerPort, 0, pConnInfo->m_sessionType);
        sw_ev_io_del(pConnInfo->m_pEvCtx, connFd, SW_EV_WRITE | SW_EV_READ);
        close(connFd);
    }
    delete pConnInfo;
}

void TcpSessionMgr::OnCheck(void *arg)
{
    TcpSessionMgr *pMgr = (TcpSessionMgr *)arg;
    if (pMgr != NULL)
    {
        pMgr->OnSessionGC();
    }
}

int TcpSessionMgr::BindAndListen(const char *ip, unsigned short port, int sessionType)
{
    if (ip == NULL)
    {
        return -1;
    }
    ListenInfo *pListenInfo = new ListenInfo;
    int listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSock == -1)
    {
       logerror("socket: %s.", strerror(errno));
       return -1;
    }
    int reuse = 1;
    if (setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) == -1)
    {
       logerror("setsockopt: %s.", strerror(errno));
       return -1;
    }
    struct sockaddr_in serverAddr;  
    bzero(&serverAddr,sizeof(struct sockaddr_in));  
    serverAddr.sin_family = AF_INET;  
    serverAddr.sin_addr.s_addr = inet_addr(ip);
    serverAddr.sin_port = htons(port);

    if(::bind(listenSock, (struct sockaddr *)(&serverAddr), sizeof(struct sockaddr)) == -1)  
    {  
       logerror("bind: %s.", strerror(errno));
       return -1;
    }  
    if (listen(listenSock, 128) == -1)
    {  
       logerror("listen: %s.", strerror(errno));
       return -1;
    }
    SetNonBlock(listenSock);

    pListenInfo->m_bindIp = ntohl(serverAddr.sin_addr.s_addr);
    pListenInfo->m_bindPort = port;
    pListenInfo->m_listenFd = listenSock;
    pListenInfo->m_pSessionMgr = this;
    pListenInfo->m_sessionType = sessionType;
    if (-1 == sw_ev_io_add(m_pEvCtx, listenSock, SW_EV_READ, OnAcceptReady, pListenInfo))
    {
        logerror("sw_ev_io_add failed.");
        close(listenSock);
        delete pListenInfo;
        return -1;
    }
    m_listens.push_back(pListenInfo);

}

void TcpSessionMgr::OnAccept(int listenFd, int sessionType)
{
    struct sockaddr_in addr;  
    socklen_t addrlen = sizeof(addr);  
    int client;
    
    while (true)
    {
        client = accept(listenFd, (struct sockaddr *) &addr, &addrlen);
        if (client >= 0)
        {
            if (-1 == BeginSession(client, ntohl(addr.sin_addr.s_addr), ntohs(addr.sin_port), sessionType))
            {
                close(client);
            }
        }
        else if (errno == EAGAIN)
        {
            break;
        }
        else // listen socket occur error
        {
            logerror("listen socket exception");
            return;
        }
    }
}

void TcpSessionMgr::OnSessionGC()
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

/**
 * return
 *   -1   connect error
 *    0   request connect ok. 
 */
int TcpSessionMgr::Connect(const char *ip, unsigned short port, int sessionType)
{
    int connFd = socket(AF_INET, SOCK_STREAM, 0);
    if (connFd == -1)
    {
       logerror("socket: %s.", strerror(errno));
       return -1;
    }
    struct sockaddr_in serverAddr;  
    bzero(&serverAddr, sizeof(struct sockaddr_in));
    serverAddr.sin_family = AF_INET;  
    serverAddr.sin_addr.s_addr = inet_addr(ip);
    if (serverAddr.sin_addr.s_addr == INADDR_NONE)
	{
	    char   dnsBuf[8192];
        struct hostent hostinfo, *phost;
        int err = 0;
        int ret = gethostbyname_r(ip, &hostinfo, dnsBuf, sizeof(dnsBuf), &phost, &err);
        if (ret == 0 && phost != NULL)
        {
            serverAddr.sin_addr.s_addr = *(uint32_t*)hostinfo.h_addr;
        }
        else
		{
			logerror("gethostbyname_r failed, ip=%s", ip);
			return -1;
		}
	}
    serverAddr.sin_port = htons(port);
    SetNonBlock(connFd);
    int ret = connect(connFd, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr));
    if (ret != 0)
    {
        if (errno != EINPROGRESS)  // occour error
        {
            close(connFd);
            logerror("connect %s:%d failed : %s.", ip, port, strerror(errno));
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
            logerror("sw_ev_io_add failed.");
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

void TcpSessionMgr::OnConnect(int connFd, int peerIp, int peerPort, int success, int sessionType)
{
    if (success)
    {
        if (-1 == BeginSession(connFd, peerIp, peerPort, sessionType))
        {
            close(connFd);
        }
    }
    else
    {
        char addrBuf[16];
        unsigned char part1 = (((unsigned)peerIp & 0xff000000) >> 24);
        unsigned char part2 = (((unsigned)peerIp & 0xff0000) >> 16);
        unsigned char part3 = (((unsigned)peerIp & 0xff00) >> 8);
        unsigned char part4 = (((unsigned)peerIp & 0xff));
        snprintf(addrBuf, sizeof(addrBuf), "%d.%d.%d.%d", 
                 part1, part2, part3, part4);
        logerror("connect %s:%d failed, sessionType=%d", addrBuf, peerPort, sessionType);
    }
}

int TcpSessionMgr::BeginSession(int connFd, int peerIp, int peerPort, int sessionType)
{
    SetNonBlock(connFd);
    TcpSession *pSession = CreateSession(sessionType);
    if (pSession == NULL)
    {
        logerror("CreateSession failed");
        return -1;
    }
    pSession->m_sessionId = GenSessionId();
    pSession->m_peerIp = peerIp;
    pSession->m_peerPort = peerPort;
    pSession->m_socket = connFd;
    pSession->m_sessionType = sessionType;
    pSession->m_pSessionMgr = this;
    if (-1 == sw_ev_io_add(m_pEvCtx, pSession->m_socket, SW_EV_READ, TcpSession::OnSessionIOReady, pSession))
    {
        logerror("sw_ev_io_add failed.");
        delete pSession;
        return -1;
    }
    while (!pSession->m_pSessionMgr->AddSession(pSession))
    {
        pSession->m_sessionId = GenSessionId();
    }

    if (m_pSessionGCCheck == NULL)
    {
        m_pSessionGCCheck =  sw_ev_check_add(m_pEvCtx, OnCheck, this);
    }
    OnSessionHasBegin(pSession);
    
    return 0;
}

void TcpSessionMgr::EndSession(unsigned long sessionId)
{
    TcpSession *pSession = GetSession(sessionId);
    if (pSession != NULL)
    {
        if (pSession->m_socket != -1)
        {
            sw_ev_io_del(m_pEvCtx, pSession->m_socket, SW_EV_READ | SW_EV_WRITE);
            close(pSession->m_socket);
            pSession->m_socket = -1;
        }
        RemoveSession(sessionId);
        m_sessionGC.insert(pSession);
    }
}

bool TcpSessionMgr::AddSession(TcpSession *pSession)
{
    if (pSession != NULL)
    {
        auto ret = m_sessions.insert(pair<unsigned int, TcpSession*>(pSession->m_sessionId, pSession));
        return ret.second;
    }
    return false;
}

void TcpSessionMgr::RemoveSession(unsigned long sessionId)
{
    m_sessions.erase(sessionId);
}

TcpSession * TcpSessionMgr::GetSession(unsigned long sessionId)
{
    auto iter = m_sessions.find(sessionId);
    if (iter != m_sessions.end())
    {
        return iter->second;
    }
    return NULL;
}

void TcpSessionMgr::SendData(unsigned long sessionId, const char * data, int len)
{
    TcpSession *pSession = GetSession(sessionId);
    if (pSession != NULL)
    {
        pSession->SendData(data, len);
    }
}

TcpSession * TcpSessionMgr::CreateSession(int sessionType)
{
    TcpSession *pNewSession = NULL;
    switch (sessionType)
    {
        case DEFAULT_TCP_SESSION:
            pNewSession = new DefaultTcpSession;
            break;
        case HTTP_SERVER_SESSION:
            pNewSession = new HttpServerSession;
            break;
        case HTTP_CLIENT_SESSION:
            pNewSession = new HttpClientSession;
            break;
#ifdef ENABLE_SSL
        case SSL_SERVER_SESSION:
        case SSL_CLIENT_SESSION:
            pNewSession = new SslSession;
            break;
        case HTTPS_SERVER_SESSION:
            pNewSession = new HttpsServerSession;
            break;
        case HTTPS_CLIENT_SESSION:
            pNewSession = new HttpsClientSession;
            break;
#endif
    }
    return pNewSession;
}

TcpSessionMgr::~TcpSessionMgr()
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
    if (m_pSessionGCCheck)
    {
        sw_ev_check_del(m_pEvCtx, m_pSessionGCCheck);
        m_pSessionGCCheck = NULL;
    }
    m_pEvCtx = NULL;
}

