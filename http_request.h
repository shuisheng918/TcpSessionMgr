#pragma once

#include <string>
#include <map>

class HttpRequest
{
public:
    void Reset();
    void SetMethod(const std::string & method) { m_method = method; }
    const std::string & GetMethod() const { return m_method; }
    void SetURL(const std::string & url) { m_url = url; }
    const std::string & GetURL() const { return m_url; }
    void SetVersion(const std::string & ver) { m_httpVersion = ver; }
    const std::string & GetVersion() const { return m_httpVersion; }
    void SetPath(const std::string & path) { m_path = path; }
    const std::string & GetPath() const { return m_path; }
    void AddHeadField(const std::string & key, const std::string & value);
    void DeleteHeadField(const std::string & key);
    bool HasHeadField(const std::string & key) const;
    std::string GetHeadField(const std::string & key) const;
    std::map<std::string, std::string> & GetHeadFields() { return m_headFields; }
    void AddParams(const std::string & key, const std::string & value);
    void DeleteParams(const std::string & key);
    bool HasParams(const std::string & key) const;
    std::string GetParam(const std::string & key) const;
    std::map<std::string, std::string> & GetParams() { return m_params; }
    void SetHttpBody(const std::string & body);
    void AppendHttpBody(const std::string & data);
    void AppendHttpBody(const char * data, int len);
    const std::string & GetHttpBody() const { return m_body; }

protected:
    std::string m_method;
    std::string m_url;
    std::string m_httpVersion;
    std::string m_path;
    std::map<std::string, std::string> m_headFields; // key and value is lowercase
    std::map<std::string, std::string> m_params;
    std::string m_body;
};