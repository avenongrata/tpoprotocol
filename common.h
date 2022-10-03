#ifndef COMMON_H
#define COMMON_H

//-----------------------------------------------------------------------------

#include <iostream>
#include <vector>
#include <sstream>

//-----------------------------------------------------------------------------

#define printerr(fmt,...) \
    do {\
            fprintf(stderr, fmt, ## __VA_ARGS__); \
            fflush(stderr); \
    } while(0)

//-----------------------------------------------------------------------------

std::vector<std::string> splitString(const std::string & msg, char delimiter);

//-----------------------------------------------------------------------------

namespace sep
{
    extern char dataSep;
    extern char baseSep;
}

//-----------------------------------------------------------------------------

#endif // COMMON_H
