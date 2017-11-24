#include "HttpServerSession.h"
#include <string.h>
#include "utils.h"

using namespace std;

void HttpServerSession::OnRecvData(const char *data, int len)
{
    int ret = 0;
    m_decoder.AppendData(data, len);
    while ((ret = m_decoder.Parse()) == 0)
    {
        m_response.SetVersion(m_request.GetVersion());
        string contentType = m_request.GetHeadField("content-type");
        if (!contentType.empty())
        {
            if (contentType == "application/x-www-form-urlencoded") // may be cover parameters in uri path
            {
                ParseHttpParameters(UrlDecode(m_request.GetHttpBody()), m_request.GetParams());
            }
        }
        string connection = m_request.GetHeadField("connection");
        if (!connection.empty())
        {
            if (connection == "keep-alive")
            {
                SetKeepAlive(true);
            }
        }
        ProcessHttpRequest();
    }
    if (ret == -1) // illegal http message
    {
        Close();
    }
}

void HttpServerSession::ProcessHttpRequest()
{
    m_response.Reset();
    m_response.SetRespCode(HttpResponse::HTTP_OK);
    m_response.AddHeadField("content-type", "text/html");
    //m_response.AppendHttpBody("<head><title>HELLO</title></head><body><h2>Hello world!</h2></body>\n");
    m_response.AppendHttpBody("<head><title>HELLO</title></head><body>");
    char buf[100];
    for (int i = 0; i < 10; ++i)
    {
        snprintf(buf, sizeof(buf), "Hello %d!!!<br>\n", i);
        m_response.AppendHttpBody(buf, strlen(buf));
    }
    m_response.AppendHttpBody("</body>\n");
    SendResponse();
}

void HttpServerSession::SendResponse()
{
    char codeBuf[10], contentLenBuf[20];
    snprintf(codeBuf, sizeof(codeBuf), "%d", m_response.GetRespCode());
    string header = m_response.GetVersion() + " " + codeBuf + " "
                    + HttpResponse::GetReason((HttpResponse::ERespCode)m_response.GetRespCode()) + "\r\n";
    snprintf(contentLenBuf, sizeof(contentLenBuf), "%lu", m_response.GetHttpBody().size());
    if (IsKeepAlive())
    {
        m_response.AddHeadField("connection", "keep-alive");
    }
    else
    {
        m_response.AddHeadField("connection", "close");
    }
    m_response.AddHeadField("content-length", contentLenBuf);
    const map<string, string> & headFields = m_response.GetHeadFields();
    for (auto it = headFields.begin(); it != headFields.end(); ++it)
    {
        header += it->first;
        header += ": ";
        header += it->second;
        header += "\r\n";
    }
    header += "\r\n";
    header += m_response.GetHttpBody();
    SendData(header.data(), header.size());
    if (!IsKeepAlive())
    {
        SetSentClose(true);
    }
}


