#pragma once

#include "TcpSession.h"
#include "HttpDecoder.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

class HttpSession : public TcpSession
{
public:
    HttpSession() : m_keepAlive(false)
    {
        m_request.m_pHttpSession = this;
        m_response.m_pHttpSession = this;
        m_decoder.SetRequest(&m_request);
        m_decoder.SetResponse(&m_response);
    }
    virtual ~HttpSession() {       }
    
    virtual void OnRead();
public:
    HttpDecoder  m_decoder;
    HttpRequest  m_request;
    HttpResponse m_response;
    bool         m_keepAlive;
};