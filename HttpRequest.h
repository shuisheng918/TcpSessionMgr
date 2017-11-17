#pragma once

#include <string>
#include <map>

class HttpSession;

class HttpRequest
{
public:
    HttpRequest() : m_pHttpSession(NULL) {      }

    void Reset()
    {
        m_method.clear();
        m_path.clear();
        m_httpVersion.clear();
        m_headFields.clear();
        m_prams.clear();
        m_body.clear();
    }

    HttpSession * m_pHttpSession;
    std::string m_method;
    std::string m_path;
    std::string m_httpVersion;
    std::string m_relativePath;
    std::map<std::string, std::string> m_headFields;
    std::map<std::string, std::string> m_prams;
    std::string m_body;
};