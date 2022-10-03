#include "common.h"

//-----------------------------------------------------------------------------

std::vector<std::string> splitString(const std::string & msg, char delimiter)
{
    std::vector<std::string> tokens;
    std::istringstream tokenStream(msg);
    std::string token;

    while (std::getline(tokenStream, token, delimiter))
        tokens.push_back(token);

    return tokens;
}

//-----------------------------------------------------------------------------

namespace sep
{
    char dataSep = ',';
    char baseSep = '/';
}
