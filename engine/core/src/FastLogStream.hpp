#pragma once

#include "FastLog.hpp"

namespace FLog
{

bool  SetLogStreamFile(const char* path);
bool  SetLogStreamNetworkClient(const char* address, unsigned int port);
bool  SetLogStreamNetworkServer(unsigned int port);

} // namespace FLog
