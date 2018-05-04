#include "http_response.h"

using namespace std;

const char * HttpResponse::GetReason(ERespCode respCode)
{
    switch (respCode)
    {
        // 1xx
        case HTTP_CONTINUE: return "Continue";
        case HTTP_SWITCH_PROTOCOL: return "Switching Protocols";

        // 2xx
        case HTTP_OK: return "OK";
        case HTTP_CREATED: return "Created";
        case HTTP_ACCEPTED: return "Accepted";
        case HTTP_NON_AUTH_INFO: return "Non-Authoritative Information";
        case HTTP_NO_CONTENT: return "No Content";
        case HTTP_RESET_CONTENT: return "Reset Content";
        case HTTP_PARTIAL_CONTENT: return "Partial Content";

        // 3xx
        case HTTP_MULTI_CHOICE: return "Multiple Choices";
        case HTTP_MOVED_PERMANENTLY: return "Moved Permanently";
        case HTTP_FOUND: return "Found";
        case HTTP_SEE_OTHER: return "See Other";
        case HTTP_NOT_MODIFIED: return "Not Modified";
        case HTTP_USE_PROXY: return "Use Proxy";
        case HTTP_UNUSED: return "Unused";
        case HTTP_TEMP_REDIRECT: return "Temporary Redirect";

        // 4xx
        case HTTP_BAD_REQUEST: return "Bad Request";
        case HTTP_UNAUTHORIZED: return "Unauthorized";
        case HTTP_PAYMENT_REQUIRED: return "Payment Required";
        case HTTP_FORBIDDEN: return "Forbidden";
        case HTTP_NOT_FOUND: return "Not Found";
        case HTTP_METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case HTTP_NOT_ACCEPTABLE: return "Not Acceptable";
        case HTTP_PROXY_AUTH_REQUIRED: return "Proxy Authentication Required";
        case HTTP_REQUEST_TIMEOUT: return "Request Timeout";
        case HTTP_CONFLICT: return "Conflict";
        case HTTP_GONE: return "Gone";
        case HTTP_LENGTH_REQUIRED: return "Length Required";
        case HTTP_PRECONDITION_FAILED: return "Precondition Failed";
        case HTTP_REQUEST_ENTITY_TOO_LARGE: return "Request Entity Too Large";
        case HTTP_REQUEST_URI_TOO_LONG: return "Request-URI Too Long";
        case HTTP_UNSUPPORT_MEDIA_TYPE: return "Unsupported Media Type";
        case HTTP_REQUEST_RANGE_NOT_SATISFY: return "Requested Range Not Satisfiable";
        case HTTP_EXPECTATION_FAILED: return "Expectation Failed";

        // 5xx
        case HTTP_INTERNAL_SERVER_ERROR: return "Internal Server Error";
        case HTTP_NOT_IMPLEMENTED: return "Not Implemented";
        case HTTP_BAD_GATEWAY: return "Bad Gateway";
        case HTTP_SERVICE_UNAVAILABLE: return "Service Unavailable";
        case HTTP_GATEWAY_TIMEOUT: return "Gateway Timeout";
        case HTTP_VERSION_NOT_SUPPORT: return "HTTP Version Not Supported";

        default: return "Unknown";
    }
}

void HttpResponse::Reset()
{
    m_httpVersion.clear();
    m_responseCode = 0;
    m_reason.clear();
    m_headFields.clear();
    m_body.clear();

}

void HttpResponse::AddHeadField(const string & key, const string & value)
{
    if (!key.empty())
    {
        m_headFields[key] = value;
    }
}

void HttpResponse::DeleteHeadField(const string & key)
{
    m_headFields.erase(key);
}

bool HttpResponse::HasHeadField(const std::string & key) const
{
    return m_headFields.find(key) != m_headFields.end();
}

std::string HttpResponse::GetHeadField(const std::string & key) const
{
    auto it = m_headFields.find(key);
    if (it != m_headFields.end())
    {
        return it->second;
    }
    return "";
}

void HttpResponse::SetHttpBody(const string & body)
{
    m_body = body;
}

void HttpResponse::AppendHttpBody(const string & data)
{
    m_body.append(data);
}

void HttpResponse::AppendHttpBody(const char * data, int len)
{
    if (data != NULL && len > 0)
    {
        m_body.append(data, len);
    }
}


