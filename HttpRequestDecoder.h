#pragma once

#include <string>
#include <map>

class HttpRequest;

class HttpRequestDecoder
{
public:
    enum EHttpRequestDecodeStatus
    {
        REQ_LINE,
        HEAD_FIELDS,
        BODY,
        DONE,
        ERROR
    };
public:
    HttpRequestDecoder() : m_status(REQ_LINE), m_pRequest(NULL) {   }

    void SetRequest(HttpRequest *pRequest) { m_pRequest = pRequest; }
    void AppendData(const char *pData, int len);
    /**
     * return
     *     0    OK, http request parse ok.
     *    -1    error, http message illegal.
     *     1    http message data not enough.
     */
    int  Parse();
    
private:
    int  ParseRequestLine();
    int  ParseHeadFields();
    int  ParseBody();
    
    EHttpRequestDecodeStatus  m_status;  // decode status
    std::string               m_recvBuf;    

    HttpRequest  * m_pRequest;  // output
};