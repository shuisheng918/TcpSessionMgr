#pragma once


#include <string>
#include "TcpSession.h"

/*
 *  message format:
 *  |--head(4B)--|------------body--------------|
 *  head is unsigned int(big endian), store the total length of message(include head self). 
 */
class DefaultMsgDecoder
{
public:
    DefaultMsgDecoder() : m_nMaxMsgLen(4096), m_nPos(0) {         }

    void AppendData(const char *pData, int len);
    int  GetMessage(const char **ppMsg, int *pMsgLen);
    
private:
    unsigned int m_nMaxMsgLen;
    std::string m_data;
    unsigned int m_nPos;
};


class DefaultTcpSession : public TcpSession
{
public:
    virtual void OnRead();
protected:
    DefaultMsgDecoder m_decoder;
};
