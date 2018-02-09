#pragma once

#include <unordered_map>
#include <vector>
#include <set>
#include <sw_event.h>

#include "TcpSession.h"

#define INVALID_SESSION_ID  (-1UL)


class TcpSessionMgr;

struct ListenInfo
{
    int m_bindIp;
    int m_bindPort;
    int m_listenFd;
    int m_sessionType; // new created sessionn use this type
    TcpSessionMgr * m_pSessionMgr;
};

enum  ESessionType
{
    DEFAULT_TCP_SESSION,
    HTTP_SERVER_SESSION,
    HTTP_CLIENT_SESSION,
#ifdef ENABLE_SSL
    SSL_SERVER_SESSION,
    SSL_CLIENT_SESSION,
    HTTPS_SERVER_SESSION,
    HTTPS_CLIENT_SESSION,
#endif
    // add your self session type here
};

class TcpSessionMgr
{
public:
    TcpSessionMgr() : m_pEvCtx(NULL), m_pSessionGCCheck(NULL)
    {
    }
    virtual ~TcpSessionMgr();
    
    void SetEventCtx(sw_ev_context_t *ctx) { m_pEvCtx = ctx; }
    sw_ev_context_t * GetEventCtx() { return m_pEvCtx; }
    void BindAndListen(const char *ip, unsigned short port, int sessionType);

    /**
     * connect asynchronous, return 0 success, -1 error.
     */
    int Connect(const char *ip, unsigned short port, int sessionType);

    /**
     * return 0 success, -1 failed. 
     * arguments:
     * connFd       can be tcp socket fd, pipe or unix socket fd.
     * sessionType  used to create sesionn object
     */
    int  BeginSession(int connFd, int peerIp, int peerPort, int sessionType);
    void EndSession(unsigned long sessionId);
    TcpSession * GetSession(unsigned long sessionId);
    void SendData(unsigned long sessionId, const char * data, int len);

    /**
     * New a different TcpSession instances according sessionType, You can overide this function
     * and create yourself TcpSession's subclass objects. You needn't delete it. It will be deleted
     * automatically.
     */
    virtual TcpSession * CreateSession(int sessionType);
    virtual void OnConnect(int connFd, int peerIp, int peerPort, int success, int sessionType);
    /**
     * Overide this function to map sessionid and user object according your business logic.
     */
    virtual void OnSessionHasBegin(TcpSession *pSession) {   }
    /**
     * Overide this function to unmap sessionid and user object according your business logic.
     */
    virtual void OnSessionWillEnd(TcpSession *pSession) {   }
protected:
    bool AddSession(TcpSession *pSession);
    void RemoveSession(unsigned long sessionId);
    void OnAccept(int listenFd, int sessionType);
    void OnSessionGC();
    static void OnAcceptReady(const int listenFd, int events, void *arg);
    static void OnConnectReady(const int connFd, int events, void *arg);
    static void OnCheck(void *arg);
protected:
    sw_ev_context_t *m_pEvCtx;
    std::unordered_map<unsigned long, TcpSession*> m_sessions; // key is sessionid
    std::vector<ListenInfo*> m_listens;
    std::set<TcpSession*> m_sessionGC;  // session objects garbage collection
    sw_ev_check_t   *m_pSessionGCCheck;
};