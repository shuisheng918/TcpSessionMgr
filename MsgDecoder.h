#pragma once

#include <string>

class MsgDecoderBase
{
public:
    virtual void AppendData(const char *pData, int len) = 0;
    /*
     *  return:
     *      -1  error, such as illegal message, illegal length...
     *       0  success, have got a message.
     *       1  success, but message data is not enough.
     *  args:
     *      ppMsg    *ppMsg is data pointer have got.
     *      pMsgLen  *pMsgLen is the message's length.
     */
    virtual int  GetMessage(const char **ppMsg, int *pMsgLen) = 0;
};


/*
 *  message format:
 *  |--head(4B)--|------------body--------------|
 *  head is unsigned int(big endian), store the total length of message(include head self). 
 */
class DefaultMsgDecoder : public MsgDecoderBase
{
public:
    DefaultMsgDecoder() : m_nMaxMsgLen(4096), m_nPos(0) {         }

    virtual void AppendData(const char *pData, int len);
    virtual int  GetMessage(const char **ppMsg, int *pMsgLen);
    
private:
    unsigned int m_nMaxMsgLen;
    std::string m_data;
    unsigned int m_nPos;
};


