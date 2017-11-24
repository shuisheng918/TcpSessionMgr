#include "HttpResponseDecoder.h"
#include "utils.h"
#include "HttpResponse.h"

using namespace std;

#define MAX_STATUS_LINE_LENGTH     (512)
#define MAX_RESP_HEAD_FIELD_LENGTH (4096)
#define MAX_RESP_BODY_LENGTH       (50*1024*1024)



void HttpResponseDecoder::AppendData(const char *pData, int len)
{
    m_recvBuf.append(pData, len);
}

/**
 * return
 *     0    OK, http response parse ok.
 *    -1    error, http message illegal.
 *     1    http message data not enough.
 */
int HttpResponseDecoder::Parse()
{
    if (m_status == DONE)
    {
        m_pResponse->Reset();
        m_status = STATUS_LINE;
    }
    int res = -1;
    if (m_status == STATUS_LINE)
    {
        res = ParseStatusLine();
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
 *     0    OK, http status line parse ok.
 *    -1    error, http message illegal.
 *     1    request line data not enough.
 */
int HttpResponseDecoder::ParseStatusLine()
{
    int posCRLF = -1;
    posCRLF = m_recvBuf.find("\r\n");
    if (posCRLF != -1)
    {
        int sp1Pos = m_recvBuf.find(' ');
        if (sp1Pos != -1)
        {
            m_pResponse->SetVersion(m_recvBuf.substr(0, sp1Pos));
        }
        else
        {
            goto error;
        }
        int sp2Pos = m_recvBuf.find(' ', sp1Pos + 1);
        if (sp2Pos != -1)
        {
            string respCodeStr = m_recvBuf.substr(sp1Pos + 1, sp2Pos - (sp1Pos + 1));
            m_pResponse->SetRespCode((HttpResponse::ERespCode)atoi(respCodeStr.c_str()));
            m_pResponse->SetReason(m_recvBuf.substr(sp2Pos + 1, posCRLF - (sp2Pos + 1)));
        }
        else
        {
            goto error;
        }

        if (m_pResponse->GetVersion() != "HTTP/1.0" && m_pResponse->GetVersion() != "HTTP/1.1")
        {
            goto error;
        }
        m_status = HEAD_FIELDS;
        m_recvBuf.erase(0, posCRLF + 2);
        return 0;
    }
    else if (m_recvBuf.size() <= MAX_STATUS_LINE_LENGTH)
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
int HttpResponseDecoder::ParseHeadFields()
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
                    m_pResponse->AddHeadField(key, value);
                }
            }
            else
            {
                goto error;
            }
            posHeadField = posCRLF + 2; // 2 is "\r\n" two bytes
            
        }
        else if(len - posHeadField <= MAX_RESP_HEAD_FIELD_LENGTH)
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
int HttpResponseDecoder::ParseBody()
{
    int bodyLen = 0;
    string contentLength = m_pResponse->GetHeadField("content-length");
    if (!contentLength.empty())
    {
        bodyLen = atoi(contentLength.c_str());
    }
    if (bodyLen < 0 || bodyLen > MAX_RESP_BODY_LENGTH)  goto error;

    if ((int)m_recvBuf.size() >= bodyLen)
    {
        m_pResponse->AppendHttpBody(m_recvBuf.data(), bodyLen);
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



