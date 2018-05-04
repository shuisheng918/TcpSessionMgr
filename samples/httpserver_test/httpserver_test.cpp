#include <signal.h>
#include <stdio.h>
#include "utils.h"
#include "tcp_session_mgr.h"

class MyHttpServer : public TcpSessionMgr
{
public:
    virtual void OnSessionHasBegin(TcpSession *pSession)
    {
        //log("OnSessionHasBegin,sid=%lu", pSession->GetSessionID());
    }
    virtual void OnSessionWillEnd(TcpSession *pSession)
    {
        //log("OnSessionWillEnd,sid=%lu", pSession->GetSessionID());
    }

};

void OnSignal(int sig, void * arg)
{
    if (sig == SIGINT || sig == SIGTERM)
    {
        sw_ev_loop_exit((sw_ev_context_t *)arg);
        log("exit loop");
    }
}

void OnLog(int level, const char *msg)
{
    log("libswevent log: level=%d, msg=%s", level, msg);
}

int main(void)
{
    if (!SimpleLog::GetInstance()->Startup("", 0, 0))
    {
        printf("log startup failed, exit\n");
        exit(1);
    }

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

