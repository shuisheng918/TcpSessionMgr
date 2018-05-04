#pragma once

#include <arpa/inet.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <string>
#include <alloca.h>
#include <map>
#include "simple_log.h"

#define DELETE(p)   if (p) { delete p; p = NULL; }
#define DELETEV(p)  if (p) { delete [] p; p = NULL; }

#define STACK_ALLOC_MAX_SIZE  10240
#define stack_first_alloc(size) \
    ({ \
        char * res = NULL; \
        if (size <= STACK_ALLOC_MAX_SIZE) \
        { \
            res = (char*)alloca( 4 + 4 + size + 4); \
        } \
        else \
        { \
            res = (char*)malloc( 4 + 4 + size + 4); \
        } \
        *(uint32_t*)res = 0xeeeeeeee; \
        *(uint32_t*)(res + 4) = size; \
        *(uint32_t*)(res + 4 + 4 + size) = 0xffffffff; \
        res = res + 4 + 4; \
        res; \
    })

#define  stack_first_free(p) \
    if(p) \
    { \
        char * ptr = (char*)p; \
        assert(*(uint32_t*)(ptr - 8) == 0xeeeeeeee); \
        int32_t size = *(uint32_t*)(ptr - 4);  \
        assert(*(uint32_t*)(ptr + size) == 0xffffffff); \
        if (size > STACK_ALLOC_MAX_SIZE) \
        { \
            free(ptr - 8);  \
        } \
    }


// log
#define __FILENAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1):__FILE__)
#define log(fmt, args...)       SimpleLog::GetInstance()->Info("%s:%d " fmt, __FILENAME__, __LINE__, ##args)
#define logwarn(fmt, args...)   SimpleLog::GetInstance()->Warn("%s:%d " fmt, __FILENAME__, __LINE__, ##args)
#define logerror(fmt, args...)  SimpleLog::GetInstance()->Error("%s:%d " fmt, __FILENAME__, __LINE__, ##args)
#define logdebug(fmt, args...)  SimpleLog::GetInstance()->Debug("%s:%d " fmt, __FILENAME__, __LINE__, ##args)

uint64_t htonll(uint64_t val);

uint64_t ntohll(uint64_t val);

void TrimString(std::string & str);

void TolowerString(std::string & str);

std::string UrlDecode(const std::string & str);

std::string UrlEncode(const std::string & str);

void ParseHttpParameters(const std::string & paramStr, std::map<std::string, std::string> & params);


