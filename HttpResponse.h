#pragma once

#include <string>
#include <map>

class TcpSession;

class HttpResponse
{
public:
    enum ERespCode
    {
        // HTTP1.1  RFC2616 definition
        HTTP_CONTINUE = 100,
        HTTP_SWITCH_PROTOCOL = 101,

        HTTP_OK = 200,
        HTTP_CREATED = 201,
        HTTP_ACCEPTED = 202,
        HTTP_NON_AUTH_INFO = 203,
        HTTP_NO_CONTENT = 204,
        HTTP_RESET_CONTENT = 205,
        HTTP_PARTIAL_CONTENT = 206,

        HTTP_MULTI_CHOICE = 300,
        HTTP_MOVED_PERMANENTLY = 301,
        HTTP_FOUND = 302,
        HTTP_SEE_OTHER = 303,
        HTTP_NOT_MODIFIED = 304,
        HTTP_USE_PROXY = 305,
        HTTP_UNUSED = 306,
        HTTP_TEMP_REDIRECT = 307,

        HTTP_BAD_REQUEST = 400,
        HTTP_UNAUTHORIZED = 401,
        HTTP_PAYMENT_REQUIRED = 402,
        HTTP_FORBIDDEN = 403,
        HTTP_NOT_FOUND = 404,
        HTTP_METHOD_NOT_ALLOWED = 405,
        HTTP_NOT_ACCEPTABLE = 406,
        HTTP_PROXY_AUTH_REQUIRED = 407,
        HTTP_REQUEST_TIMEOUT = 408,
        HTTP_CONFLICT = 409,
        HTTP_GONE = 410,
        HTTP_LENGTH_REQUIRED = 411,
        HTTP_PRECONDITION_FAILED = 412,
        HTTP_REQUEST_ENTITY_TOO_LARGE = 413,
        HTTP_REQUEST_URI_TOO_LONG = 414,
        HTTP_UNSUPPORT_MEDIA_TYPE = 415,
        HTTP_REQUEST_RANGE_NOT_SATISFY = 416,
        HTTP_EXPECTATION_FAILED = 417,

        HTTP_INTERNAL_SERVER_ERROR = 500,
        HTTP_NOT_IMPLEMENTED = 501,
        HTTP_BAD_GATEWAY = 502,
        HTTP_SERVICE_UNAVAILABLE = 503,
        HTTP_GATEWAY_TIMEOUT = 504,
        HTTP_VERSION_NOT_SUPPORT = 505
    };
    
public:
    void Reset();
    void SetVersion(const std::string & ver) { m_httpVersion = ver; }
    const std::string & GetVersion() const { return m_httpVersion; }
    void SetRespCode(ERespCode code) { m_responseCode = code; }
    int  GetRespCode() const { return m_responseCode; }
    void SetReason(const std::string & reason) { m_reason = reason; }
    const std::string & GetReason() const { return m_reason; }
    void AddHeadField(const std::string & key, const std::string & value);
    void DeleteHeadField(const std::string & key);
    bool HasHeadField(const std::string & key) const;
    std::string GetHeadField(const std::string & key) const;
    const std::map<std::string, std::string> & GetHeadFields() const { return m_headFields; }
    void SetHttpBody(const std::string & body);
    void AppendHttpBody(const std::string & data);
    void AppendHttpBody(const char * data, int len);
    const std::string & GetHttpBody() const { return m_body; }
    
    static const char * GetReason(ERespCode respCode);

protected:
    std::string   m_httpVersion;
    int           m_responseCode;
    std::string   m_reason;
    std::map<std::string, std::string> m_headFields; // key and value is lowercase
    std::string   m_body;
};