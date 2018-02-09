//author : wushuisheng, simple key value config file loader
#include "easyconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <vector>
#include <string.h>
#include "utils.h"

using namespace std;


bool LoadConfigFile(const char * path, map<string, string> & config)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL)
    {
        printf("fopen error : %s. path=%s, At %s:%d\n", strerror(errno), path, basename(__FILE__), __LINE__);
        return false;
    }
    char lineBuf[4096];
    vector<string> key_val;
    while ( fgets(lineBuf, 4096, fp) )
    {
        string line = lineBuf;
        TrimString(line);
        if (line.size() == 0 || line[0] == '#')  //ignore empty line and comment line
        {
            continue;
        }
        else
        {
            string key, val;
            int posOfEq = line.find_first_of("=");
            if (posOfEq > 0)
            {
                key = line.substr(0, posOfEq);
                val = line.substr(posOfEq + 1);
                TrimString(key);
                TrimString(val);
                if (key.size() > 0)
                {
                    config[key] = val;
                }
            }
        }
    }
    fclose(fp);
    return true;
}


void ExtractIpPort(const string &ipAndPort, string &ip, int &port)
{
    int spacePos = ipAndPort.find_first_of(":");
    if (spacePos != -1)
    {
        ip = ipAndPort.substr(0, spacePos);
        string portSection = ipAndPort.substr(spacePos + 1);
        TrimString(portSection);
        port = atoi(portSection.c_str());
    }
    else
    {
        ip = "";
        port = 0;
    }

}

