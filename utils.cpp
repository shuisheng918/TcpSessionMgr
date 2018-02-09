#include "utils.h"
#include <ctype.h>

using namespace std;

uint64_t htonll(uint64_t val)
{
    return (((uint64_t) htonl(val)) << 32) + htonl(val >> 32);
}

uint64_t ntohll(uint64_t val)
{
    return (((uint64_t) ntohl(val)) << 32) + ntohl(val >> 32);
}

void TrimString(string & str)
{
    int len = (int)str.size();
    if (len <= 0)   return;
    int i = 0;
    for (; i < len; ++i)
    {
        if (str[i] != ' ' && str[i] != '\t' && str[i] != '\n' && str[i] != '\r')
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
        if (str[i] != ' ' && str[i] != '\t' && str[i] != '\n' && str[i] != '\r')
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


/**
 * paramStr like name=ZhangSan&phone=18888888888&age=30
 */
void ParseHttpParameters(const string & paramStr, map<string, string> & params)
{
    int curPos = 0, tmpAndPos = 0;
    int len = (int)paramStr.size();
    string field, key, value;
    while (curPos >= 0 && curPos + 1 < len)
    {   
        tmpAndPos = paramStr.find('&', curPos);
        if (tmpAndPos != -1) 
        {
            field = paramStr.substr(curPos, tmpAndPos - curPos);
        }
        else
        {
            field = paramStr.substr(curPos);
        }
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
                    params[key] = value;
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


