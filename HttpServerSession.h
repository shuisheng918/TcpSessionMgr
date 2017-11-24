#pragma once

#include "TcpSession.h"
#include "HttpRequestDecoder.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

class HttpServerSession : public TcpSession
{
public:
    HttpServerSession() : m_keepAlive(false)
    {
        m_decoder.SetRequest(&m_request);
    }

    HttpRequest * GetHttpRequest() { return &m_request; }
    HttpResponse * GetHttpResponse() { return &m_response; }
    bool IsKeepAlive() { return m_keepAlive; }
    void SetKeepAlive(bool keepAlive) { m_keepAlive = keepAlive; }
    
    virtual void OnRecvData(const char *data, int len);
    virtual void ProcessHttpRequest();

    void SendResponse();
    
protected:
    HttpRequestDecoder m_decoder;
    HttpRequest  m_request;
    HttpResponse m_response;
    bool         m_keepAlive;
};