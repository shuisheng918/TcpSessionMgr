#pragma once

#include "tcp_session.h"
#include "http_request_decoder.h"
#include "http_request.h"
#include "http_response.h"

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

    /**
     * chunked data response
     */
    void SendResponseStart();
    void SendChunk(const char * pChunk, int len);
    void SendResponseEnd();
    
protected:
    HttpRequestDecoder m_decoder;
    HttpRequest  m_request;
    HttpResponse m_response;
    bool         m_keepAlive;
    std::string  m_tempChunkData;
};