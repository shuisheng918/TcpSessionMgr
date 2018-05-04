#pragma once

#include <string>
#include <map>

class HttpResponse;

class HttpResponseDecoder
{
public:
    enum EHttpResponseDecodeStatus
    {
        STATUS_LINE,
        HEAD_FIELDS,
        BODY,
        DONE,
        ERROR
    };
public:
    HttpResponseDecoder() : m_status(STATUS_LINE), m_pResponse(NULL) {   }

    void SetResponse(HttpResponse *pResponse) { m_pResponse = pResponse; }
    void AppendData(const char *pData, int len);
    /**
     * return
     *     0    OK, http response parse ok.
     *    -1    error, http message illegal.
     *     1    http message data not enough.
     */
    int  Parse();
    
private:
    int  ParseStatusLine();
    int  ParseHeadFields();
    int  ParseBody();
    
    EHttpResponseDecodeStatus  m_status;  // decode status
    std::string                m_recvBuf;    

    HttpResponse  * m_pResponse;  // output
};