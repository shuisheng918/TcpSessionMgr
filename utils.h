#pragma once

#include <string>
#include <map>

void TrimString(std::string & str);

void TolowerString(std::string & str);

std::string UrlDecode(const std::string & str);

std::string UrlEncode(const std::string & str);

void ParseHttpParameters(const std::string & paramStr, std::map<std::string, std::string> & params);


