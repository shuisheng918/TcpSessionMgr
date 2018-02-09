#include <signal.h>
#include <stdio.h>
#include "utils.h"
#include "TcpSessionMgr.h"
#include "HttpClientSession.h"

class MyHttpClient : public TcpSessionMgr
{
public:
    virtual void OnSessionHasBegin(TcpSession *pSession)
    {
        //log("OnSessionHasBegin,sid=%lu", pSession->GetSessionID());
        if (pSession->GetSessionType() == HTTP_CLIENT_SESSION)
        {
            HttpClientSession *pHttpSession = dynamic_cast<HttpClientSession*>(pSession);
            HttpRequest *pRequest = pHttpSession->GetHttpRequest();
            pRequest->SetMethod("GET");
            pRequest->SetURL("/");
            pHttpSession->SetKeepAlive(true);
            for (int i = 0; i < 10; ++i)
            {
                pHttpSession->SendRequest();
            }
        }
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
    MyHttpClient *pHttpClient = new MyHttpClient;
    pHttpClient->SetEventCtx(pEvCtx);
    for (int i = 0; i < 1; ++i)
    {
        pHttpClient->Connect("localhost", 1111, HTTP_CLIENT_SESSION);
    }
    sw_ev_loop(pEvCtx);
    delete pHttpClient;
    pHttpClient = NULL;
    sw_ev_context_free(pEvCtx);
}


