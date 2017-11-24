#include "TcpSessionMgr.h"
#include <signal.h>
#include <stdio.h>

class MyHttpServer : public TcpSessionManager
{
public:
    virtual void OnSessionHasBegin(TcpSession *pSession)
    {
        //printf("OnSessionHasBegin,sid=%lu\n", pSession->GetSessionID());
    }
    virtual void OnSessionWillEnd(TcpSession *pSession)
    {
        //printf("OnSessionWillEnd,sid=%lu\n", pSession->GetSessionID());
    }

};

void OnSignal(int sig, void * arg)
{
    if (sig == SIGINT || sig == SIGTERM)
    {
        sw_ev_loop_exit((sw_ev_context_t *)arg);
        printf("exit loop\n");
    }
}

void OnLog(int level, const char *msg)
{
    printf("libswevent log: level=%d, msg=%s\n", level, msg);
}

int main(void)
{
    sw_ev_context_t * pEvCtx = sw_ev_context_new();
    sw_set_log_func(OnLog);
    sw_ev_signal_add(pEvCtx, SIGINT, OnSignal, pEvCtx);
    sw_ev_signal_add(pEvCtx, SIGTERM, OnSignal, pEvCtx);
    sw_ev_signal_add(pEvCtx, SIGPIPE, OnSignal, pEvCtx);
    MyHttpServer *pHttpd = new MyHttpServer;
    pHttpd->SetEventCtx(pEvCtx);
    pHttpd->BindAndListen("0.0.0.0", 1111, HTTP_SERVER_SESSION);
    sw_ev_loop(pEvCtx);
    delete pHttpd;
    pHttpd = NULL;
    sw_ev_context_free(pEvCtx);
}

