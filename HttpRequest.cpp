#include "HttpRequest.h"

using namespace std;

void HttpRequest::Reset()
{
    m_method.clear();
    m_path.clear();
    m_httpVersion.clear();
    m_headFields.clear();
    m_params.clear();
    m_body.clear();
}

void HttpRequest::AddHeadField(const string & key, const string & value)
{
    if (!key.empty())
    {
        m_headFields[key] = value;
    }
}

void HttpRequest::DeleteHeadField(const string & key)
{
    m_headFields.erase(key);
}

bool HttpRequest::HasHeadField(const string & key) const
{
    return m_headFields.find(key) != m_headFields.end();
}

string HttpRequest::GetHeadField(const string & key) const
{
    auto it = m_headFields.find(key);
    if (it != m_headFields.end())
    {
        return it->second;
    }
    return "";
}

void HttpRequest::AddParams(const string & key, const string & value)
{
    if (!key.empty())
    {
        m_params[key] = value;
    }
}

void HttpRequest::DeleteParams(const string & key)
{
    m_params.erase(key);
}

bool HttpRequest::HasParams(const string & key) const
{
    return m_params.find(key) != m_params.end();
}

string HttpRequest::GetParam(const string & key) const
{
    auto it = m_params.find(key);
    if (it != m_params.end())
    {
        return it->second;
    }
    return "";
}

void HttpRequest::SetHttpBody(const string & body)
{
    m_body = body;
}

void HttpRequest::AppendHttpBody(const string & data)
{
    m_body.append(data);
}

void HttpRequest::AppendHttpBody(const char * data, int len)
{
    if (data != NULL && len > 0)
    {
        m_body.append(data, len);
    }
}


