#pragma once

#include <string>

class HttpRequest;
class HttpResponse;

class HttpDecoder
{
public:
    enum EHttpDecodeStatus
    {
        REQ_LINE,
        HEAD_FIELDS,
        BODY,
        DONE,
        ERROR
    };
public:
    HttpDecoder() : m_status(REQ_LINE), m_pRequest(NULL),    m_pResponse(NULL) {   }

    void SetRequest(HttpRequest *pRequest)
    {
        m_pRequest = pRequest;
    }
    void SetResponse(HttpResponse *pResponse)
    {
        m_pResponse = pResponse;
    }
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
    void ParseParameters(const std::string & str);
    int  ParseHeadFields();
    int  ParseBody();
    EHttpDecodeStatus m_status;  // decode status
    std::string   m_recvBuf;    

    HttpRequest  * m_pRequest;
    HttpResponse * m_pResponse;
};