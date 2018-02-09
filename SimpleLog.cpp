#include "SimpleLog.h"
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <vector>
#include <utility>
#include <chrono>

#define MAX_LOG_LENGTH   1024 * 4

using namespace std;


SimpleLog * SimpleLog::m_pInst = NULL;

SimpleLog::~SimpleLog()
{
    Shutdown();
}

bool SimpleLog::Startup(const std::string  & logPath, unsigned int delay, unsigned int rotateCycle)
{
    if (m_startup)
    {
        Shutdown();
    }
    
    m_logPath = logPath;
    if (m_logPath == "")
    {
        m_pFile = stdout;
    }
    else
    {
        m_pFile = fopen(m_logPath.c_str(), "ab");
    }
    if (m_pFile == NULL)
    {
        printf("open log file %s failed.", logPath.c_str());
        return false;
    }
    m_delay = delay;
    m_rotateCycle = rotateCycle;
    m_startup = true;
    m_logThread = thread(&SimpleLog::WriteLogThread, this);
    return true;
}

void SimpleLog::Shutdown()
{
    m_startup = false;
    if (m_logThread.joinable())
    {
        m_logThread.join();
        if (m_pFile != stdout)
        {
            fflush(m_pFile);
            fclose(m_pFile);
        }
        m_pFile = NULL;
    }
}

void SimpleLog::Debug(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    _Log(" DEBUG ", format, args);
    va_end(args);
}

void SimpleLog::Info(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    _Log(" INFO  ", format, args);
    va_end(args);
}

void SimpleLog::Warn(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    _Log(" WARN  ", format, args);
    va_end(args);
}

void SimpleLog::Error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    _Log(" ERROR ", format, args);
    va_end(args);
}

void SimpleLog::WriteLogThread(void)
{
    vector<pair<const char*, int> > tempLogs;
    while(1)
    {
        tempLogs.clear();
        unique_lock<mutex> lck(m_lock);
        if (m_logQue.empty())
        {
            if (!m_startup)
            {
                break;
            }
            m_cond.wait_for(lck, chrono::seconds(1));
        }
        while (!m_logQue.empty())
        {
            tempLogs.push_back(m_logQue.front());
            m_logQue.pop();
        }
        lck.unlock();
        if (m_pFile)
        {
            for (size_t i = 0; i < tempLogs.size(); ++i)
            {
                 fwrite(tempLogs[i].first, tempLogs[i].second, 1, m_pFile);
                 delete [] tempLogs[i].first;
            }
            _Check();
        }
        
    }
}

void SimpleLog::_GetTimeStr(std::string &timeStr)
{
    time_t rawtime = time(NULL);
    struct tm timeinfo;
    localtime_r(&rawtime, &timeinfo);
    char buf[128];
    snprintf(buf, sizeof(buf), 
             "%4d-%02d-%02d %02d:%02d:%02d P%d", 
             1900 + timeinfo.tm_year,
             timeinfo.tm_mon + 1,
             timeinfo.tm_mday,
             timeinfo.tm_hour,
             timeinfo.tm_min,
             timeinfo.tm_sec,
             getpid());
    timeStr = buf;
}

void SimpleLog::_GetTimeStr2(std::string &timeStr)
{
    time_t rawtime = time(NULL);
    struct tm timeinfo;
    localtime_r(&rawtime, &timeinfo);
    char buf[128];
    snprintf(buf, sizeof(buf), 
             "%4d-%02d-%02dT%02d-%02d-%02d", 
             1900 + timeinfo.tm_year,
             timeinfo.tm_mon + 1,
             timeinfo.tm_mday,
             timeinfo.tm_hour,
             timeinfo.tm_min,
             timeinfo.tm_sec);
    timeStr = buf;
}

void SimpleLog::_Log(const char * pLogLevelStr, const char * format, va_list args)
{
    char szBuffer[MAX_LOG_LENGTH];
    string logHead;
    _GetTimeStr(logHead);
    logHead.append(pLogLevelStr);
    assert(logHead.size() + 1 < sizeof(szBuffer));
    memcpy(szBuffer, logHead.data(), logHead.size());
    int ret = vsnprintf(szBuffer + logHead.size(), sizeof(szBuffer) - logHead.size(), format, args);
    if (ret >= 0)
    {
        if (ret >= (int)(sizeof(szBuffer) - logHead.size()))
        {
            ret = sizeof(szBuffer) - logHead.size() - 1;
        }
        szBuffer[logHead.size() + ret] = '\n';
        _AppendLogQueue(szBuffer, logHead.size() + ret + 1);
    }

}

void SimpleLog::_AppendLogQueue(const char * pLogMsg, int len)
{
    if (len <= 0)    return;
    char *pNew = new char [len];
    memcpy(pNew, pLogMsg, len);
    
    unique_lock<mutex> lck(m_lock);
    m_logQue.push(make_pair(pNew, len));
    lck.unlock();
    m_cond.notify_all();
}


void SimpleLog::_Check()
{
    time_t now = time(NULL);
    if (m_delay == 0 || now - m_lastFlushTime >= m_delay)
    {
        fflush(m_pFile);
        m_lastFlushTime = now;
    }
    if (m_rotateCycle != 0 && now - m_lastRotateLogTime >= m_rotateCycle)
    {
        if (m_lastRotateLogTime != 0) // first startup program don't rotate log
        {
            _LogRotate();
        }
        m_lastRotateLogTime = now;
    }
}

int SimpleLog::_LogRotate()
{
    int ret = 0;

    if (m_pFile != NULL && m_pFile != stdout)
    {
        fflush(m_pFile);
        fclose(m_pFile);  
        m_pFile = NULL;
        string timeStr;
        _GetTimeStr2(timeStr);
        string backName = m_logPath + "_" + timeStr;
        ret = rename(m_logPath.c_str(), backName.c_str());
        m_pFile = fopen(m_logPath.c_str(), "ab");
        if (m_pFile == NULL)
        {
            ret = errno;
        }
    }
    return ret;
}


