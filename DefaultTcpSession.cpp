#include "DefaultTcpSession.h"
#include "TcpSessionMgr.h"

using namespace std;

void DefaultMsgDecoder::AppendData(const char *pData, int len)
{
    if (pData && len > 0)
    {
        if (m_nPos > 0)
        {
            m_data.erase(0, m_nPos);
            m_nPos = 0;
        }
        m_data.append(pData, len);
    }
}

int  DefaultMsgDecoder::GetMessage(const char **ppMsg, int *pMsgLen)
{
    if (m_nPos > 0)
    {
        m_data.erase(0, m_nPos);
        m_nPos = 0;
    }
    uint32_t nTotalLen = (uint32_t)m_data.size();
    if (nTotalLen >= sizeof(uint32_t))
    {
        uint32_t nMsgLen = ((uint8_t)m_data[0] << 24)
                          | ((uint8_t)m_data[1] << 16)
                          | ((uint8_t)m_data[2] << 8)
                          | ((uint8_t)m_data[3]);
        if (nMsgLen < 4 || nMsgLen > m_nMaxMsgLen)
        {
            return -1;
        }
        if (nTotalLen >=  nMsgLen)
        {
            *ppMsg = m_data.data();
            *pMsgLen = nMsgLen;
            m_nPos = nMsgLen;
            return 0;
        }
        else
        {
            return 1;
        }
    }
    else
    {
        return 1;
    }
}

void DefaultTcpSession::OnRecvData(const char *data, int len)
{
    m_decoder.AppendData(data, len);

    const char *pMsg = NULL;
    int msgLen = 0, ret;
    while ((ret = m_decoder.GetMessage(&pMsg, &msgLen)) == 0)
    {
        if (pMsg != NULL && msgLen > 0)
        {
            ProcessMessage(pMsg, msgLen);
        }
    }
    if (ret == -1)
    {
        Close();
    }
}



