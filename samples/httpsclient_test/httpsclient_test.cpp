#include <signal.h>
#include <stdio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "TcpSessionMgr.h"
#include "HttpsClientSession.h"

SSL_CTX * g_pSSLCtx;

class MyHttpsClient : public TcpSessionManager
{
public:
    virtual void OnSessionHasBegin(TcpSession *pSession)
    {
        //printf("OnSessionHasBegin,sid=%lu\n", pSession->GetSessionID());
        if (pSession->GetSessionType() == HTTPS_CLIENT_SESSION)
        {
            HttpsClientSession *pHttpsSession = dynamic_cast<HttpsClientSession*>(pSession);
            pHttpsSession->Init(g_pSSLCtx, false);
            HttpRequest *pRequest = pHttpsSession->GetHttpRequest();
            pRequest->SetMethod("GET");
            pRequest->SetURL("/");
            pHttpsSession->SetKeepAlive(true);
            for (int i = 0; i < 10; ++i)
            {
                pHttpsSession->SendRequest();
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
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    g_pSSLCtx = SSL_CTX_new(SSLv23_client_method());
    if (g_pSSLCtx == NULL)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    
    sw_ev_context_t * pEvCtx = sw_ev_context_new();
    sw_set_log_func(OnLog);
    sw_ev_signal_add(pEvCtx, SIGINT, OnSignal, pEvCtx);
    sw_ev_signal_add(pEvCtx, SIGTERM, OnSignal, pEvCtx);
    sw_ev_signal_add(pEvCtx, SIGPIPE, OnSignal, pEvCtx);
    MyHttpsClient *pHttpsClient = new MyHttpsClient;
    pHttpsClient->SetEventCtx(pEvCtx);
    for (int i = 0; i < 1; ++i)
    {
        pHttpsClient->Connect("localhost", 443, HTTPS_CLIENT_SESSION);
    }
    sw_ev_loop(pEvCtx);
    delete pHttpsClient;
    pHttpsClient = NULL;
    sw_ev_context_free(pEvCtx);
    SSL_CTX_free(g_pSSLCtx);
}

