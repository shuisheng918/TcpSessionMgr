#include "TcpSessionMgr.h"

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

int main(void)
{
    struct sw_ev_context * pEvCtx = sw_ev_context_new();
    MyHttpServer httpd;
    httpd.SetEventCtx(pEvCtx);
    httpd.BindAndListen("0.0.0.0", 1111, HTTP_SESSION);
    sw_ev_loop(pEvCtx);
    sw_ev_context_free(pEvCtx);
}

