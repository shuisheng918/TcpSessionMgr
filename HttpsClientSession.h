#pragma once

#ifdef ENABLE_SSL

#include "SslSession.h"
#include "HttpResponseDecoder.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

class HttpsClientSession : public SslSession
{
public:
    HttpsClientSession() : m_keepAlive(false)
    {
        m_decoder.SetResponse(&m_response);
        m_request.SetVersion("HTTP/1.1");
    }
    HttpRequest * GetHttpRequest() { return &m_request; }
    HttpResponse * GetHttpResponse() { return &m_response; }
    bool IsKeepAlive() { return m_keepAlive; }
    void SetKeepAlive(bool keepAlive) { m_keepAlive = keepAlive; }
    
    virtual void OnRecvData(const char *data, int len);
    virtual void ProcessHttpResponse();

    void SendRequest();
    
protected:
    HttpResponseDecoder m_decoder;
    HttpRequest  m_request;
    HttpResponse m_response;
    std::string  m_host;
    std::string  m_port;
    bool         m_keepAlive;
};

#endif