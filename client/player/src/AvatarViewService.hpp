#include "Tree/Instance.hpp"
#include "Tree/Service.hpp"
#include "LauncherView.hpp"
#include "Utility/BrickColor.hpp"

namespace Aya
{
extern const char* const sAvatarViewService;

class AvatarViewService
    : public DescribedCreatable<AvatarViewService, Instance, sAvatarViewService, Reflection::ClassDescriptor::RUNTIME_LOCAL>
    , public Service
{
    typedef DescribedCreatable<AvatarViewService, Instance, sAvatarViewService, Reflection::ClassDescriptor::RUNTIME_LOCAL> Super;
        
    JsHelpers* helpers;
public:
    AvatarViewService();
    virtual ~AvatarViewService();

    void setJsHelpers(JsHelpers* helpers);            
    Aya::signal<void(int64_t, std::string)> assetWornSignal;
    Aya::signal<void(std::string, BrickColor)> setBodyColorSignal; 

    void setColor(std::string json);
    std::string getColors();
};
} // namespace Aya