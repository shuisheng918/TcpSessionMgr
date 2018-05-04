#include "http_client_session.h"
#include <string.h>
#include "utils.h"
#include "tcp_session_mgr.h"


using namespace std;

void HttpClientSession::OnRecvData(const char *data, int len)
{
    int ret = 0;
    m_decoder.AppendData(data, len);
    while ((ret = m_decoder.Parse()) == 0)
    {
        string connection = m_request.GetHeadField("connection");
        if (!connection.empty())
        {
            if (connection == "keep-alive")
            {
                SetKeepAlive(true);
            }
        }
        ProcessHttpResponse();
        if (!IsKeepAlive())
        {
            Close();
            return;
        }
    }
    if (ret == -1) // illegal http message
    {
        Close();
    }
}

void HttpClientSession::ProcessHttpResponse()
{
    log("recv http response:");
    log("%s", m_response.GetHttpBody().c_str());
}

void HttpClientSession::SendRequest()
{
    char hostBuf[64], contentLenBuf[20];

    string header = m_request.GetMethod() + " " + m_request.GetURL() + " "
                    + m_request.GetVersion() + "\r\n";
                    
    snprintf(hostBuf, sizeof(hostBuf), 
            "%d.%d.%d.%d:%d", 
            (unsigned)m_peerIp >> 24, 
            ((unsigned)m_peerIp & 0x00ff0000) >> 16,
            ((unsigned)m_peerIp & 0x0000ff00) >> 8,
            (unsigned)m_peerIp & 0x000000ff,
            m_peerPort);
    m_response.AddHeadField("host", hostBuf);
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
}



