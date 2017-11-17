#pragma once

#include <unordered_map>
#include <vector>
#include <set>
#include <sw_event.h>

#include "TcpSession.h"
#include "DefaultTcpSession.h"
#include "HttpSession.h"

#define INVALID_SESSION_ID  (-1UL)


class TcpSessionManager;

struct ListenInfo
{
    int m_bindIp;
    int m_bindPort;
    int m_listenFd;
    int m_sessionType; // new created sessionn use this type
    TcpSessionManager * m_pSessionMgr;
};

enum  ESessionType
{
    DEFAULT_TCP_SESSION,
    HTTP_SESSION
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
     * Overide this function to map sessionid and user object.
     */
    virtual void OnSessionHasBegin(TcpSession *pSession) {   }
    /**
     * Overide this function to unmap sessionid and user object.
     */
    virtual void OnSessionWillEnd(TcpSession *pSession) {   }
    /**
     * Overide this function to implement business logic for mormal TcpSession .
     */
    virtual void ProcessMessage(TcpSession *pSession, const char * data, unsigned len) {   }
    /**
     * Process http request
     */
    virtual void ProcessHttpMessage(HttpSession *pHttpSession) {        }

protected:
    bool AddSession(TcpSession *pSession);
    void RemoveSession(unsigned long sessionId);
    void OnAccept(int listenFd, int sessionType);
    void OnSessionGC();
    static void OnAcceptReady(const int listenFd, int events, void *arg);
    static void OnConnectReady(const int connFd, int events, void *arg);
    static void OnSessionGCTimer(void *arg);
protected:
    struct sw_ev_context *m_pEvCtx;
    std::unordered_map<unsigned long, TcpSession*> m_sessions; // key is sessionid
    std::vector<ListenInfo*> m_listens;
    std::set<TcpSession*> m_sessionGC;  // session objects garbage collection
    struct sw_ev_timer *  m_pSessionGCTimer;
};