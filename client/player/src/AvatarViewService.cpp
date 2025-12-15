#include "AvatarViewService.hpp"
#include "LauncherView.hpp"
#include "Reflection/Reflection.hpp"
#include "Utility/StandardOut.hpp"
#include <QString>
namespace Aya
{
static Reflection::BoundFuncDesc<AvatarViewService, void(std::string)> avs_setColor(
    &AvatarViewService::setColor, "SetColor", "colorJSON", Security::None);
static Reflection::BoundFuncDesc<AvatarViewService, std::string()> avs_getColor(&AvatarViewService::getColors, "GetColor", Security::None);

const char* const sAvatarViewService = "AvatarViewService";

static Reflection::EventDesc<AvatarViewService, void(int64_t, std::string)> event_assetWornSignal(
    &AvatarViewService::assetWornSignal, "AssetWorn", "assetId", "assetType");

AvatarViewService::AvatarViewService()
{
    setName(sAvatarViewService);
}

AvatarViewService::~AvatarViewService() {}

void AvatarViewService::setJsHelpers(JsHelpers* helpers)
{
    this->helpers = helpers;
}

void AvatarViewService::setColor(std::string json)
{
    if (!this->helpers)
        throw std::runtime_error("helpers invalid");

    this->helpers->setBodyColorJson(QString::fromStdString(json));
    // this->helpers->onMapPicked()
}

std::string AvatarViewService::getColors()
{
    if (!this->helpers)
        throw std::runtime_error("helpers invalid");

    QString json = this->helpers->getBodyColorJson();
    return json.toUtf8().constData();
    ;
    // this->helpers->onMapPicked()
}
} // namespace Aya