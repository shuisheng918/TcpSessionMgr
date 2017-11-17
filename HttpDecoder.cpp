#include "HttpDecoder.h"
#include <ctype.h>
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "HttpSession.h"

using namespace std;

#define MAX_REQ_LINE_LENGTH   (512)
#define MAX_HEAD_FIELD_LENGTH (4096)
#define MAX_BODY_LENGTH       (1024*1024*1024)


void TrimString(string & str)
{
    int len = (int)str.size();
    if (len <= 0)   return;
    int i = 0;
    for (; i < len; ++i)
    {
        if (str[i] != ' ' && str[i] != '\t' && str[i] != '\n')
        {
            break;
        }
    }
    if (i > 0)
    {
        str.erase(0, i);
    }
    len = (int)str.size();
    for (i = len - 1; i >= 0; --i)
    {
        if (str[i] != ' ' && str[i] != '\t' && str[i] != '\n')
        {
            break;
        } 
    }
    if (i >= 0 && i + 1 < len)
    {
        str.erase(i + 1);
    }
}

void TolowerString(string & str)
{
    int len = (int)str.size();
    for (int i = 0; i < len; ++i)
    {
        str[i] = tolower(str[i]);
    }
}

string UrlDecode(const string & str)
{
    string res;
    int len = (int)str.size();
    for (int i = 0; i < len; ++i)
    {
        char c = str[i];
        if (c == '+')  // compatible for old style
        {
            res.push_back(' ');
        }
        else if (c == '%' && i + 2 < len && isdigit(str[i+1]) && isdigit(str[i+2]))
        {
            res.push_back(((unsigned char)(str[i+1] - '0') << 4) | ((unsigned char)(str[i+2] - '0')));
            i += 2;
        }
        else
        {
            res.push_back(c);
        }
    }
    return res;
}

string UrlEncode(const string & str)
{
    static char hex[16] = { '0', '1', '2', '3',
                            '4', '5', '6', '7',
                            '8', '9', 'A', 'B',
                            'C', 'D', 'E', 'F'  };
    string res;
    int len = (int)str.size();
    if (len > 0)
    {
        res.reserve(3*len);
    }
    for (int i = 0; i < len; ++i)
    {
        unsigned char c = str[i];
        if (c != '-' && c != '_' && c != '.' && c != '~')
        {
            res.push_back('%');
            res.push_back(hex[c >> 4]);
            res.push_back(hex[c & 0x0F]);
        }
        else
        {
            res.push_back((char)c);
        }
    }
    return res;
}



void HttpDecoder::AppendData(const char *pData, int len)
{
    m_recvBuf.append(pData, len);
}

/**
 * return
 *     0    OK, http request parse ok.
 *    -1    error, http message illegal.
 *     1    http message data not enough.
 */
int HttpDecoder::Parse()
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
        if (m_pRequest->m_method == "post")
        {
            auto it = m_pRequest->m_headFields.find("content-type");
            if (it != m_pRequest->m_headFields.end())
            {
                string contentType = it->second;
                TolowerString(contentType);
                if (contentType == "application/x-www-form-urlencoded")
                {
                    ParseParameters(UrlDecode(m_pRequest->m_body));
                }
                else if(contentType == "text/plain")
                {
                    ParseParameters(m_pRequest->m_body);
                }
            }
            it = m_pRequest->m_headFields.find("connection");
            if (it != m_pRequest->m_headFields.end())
            {
                string connection = it->second;
                TolowerString(connection);
                if (connection == "keep-alive")
                {
                    m_pRequest->m_pHttpSession->m_keepAlive = true;
                }
            }
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
int HttpDecoder::ParseRequestLine()
{
    int posCRLF = -1;
    posCRLF = m_recvBuf.find("\r\n");
    if (posCRLF != -1)
    {
        int sp1Pos = m_recvBuf.find(' ');
        if (sp1Pos != -1)
        {
            m_pRequest->m_method = m_recvBuf.substr(0, sp1Pos);
        }
        else
        {
            goto error;
        }
        int sp2Pos = m_recvBuf.find(' ', sp1Pos + 1);
        if (sp2Pos != -1)
        {
            m_pRequest->m_path = m_recvBuf.substr(sp1Pos + 1, sp2Pos - (sp1Pos + 1));
            m_pRequest->m_httpVersion = m_recvBuf.substr(sp2Pos + 1, posCRLF - (sp2Pos + 1));
            m_pResponse->m_httpVersion = m_pRequest->m_httpVersion;
            int questionMarkPos = m_pRequest->m_path.find('?');
            if (questionMarkPos != -1)
            {
                m_pRequest->m_relativePath = m_pRequest->m_path.substr(0, questionMarkPos);
                string paramStr = UrlDecode(m_pRequest->m_path.substr(questionMarkPos + 1));
                ParseParameters(paramStr);
            }
            else
            {
                m_pRequest->m_relativePath = m_pRequest->m_path;
            }
        }
        else
        {
            goto error;
        }
        TolowerString(m_pRequest->m_method);
        TolowerString(m_pRequest->m_httpVersion);
        if (m_pRequest->m_method != "get" && m_pRequest->m_method != "post"
            && m_pRequest->m_method != "put" && m_pRequest->m_method != "head"
            && m_pRequest->m_method != "trace" && m_pRequest->m_method != "delete"
            && m_pRequest->m_method != "connect" && m_pRequest->m_method != "options")
        {
            goto error;
        }
        if (m_pRequest->m_httpVersion != "http/1.0" && m_pRequest->m_httpVersion != "http/1.1"
            && m_pRequest->m_httpVersion != "http/2.0")
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

void HttpDecoder::ParseParameters(const string & paramStr)
{
    int curPos = 0, tmpAndPos = 0;
    int len = (int)paramStr.size();
    string field, key, value;
    while (curPos >= 0 && curPos + 1 < len)
    {   
        tmpAndPos = paramStr.find('&', curPos);
        if (tmpAndPos != -1) 
            field = paramStr.substr(curPos, tmpAndPos - curPos);
        else
            field = paramStr.substr(curPos);
        if (!field.empty())
        {   
            int equalPos = field.find('=');
            if (equalPos != -1) 
            {   
                key = field.substr(0, equalPos);
                value = field.substr(equalPos + 1); 
                TrimString(key);
                TrimString(value);
                if (!key.empty())
                {   
                    m_pRequest->m_prams[key] = value;
                }   
            }   
        }   
        if (tmpAndPos == -1) 
        {   
            break;
        }   
        curPos = tmpAndPos + 1;
    }   
}

/**
 * return
 *     0    OK, head fields parse ok.
 *    -1    error, http message illegal.
 *     1    head fields data not enough.
 */
int HttpDecoder::ParseHeadFields()
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
                if (key.empty())
                {
                    goto error;
                }
                else
                {
                    m_pRequest->m_headFields[key] = value;
                }
            }
            else
            {
                goto error;
            }
            posHeadField = posCRLF + 2; // 2 is "\r\n" two bytes
            
        }
        else if(len - posHeadField <= MAX_HEAD_FIELD_LENGTH)
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
int HttpDecoder::ParseBody()
{
    int bodyLen = 0;
    auto it = m_pRequest->m_headFields.find("content-length");
    if (it != m_pRequest->m_headFields.end())
    {
        bodyLen = atoi(it->second.c_str());
    }
    if (bodyLen < 0 || bodyLen > MAX_BODY_LENGTH)  goto error;

    if ((int)m_recvBuf.size() >= bodyLen)
    {
        m_pRequest->m_body.append(m_recvBuf.data(), bodyLen);
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


