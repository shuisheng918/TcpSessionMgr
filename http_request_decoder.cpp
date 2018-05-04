#include "http_request_decoder.h"
#include "utils.h"
#include "http_request.h"

using namespace std;

#define MAX_REQ_LINE_LENGTH       (512)
#define MAX_REQ_HEAD_FIELD_LENGTH (4096)
#define MAX_REQ_BODY_LENGTH       (50*1024*1024)



void HttpRequestDecoder::AppendData(const char *pData, int len)
{
    m_recvBuf.append(pData, len);
}

/**
 * return
 *     0    OK, http request parse ok.
 *    -1    error, http message illegal.
 *     1    http message data not enough.
 */
int HttpRequestDecoder::Parse()
{
    if (m_status == DONE)
    {
        m_pRequest->Reset();
        m_status = REQ_LINE;
    }
    int res = -1;
    if (m_status == REQ_LINE)
    {
        res = ParseRequestLine();
        if (res)
        {
            return res;
        }
    }
    if (m_status == HEAD_FIELDS)
    {
        res = ParseHeadFields();
        if (res)
        {
            return res;
        }
    }
    if (m_status == BODY)
    {
        res = ParseBody();
        if (res)
        {
            return res;
        }
    }
    return 0;
}

/**
 * return
 *     0    OK, http request line parse ok.
 *    -1    error, http message illegal.
 *     1    request line data not enough.
 */
int HttpRequestDecoder::ParseRequestLine()
{
    int posCRLF = -1;
    posCRLF = m_recvBuf.find("\r\n");
    if (posCRLF != -1)
    {
        int sp1Pos = m_recvBuf.find(' ');
        string method;
        if (sp1Pos != -1)
        {
            method = m_recvBuf.substr(0, sp1Pos);
            m_pRequest->SetMethod(method);
        }
        else
        {
            goto error;
        }
        int sp2Pos = m_recvBuf.find(' ', sp1Pos + 1);
        if (sp2Pos != -1)
        {
            string url = m_recvBuf.substr(sp1Pos + 1, sp2Pos - (sp1Pos + 1));
            m_pRequest->SetURL(url);
            m_pRequest->SetVersion(m_recvBuf.substr(sp2Pos + 1, posCRLF - (sp2Pos + 1)));
            int questionMarkPos = url.find('?');
            if (questionMarkPos != -1)
            {
                m_pRequest->SetPath(url.substr(0, questionMarkPos));
                string paramStr = UrlDecode(url.substr(questionMarkPos + 1));
                ParseHttpParameters(paramStr, m_pRequest->GetParams());
            }
            else
            {
                m_pRequest->SetPath(url);
            }
        }
        else
        {
            goto error;
        }

        if (method != "GET" && method != "POST"
            && method != "PUT" && method != "HEAD"
            && method != "TRACE" && method != "DELETE"
            && method != "CONNECT" && method != "OPTIONS")
        {
            goto error;
        }
        if (m_pRequest->GetVersion() != "HTTP/1.0" && m_pRequest->GetVersion() != "HTTP/1.1")
        {
            goto error;
        }
        m_status = HEAD_FIELDS;
        m_recvBuf.erase(0, posCRLF + 2);
        return 0;
    }
    else if (m_recvBuf.size() <= MAX_REQ_LINE_LENGTH)
    {
        return 1;
    }
error:
    m_status = ERROR;
    m_recvBuf.clear();
    return -1;
    
}



/**
 * return
 *     0    OK, head fields parse ok.
 *    -1    error, http message illegal.
 *     1    head fields data not enough.
 */
int HttpRequestDecoder::ParseHeadFields()
{
    int posHeadField = 0;
    int posCRLF = -1;
    int len = (int)m_recvBuf.size();
    while (true)
    {
        posCRLF = m_recvBuf.find("\r\n", posHeadField);  
        if (posCRLF != -1)
        {
            if (posCRLF == posHeadField) // two "\r\n" together
            {
                m_status = BODY;
                m_recvBuf.erase(0, posCRLF + 2);
                return 0;
            }
            string headField = m_recvBuf.substr(posHeadField, posCRLF - posHeadField);
            int colonPos = headField.find(':');
            if (colonPos != -1)
            {
                string key = headField.substr(0, colonPos);
                string value = headField.substr(colonPos + 1);
                TrimString(key);
                TrimString(value);
                TolowerString(key);
                TolowerString(value);
                if (key.empty())
                {
                    goto error;
                }
                else
                {
                    m_pRequest->AddHeadField(key, value);
                }
            }
            else
            {
                goto error;
            }
            posHeadField = posCRLF + 2; // 2 is "\r\n" two bytes
            
        }
        else if(len - posHeadField <= MAX_REQ_HEAD_FIELD_LENGTH)
        {
            if (posHeadField > 0)
            {
                m_recvBuf.erase(0, posHeadField);
            }
            return 1;
        }
        else
        {
            goto error;
        }
    }
error:
    m_status = ERROR;
    m_recvBuf.clear();
    return -1;
}

/**
 * return
 *     0    OK, body parse ok.
 *    -1    error, http message illegal.
 *     1    body data not enough.
 */
int HttpRequestDecoder::ParseBody()
{
    int bodyLen = 0;
    string contentLength = m_pRequest->GetHeadField("content-length");
    if (!contentLength.empty())
    {
        bodyLen = atoi(contentLength.c_str());
    }
    if (bodyLen < 0 || bodyLen > MAX_REQ_BODY_LENGTH)  goto error;

    if ((int)m_recvBuf.size() >= bodyLen)
    {
        m_pRequest->AppendHttpBody(m_recvBuf.data(), bodyLen);
        m_recvBuf.erase(0, bodyLen);
        m_status = DONE;
        return 0;
    }
    else
    {
        return 1;
    }
error:
    m_status = ERROR;
    m_recvBuf.clear();
    return -1;

}


