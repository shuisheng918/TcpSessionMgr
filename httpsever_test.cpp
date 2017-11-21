#include "TcpSessionMgr.h"
#include <signal.h>

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

    virtual void ProcessHttpMessage(HttpSession *pHttpSession)
    {
        //printf("-----ProcessHttpMessage, sid=%lu\n", pHttpSession->GetSessionID());
        pHttpSession->m_response.m_responseCode = HttpResponse::HTTP_OK;
        pHttpSession->m_response.m_headFields["Content-Type"] = "text/html";
        pHttpSession->m_response.m_body = "<head><title>HELLO</title></head><body><h2>Hello world!</h2></body>";
        pHttpSession->m_response.SendResponse();
    }

};

void OnSignal(int sig, void * arg)
{
    sw_ev_loop_exit((sw_ev_context_t *)arg);
    printf("OnSignal\n");
}

void OnLog(int level, const char *msg)
{
    printf("%d: %s\n", level, msg);
}

int main(void)
{
    sw_ev_context_t * pEvCtx = sw_ev_context_new();
    sw_set_log_func(OnLog);
    sw_ev_signal_add(pEvCtx, SIGINT, OnSignal, pEvCtx);
    MyHttpServer *pHttpd = new MyHttpServer;
    pHttpd->SetEventCtx(pEvCtx);
    pHttpd->BindAndListen("0.0.0.0", 1111, HTTP_SESSION);
    sw_ev_loop(pEvCtx);
    delete pHttpd;
    pHttpd = NULL;
    sw_ev_context_free(pEvCtx);
}

