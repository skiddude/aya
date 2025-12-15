#pragma once

#include <string>
#include <boost/unordered_map.hpp>

namespace Aya
{

class LegacyContentTable
{
private:
    typedef boost::unordered_map<std::string, std::string> UrlMap;
    std::string mEmpty;
    UrlMap mMap;

public:
    LegacyContentTable();

    void AddEntry(const std::string& path, const std::string& contentId);
    const std::string& FindEntry(const std::string& path);
};
} // namespace Aya