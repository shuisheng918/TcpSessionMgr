#pragma once

#include <stdio.h>
#include <string>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>

class SimpleLog
{
public:
    static SimpleLog * GetInstance()
    {
        if (m_pInst == NULL)
        {
            m_pInst = new SimpleLog;
        }
        return m_pInst;
    }

    /**
     * Before using it, call Startup to init.
     * @param  logPath       If it is "" logs will be print to stdout else print logs to stdout.
     * @param  delay         Flush to disk file interval, set to 0 present realtime flush disk file. Unit is second.
     * @param  rotateCycle   Rotate(rename old logfile) log cycle, set to 0 present not roate log. Unit is second.
     * @return  true - succes, false - failed
     */
    bool Startup(const std::string  & logPath, unsigned int delay, unsigned int rotateCycle);
    void Shutdown();

    /**
     *  format output log msgs, format string is like printf.
     */
    void Debug(const char* format, ...);
    void Info(const char* format, ...);
    void Warn(const char* format, ...);
    void Error(const char* format, ...);
    
private:
    SimpleLog()
    {
        m_startup = false;
        m_delay = 0;
        m_rotateCycle = 0;
        m_lastFlushTime = 0;
        m_lastRotateLogTime = 0;
        m_pFile = NULL;
    }
    ~SimpleLog();
    
    void WriteLogThread(void);     // thread function, just write log to file

    static void _GetTimeStr(std::string &timeStr);
    static void _GetTimeStr2(std::string &timeStr);
    void _Log(const char * pLogLevel, const char * format, va_list args);
    void _AppendLogQueue(const char * pLogMsg, int len);
    void _Check();
    /**
     * rename old logfile to xxxx_YYYY-MM-DDTHH-mm-SS
     * success return 0, else return errno
     */
    int  _LogRotate();
private:
    std::thread   m_logThread;
    std::mutex    m_lock;
    std::condition_variable m_cond;
    bool          m_startup;
    std::queue<std::pair<const char*, int> > m_logQue;

    std::string   m_logPath;
    int           m_delay;  // seconds
    int           m_rotateCycle; // seconds
    time_t        m_lastFlushTime;
    time_t        m_lastRotateLogTime;
    FILE         *m_pFile;

    static SimpleLog * m_pInst;
};


