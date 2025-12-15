

#pragma once

#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <vector>

namespace Aya
{
class Instance;
class DataModel;
namespace Network
{
typedef enum
{
    Accept,
    Reject
} FilterResult;

extern std::string password;
extern std::string securityKey;
bool isPlayerAuthenticationEnabled();
void init();
bool isNetworkClient(const Instance* context);
bool getSystemUrlLocal(DataModel* dataModel);

// Used for debugging and development:
void setPassword(const char* password);

bool isTrustedContent(const char* url);
} // namespace Network

void spawnDebugCheckThreads(boost::weak_ptr<Aya::DataModel> weakDataModel);
} // namespace Aya