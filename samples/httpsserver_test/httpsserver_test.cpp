#include <signal.h>
#include <stdio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "TcpSessionMgr.h"
#include "HttpsServerSession.h"

SSL_CTX * g_pSSLCtx;

class MyHttpsServer : public TcpSessionManager
{
public:
    virtual void OnSessionHasBegin(TcpSession *pSession)
    {
        printf("OnSessionHasBegin,sid=%lu\n", pSession->GetSessionID());
        if (pSession->GetSessionType() == HTTPS_SERVER_SESSION)
        {
            ((HttpsServerSession*)pSession)->Init(g_pSSLCtx);
        }
    }
    virtual void OnSessionWillEnd(TcpSession *pSession)
    {
        printf("OnSessionWillEnd,sid=%lu\n", pSession->GetSessionID());
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
    g_pSSLCtx = SSL_CTX_new(SSLv23_server_method());
    if (g_pSSLCtx == NULL)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    if (SSL_CTX_use_certificate_file(g_pSSLCtx, "./cert.pem", SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    if (SSL_CTX_use_PrivateKey_file(g_pSSLCtx, "./key.pem", SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    if (!SSL_CTX_check_private_key(g_pSSLCtx))
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }

    sw_ev_context_t * pEvCtx = sw_ev_context_new();
    sw_set_log_func(OnLog);
    sw_ev_signal_add(pEvCtx, SIGINT, OnSignal, pEvCtx);
    sw_ev_signal_add(pEvCtx, SIGTERM, OnSignal, pEvCtx);
    sw_ev_signal_add(pEvCtx, SIGPIPE, OnSignal, pEvCtx);
    MyHttpsServer *pServer = new MyHttpsServer;
    pServer->SetEventCtx(pEvCtx);
    pServer->BindAndListen("0.0.0.0", 443, HTTPS_SERVER_SESSION);
    sw_ev_loop(pEvCtx);
    delete pServer;
    pServer = NULL;
    sw_ev_context_free(pEvCtx);
    SSL_CTX_free(g_pSSLCtx);
}

