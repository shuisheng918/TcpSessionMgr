#include "HttpSession.h"
#include "TcpSessionMgr.h"

void HttpSession::OnRead()
{
    int ret, httpParseRet;
    bool bNeedEndSession = false;
    char buf[4096];
    while (true)
    {
        ret = read(m_socket, buf, sizeof(buf));
        if (ret > 0)
        {
            m_decoder.AppendData(buf, ret);
            while ((httpParseRet = m_decoder.Parse()) == 0)
            {
                //m_decoder.
                m_pSessionMgr->ProcessHttpMessage(this);
            }
            if (httpParseRet == -1) // illegal http message
            {
                bNeedEndSession = true;
                break;
            }
        }
        else if (ret == 0)
        {// closed by peer host
            bNeedEndSession = true;
            break;
        }
        else if (ret < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            else
            {// socket occur exception
                bNeedEndSession = true;
                break;
            }
        }
    }

    if (bNeedEndSession)
    {
        m_pSessionMgr->EndSession(m_sessionId);
    }
    

}


