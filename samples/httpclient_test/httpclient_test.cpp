#include <signal.h>
#include <stdio.h>
#include "TcpSessionMgr.h"
#include "HttpClientSession.h"

class MyHttpClient : public TcpSessionManager
{
public:
    virtual void OnSessionHasBegin(TcpSession *pSession)
    {
        //printf("OnSessionHasBegin,sid=%lu\n", pSession->GetSessionID());
        if (pSession->GetSessionType() == HTTP_CLIENT_SESSION)
        {
            HttpClientSession *pHttpSession = (HttpClientSession*)pSession;
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


