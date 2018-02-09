#pragma once
#include <map>
#include <string>

bool LoadConfigFile(const char * path, std::map<std::string, std::string> & config);

void ExtractIpPort(const std::string &ipAndPort, std::string &ip, int &port);



