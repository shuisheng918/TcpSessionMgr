#pragma once

#include <string>
#include <map>

class HttpSession;

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
    HttpResponse() : m_pHttpSession(NULL) {      }
    void SendResponse();

    static const char * GetReason(ERespCode respCode);
    
    HttpSession * m_pHttpSession;
    std::string   m_httpVersion;
    int           m_responseCode;
    std::map<std::string, std::string> m_headFields;
    std::string   m_body;
};