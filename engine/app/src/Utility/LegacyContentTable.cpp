

#include "Utility/LegacyContentTable.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include "FastLog.hpp"
FASTFLAGVARIABLE(DebugRenderDownloadAssets, false)

namespace
{
inline bool isSlash(char ch)
{
    return ch == '\\' || ch == '/';
}

void normalizeUrl(std::string& url)
{
    char* data = &url[0];
    size_t size = url.size();
    size_t write = 0;

    for (size_t i = 0; i < size; ++i)
    {
        // that's faster than tolower
        if (static_cast<unsigned int>(data[i] - 'A') < 26)
            data[write++] = (data[i] - 'A') + 'a';
        else if (isSlash(data[i]))
        {
            data[write++] = '/';

            // skip subsequent slashes
            while (i + 1 < size && isSlash(data[i + 1]))
                i++;
        }
        else
            data[write++] = data[i];
    }

    url.resize(write);
}
} // namespace

namespace Aya
{
LegacyContentTable::LegacyContentTable()
{
    AddEntry("ayaasset://sounds/bass.wav", "ayaasset://sounds/bass.mp3");
    AddEntry("ayaasset://music/bass.wav", "ayaasset://sounds/bass.wav");
    AddEntry("ayaasset://music/ufofly.wav", "ayaasset://sounds/ufofly.wav");
    AddEntry("ayaasset://sounds/button.wav", "ayaasset://sounds/button.mp3");
    AddEntry("ayaasset://sounds/clickfast.wav", "ayaasset://sounds/clickfast.mp3");
    AddEntry("ayaasset://sounds/collide.wav", "ayaasset://sounds/collide.mp3");
    AddEntry("ayaasset://sounds/electronicpingshort.wav", "ayaasset://sounds/electronicpingshort.mp3");
    AddEntry("ayaasset://sounds/flashbulb.wav", "ayaasset://sounds/flashbulb.mp3");
    AddEntry("ayaasset://sounds/grass.ogg", "ayaasset://sounds/grass.mp3");
    AddEntry("ayaasset://sounds/grass2.ogg", "ayaasset://sounds/grass2.mp3");
    AddEntry("ayaasset://sounds/grass3.ogg", "ayaasset://sounds/grass3.mp3");
    AddEntry("ayaasset://sounds/grassstone.ogg", "ayaasset://sounds/grassstone.mp3");
    AddEntry("ayaasset://sounds/grassstone2.ogg", "ayaasset://sounds/grassstone2.mp3");
    AddEntry("ayaasset://sounds/grassstone3.ogg", "ayaasset://sounds/grassstone3.mp3");
    AddEntry("ayaasset://sounds/hit.wav", "ayaasset://sounds/hit.mp3");
    AddEntry("ayaasset://sounds/ice.ogg", "ayaasset://sounds/ice.mp3");
    AddEntry("ayaasset://sounds/ice2.ogg", "ayaasset://sounds/ice2.mp3");
    AddEntry("ayaasset://sounds/ice3.ogg", "ayaasset://sounds/ice3.mp3");
    AddEntry("ayaasset://sounds/icegrass.ogg", "ayaasset://sounds/icegrass.mp3");
    AddEntry("ayaasset://sounds/icegrass2.ogg", "ayaasset://sounds/icegrass2.mp3");
    AddEntry("ayaasset://sounds/icegrass3.ogg", "ayaasset://sounds/icegrass3.mp3");
    AddEntry("ayaasset://sounds/icemetal.ogg", "ayaasset://sounds/icemetal.mp3");
    AddEntry("ayaasset://sounds/icemetal2.ogg", "ayaasset://sounds/icemetal2.mp3");
    AddEntry("ayaasset://sounds/icemetal3.ogg", "ayaasset://sounds/icemetal3.mp3");
    AddEntry("ayaasset://sounds/icestone.ogg", "ayaasset://sounds/icestone.mp3");
    AddEntry("ayaasset://sounds/icestone2.ogg", "ayaasset://sounds/icestone2.mp3");
    AddEntry("ayaasset://sounds/icestone3.ogg", "ayaasset://sounds/icestone3.mp3");
    AddEntry("ayaasset://sounds/Kerplunk.wav", "ayaasset://sounds/Kerplunk.mp3");
    AddEntry("ayaasset://sounds/Kid", "ayaasset://sounds/Kid");
    AddEntry("ayaasset://sounds/metal.ogg", "ayaasset://sounds/metal.mp3");
    AddEntry("ayaasset://sounds/metal2.ogg", "ayaasset://sounds/metal2.mp3");
    AddEntry("ayaasset://sounds/metal3.ogg", "ayaasset://sounds/metal3.mp3");
    AddEntry("ayaasset://sounds/metalgrass.ogg", "ayaasset://sounds/metalgrass.mp3");
    AddEntry("ayaasset://sounds/metalgrass2.ogg", "ayaasset://sounds/metalgrass2.mp3");
    AddEntry("ayaasset://sounds/metalgrass3.ogg", "ayaasset://sounds/metalgrass3.mp3");
    AddEntry("ayaasset://sounds/metalstone.ogg", "ayaasset://sounds/metalstone.mp3");
    AddEntry("ayaasset://sounds/metalstone2.ogg", "ayaasset://sounds/metalstone2.mp3");
    AddEntry("ayaasset://sounds/metalstone3.ogg", "ayaasset://sounds/metalstone3.mp3");
    AddEntry("ayaasset://sounds/pageturn.wav", "ayaasset://sounds/pageturn.mp3");
    AddEntry("ayaasset://sounds/plasticgrass.ogg", "ayaasset://sounds/plasticgrass.mp3");
    AddEntry("ayaasset://sounds/plasticgrass2.ogg", "ayaasset://sounds/plasticgrass2.mp3");
    AddEntry("ayaasset://sounds/plasticgrass3.ogg", "ayaasset://sounds/plasticgrass3.mp3");
    AddEntry("ayaasset://sounds/plasticice.ogg", "ayaasset://sounds/plasticice.mp3");
    AddEntry("ayaasset://sounds/plasticice2.ogg", "ayaasset://sounds/plasticice2.mp3");
    AddEntry("ayaasset://sounds/plasticice3.ogg", "ayaasset://sounds/plasticice3.mp3");
    AddEntry("ayaasset://sounds/plasticmetal.ogg", "ayaasset://sounds/plasticmetal.mp3");
    AddEntry("ayaasset://sounds/plasticmetal2.ogg", "ayaasset://sounds/plasticmetal2.mp3");
    AddEntry("ayaasset://sounds/plasticmetal3.ogg", "ayaasset://sounds/plasticmetal3.mp3");
    AddEntry("ayaasset://sounds/plasticplastic.ogg", "ayaasset://sounds/plasticplastic.mp3");
    AddEntry("ayaasset://sounds/plasticplastic2.ogg", "ayaasset://sounds/plasticplastic2.mp3");
    AddEntry("ayaasset://sounds/plasticplastic3.ogg", "ayaasset://sounds/plasticplastic3.mp3");
    AddEntry("ayaasset://sounds/plasticstone.ogg", "ayaasset://sounds/plasticstone.mp3");
    AddEntry("ayaasset://sounds/plasticstone2.ogg", "ayaasset://sounds/plasticstone2.mp3");
    AddEntry("ayaasset://sounds/plasticstone3.ogg", "ayaasset://sounds/plasticstone3.mp3");
    AddEntry("ayaasset://sounds/Rubber band sling shot.wav", "ayaasset://sounds/Rubber band sling shot.mp3");
    AddEntry("ayaasset://sounds/Rubber band.mp3", "ayaasset://sounds/Rubber band.mp3");
    AddEntry("ayaasset://sounds/snap.wav", "ayaasset://sounds/snap.mp3");
    AddEntry("ayaasset://sounds/splat.wav", "ayaasset://sounds/splat.mp3");
    AddEntry("ayaasset://sounds/stone.ogg", "ayaasset://sounds/stone.mp3");
    AddEntry("ayaasset://sounds/stone2.ogg", "ayaasset://sounds/stone2.mp3");
    AddEntry("ayaasset://sounds/stone3.ogg", "ayaasset://sounds/stone3.mp3");
    AddEntry("ayaasset://sounds/switch.wav", "ayaasset://sounds/switch.mp3");
    AddEntry("ayaasset://sounds/SWITCH3.wav", "ayaasset://sounds/SWITCH3.mp3");
    AddEntry("ayaasset://sounds/swoosh.wav", "ayaasset://sounds/swoosh.mp3");
    AddEntry("ayaasset://sounds/swordlunge.wav", "ayaasset://sounds/swordlunge.mp3");
    AddEntry("ayaasset://sounds/swordslash.wav", "ayaasset://sounds/swordslash.mp3");
    AddEntry("ayaasset://sounds/unsheath.wav", "ayaasset://sounds/unsheath.mp3");
    AddEntry("ayaasset://sounds/uuhhh.wav", "ayaasset://sounds/uuhhh.mp3");
    AddEntry("ayaasset://sounds/victory.wav", "ayaasset://sounds/victory.mp3");
    AddEntry("ayaasset://sounds/woodgrass.ogg", "ayaasset://sounds/woodgrass.mp3");
    AddEntry("ayaasset://sounds/woodgrass2.ogg", "ayaasset://sounds/woodgrass2.mp3");
    AddEntry("ayaasset://sounds/woodgrass3.ogg", "ayaasset://sounds/woodgrass3.mp3");
    AddEntry("ayaasset://sounds/woodice.ogg", "ayaasset://sounds/woodice.mp3");
    AddEntry("ayaasset://sounds/woodice2.ogg", "ayaasset://sounds/woodice2.mp3");
    AddEntry("ayaasset://sounds/woodice3.ogg", "ayaasset://sounds/woodice3.mp3");
    AddEntry("ayaasset://sounds/woodmetal.ogg", "ayaasset://sounds/woodmetal.mp3");
    AddEntry("ayaasset://sounds/woodmetal2.ogg", "ayaasset://sounds/woodmetal2.mp3");
    AddEntry("ayaasset://sounds/woodmetal3.ogg", "ayaasset://sounds/woodmetal3.mp3");
    AddEntry("ayaasset://sounds/woodplastic.ogg", "ayaasset://sounds/woodplastic.mp3");
    AddEntry("ayaasset://sounds/woodplastic2.ogg", "ayaasset://sounds/woodplastic2.mp3");
    AddEntry("ayaasset://sounds/woodplastic3.ogg", "ayaasset://sounds/woodplastic3.mp3");
    AddEntry("ayaasset://sounds/woodstone.ogg", "ayaasset://sounds/woodstone.mp3");
    AddEntry("ayaasset://sounds/woodstone2.ogg", "ayaasset://sounds/woodstone2.mp3");
    AddEntry("ayaasset://sounds/woodstone3.ogg", "ayaasset://sounds/woodstone3.mp3");
    AddEntry("ayaasset://sounds/woodwood.ogg", "ayaasset://sounds/woodwood.mp3");
    AddEntry("ayaasset://sounds/woodwood2.ogg", "ayaasset://sounds/woodwood2.mp3");
    AddEntry("ayaasset://sounds/woodwood3.ogg", "ayaasset://sounds/woodwood3.mp3");
    AddEntry("rbxasset://sounds/bass.wav", "ayaasset://sounds/bass.mp3");
    AddEntry("rbxasset://music/bass.wav", "ayaasset://sounds/bass.wav");
    AddEntry("rbxasset://music/ufofly.wav", "ayaasset://sounds/ufofly.wav");
    AddEntry("rbxasset://sounds/button.wav", "ayaasset://sounds/button.mp3");
    AddEntry("rbxasset://sounds/clickfast.wav", "ayaasset://sounds/clickfast.mp3");
    AddEntry("rbxasset://sounds/collide.wav", "ayaasset://sounds/collide.mp3");
    AddEntry("rbxasset://sounds/electronicpingshort.wav", "ayaasset://sounds/electronicpingshort.mp3");
    AddEntry("rbxasset://sounds/flashbulb.wav", "ayaasset://sounds/flashbulb.mp3");
    AddEntry("rbxasset://sounds/grass.ogg", "ayaasset://sounds/grass.mp3");
    AddEntry("rbxasset://sounds/grass2.ogg", "ayaasset://sounds/grass2.mp3");
    AddEntry("rbxasset://sounds/grass3.ogg", "ayaasset://sounds/grass3.mp3");
    AddEntry("rbxasset://sounds/grassstone.ogg", "ayaasset://sounds/grassstone.mp3");
    AddEntry("rbxasset://sounds/grassstone2.ogg", "ayaasset://sounds/grassstone2.mp3");
    AddEntry("rbxasset://sounds/grassstone3.ogg", "ayaasset://sounds/grassstone3.mp3");
    AddEntry("rbxasset://sounds/hit.wav", "ayaasset://sounds/hit.mp3");
    AddEntry("rbxasset://sounds/ice.ogg", "ayaasset://sounds/ice.mp3");
    AddEntry("rbxasset://sounds/ice2.ogg", "ayaasset://sounds/ice2.mp3");
    AddEntry("rbxasset://sounds/ice3.ogg", "ayaasset://sounds/ice3.mp3");
    AddEntry("rbxasset://sounds/icegrass.ogg", "ayaasset://sounds/icegrass.mp3");
    AddEntry("rbxasset://sounds/icegrass2.ogg", "ayaasset://sounds/icegrass2.mp3");
    AddEntry("rbxasset://sounds/icegrass3.ogg", "ayaasset://sounds/icegrass3.mp3");
    AddEntry("rbxasset://sounds/icemetal.ogg", "ayaasset://sounds/icemetal.mp3");
    AddEntry("rbxasset://sounds/icemetal2.ogg", "ayaasset://sounds/icemetal2.mp3");
    AddEntry("rbxasset://sounds/icemetal3.ogg", "ayaasset://sounds/icemetal3.mp3");
    AddEntry("rbxasset://sounds/icestone.ogg", "ayaasset://sounds/icestone.mp3");
    AddEntry("rbxasset://sounds/icestone2.ogg", "ayaasset://sounds/icestone2.mp3");
    AddEntry("rbxasset://sounds/icestone3.ogg", "ayaasset://sounds/icestone3.mp3");
    AddEntry("rbxasset://sounds/Kerplunk.wav", "ayaasset://sounds/Kerplunk.mp3");
    AddEntry("rbxasset://sounds/Kid", "ayaasset://sounds/Kid");
    AddEntry("rbxasset://sounds/metal.ogg", "ayaasset://sounds/metal.mp3");
    AddEntry("rbxasset://sounds/metal2.ogg", "ayaasset://sounds/metal2.mp3");
    AddEntry("rbxasset://sounds/metal3.ogg", "ayaasset://sounds/metal3.mp3");
    AddEntry("rbxasset://sounds/metalgrass.ogg", "ayaasset://sounds/metalgrass.mp3");
    AddEntry("rbxasset://sounds/metalgrass2.ogg", "ayaasset://sounds/metalgrass2.mp3");
    AddEntry("rbxasset://sounds/metalgrass3.ogg", "ayaasset://sounds/metalgrass3.mp3");
    AddEntry("rbxasset://sounds/metalstone.ogg", "ayaasset://sounds/metalstone.mp3");
    AddEntry("rbxasset://sounds/metalstone2.ogg", "ayaasset://sounds/metalstone2.mp3");
    AddEntry("rbxasset://sounds/metalstone3.ogg", "ayaasset://sounds/metalstone3.mp3");
    AddEntry("rbxasset://sounds/pageturn.wav", "ayaasset://sounds/pageturn.mp3");
    AddEntry("rbxasset://sounds/plasticgrass.ogg", "ayaasset://sounds/plasticgrass.mp3");
    AddEntry("rbxasset://sounds/plasticgrass2.ogg", "ayaasset://sounds/plasticgrass2.mp3");
    AddEntry("rbxasset://sounds/plasticgrass3.ogg", "ayaasset://sounds/plasticgrass3.mp3");
    AddEntry("rbxasset://sounds/plasticice.ogg", "ayaasset://sounds/plasticice.mp3");
    AddEntry("rbxasset://sounds/plasticice2.ogg", "ayaasset://sounds/plasticice2.mp3");
    AddEntry("rbxasset://sounds/plasticice3.ogg", "ayaasset://sounds/plasticice3.mp3");
    AddEntry("rbxasset://sounds/plasticmetal.ogg", "ayaasset://sounds/plasticmetal.mp3");
    AddEntry("rbxasset://sounds/plasticmetal2.ogg", "ayaasset://sounds/plasticmetal2.mp3");
    AddEntry("rbxasset://sounds/plasticmetal3.ogg", "ayaasset://sounds/plasticmetal3.mp3");
    AddEntry("rbxasset://sounds/plasticplastic.ogg", "ayaasset://sounds/plasticplastic.mp3");
    AddEntry("rbxasset://sounds/plasticplastic2.ogg", "ayaasset://sounds/plasticplastic2.mp3");
    AddEntry("rbxasset://sounds/plasticplastic3.ogg", "ayaasset://sounds/plasticplastic3.mp3");
    AddEntry("rbxasset://sounds/plasticstone.ogg", "ayaasset://sounds/plasticstone.mp3");
    AddEntry("rbxasset://sounds/plasticstone2.ogg", "ayaasset://sounds/plasticstone2.mp3");
    AddEntry("rbxasset://sounds/plasticstone3.ogg", "ayaasset://sounds/plasticstone3.mp3");
    AddEntry("rbxasset://sounds/Rubber band sling shot.wav", "ayaasset://sounds/Rubber band sling shot.mp3");
    AddEntry("rbxasset://sounds/Rubber band.mp3", "ayaasset://sounds/Rubber band.mp3");
    AddEntry("rbxasset://sounds/snap.wav", "ayaasset://sounds/snap.mp3");
    AddEntry("rbxasset://sounds/splat.wav", "ayaasset://sounds/splat.mp3");
    AddEntry("rbxasset://sounds/stone.ogg", "ayaasset://sounds/stone.mp3");
    AddEntry("rbxasset://sounds/stone2.ogg", "ayaasset://sounds/stone2.mp3");
    AddEntry("rbxasset://sounds/stone3.ogg", "ayaasset://sounds/stone3.mp3");
    AddEntry("rbxasset://sounds/switch.wav", "ayaasset://sounds/switch.mp3");
    AddEntry("rbxasset://sounds/SWITCH3.wav", "ayaasset://sounds/SWITCH3.mp3");
    AddEntry("rbxasset://sounds/swoosh.wav", "ayaasset://sounds/swoosh.mp3");
    AddEntry("rbxasset://sounds/swordlunge.wav", "ayaasset://sounds/swordlunge.mp3");
    AddEntry("rbxasset://sounds/swordslash.wav", "ayaasset://sounds/swordslash.mp3");
    AddEntry("rbxasset://sounds/unsheath.wav", "ayaasset://sounds/unsheath.mp3");
    AddEntry("rbxasset://sounds/uuhhh.wav", "ayaasset://sounds/uuhhh.mp3");
    AddEntry("rbxasset://sounds/victory.wav", "ayaasset://sounds/victory.mp3");
    AddEntry("rbxasset://sounds/woodgrass.ogg", "ayaasset://sounds/woodgrass.mp3");
    AddEntry("rbxasset://sounds/woodgrass2.ogg", "ayaasset://sounds/woodgrass2.mp3");
    AddEntry("rbxasset://sounds/woodgrass3.ogg", "ayaasset://sounds/woodgrass3.mp3");
    AddEntry("rbxasset://sounds/woodice.ogg", "ayaasset://sounds/woodice.mp3");
    AddEntry("rbxasset://sounds/woodice2.ogg", "ayaasset://sounds/woodice2.mp3");
    AddEntry("rbxasset://sounds/woodice3.ogg", "ayaasset://sounds/woodice3.mp3");
    AddEntry("rbxasset://sounds/woodmetal.ogg", "ayaasset://sounds/woodmetal.mp3");
    AddEntry("rbxasset://sounds/woodmetal2.ogg", "ayaasset://sounds/woodmetal2.mp3");
    AddEntry("rbxasset://sounds/woodmetal3.ogg", "ayaasset://sounds/woodmetal3.mp3");
    AddEntry("rbxasset://sounds/woodplastic.ogg", "ayaasset://sounds/woodplastic.mp3");
    AddEntry("rbxasset://sounds/woodplastic2.ogg", "ayaasset://sounds/woodplastic2.mp3");
    AddEntry("rbxasset://sounds/woodplastic3.ogg", "ayaasset://sounds/woodplastic3.mp3");
    AddEntry("rbxasset://sounds/woodstone.ogg", "ayaasset://sounds/woodstone.mp3");
    AddEntry("rbxasset://sounds/woodstone2.ogg", "ayaasset://sounds/woodstone2.mp3");
    AddEntry("rbxasset://sounds/woodstone3.ogg", "ayaasset://sounds/woodstone3.mp3");
    AddEntry("rbxasset://sounds/woodwood.ogg", "ayaasset://sounds/woodwood.mp3");
    AddEntry("rbxasset://sounds/woodwood2.ogg", "ayaasset://sounds/woodwood2.mp3");
    AddEntry("rbxasset://sounds/woodwood3.ogg", "ayaasset://sounds/woodwood3.mp3");

    // Aya remaps
    AddEntry("ayaasset://fonts/character3.rbxm", "ayaasset://avatar/character.rbxmx");
    AddEntry("ayaasset://fonts/characterR15.rbxm", "ayaasset://avatar/characterR15.rbxm");
    AddEntry("ayaasset://fonts/CompositExtraSlot0.mesh", "ayaasset://avatar/compositing/CompositExtraSlot0.mesh");
    AddEntry("ayaasset://fonts/CompositExtraSlot1.mesh", "ayaasset://avatar/compositing/CompositExtraSlot1.mesh");
    AddEntry("ayaasset://fonts/CompositExtraSlot2.mesh", "ayaasset://avatar/compositing/CompositExtraSlot2.mesh");
    AddEntry("ayaasset://fonts/CompositExtraSlot3.mesh", "ayaasset://avatar/compositing/CompositExtraSlot3.mesh");
    AddEntry("ayaasset://fonts/CompositExtraSlot4.mesh", "ayaasset://avatar/compositing/CompositExtraSlot4.mesh");
    AddEntry("ayaasset://fonts/CompositFullAtlasBaseTexture.mesh", "ayaasset://avatar/compositing/CompositFullAtlasBaseTexture.mesh");
    AddEntry("ayaasset://fonts/CompositFullAtlasOverlayTexture.mesh", "ayaasset://avatar/compositing/CompositFullAtlasOverlayTexture.mesh");
    AddEntry("ayaasset://fonts/CompositLeftArmBase.mesh", "ayaasset://avatar/compositing/CompositLeftArmBase.mesh");
    AddEntry("ayaasset://fonts/CompositRightArmBase.mesh", "ayaasset://avatar/compositing/CompositRightArmBase.mesh");
    AddEntry("ayaasset://fonts/CompositTorsoBase.mesh", "ayaasset://avatar/compositing/CompositTorsoBase.mesh");
    AddEntry("ayaasset://fonts/CompositLeftLegBase.mesh", "ayaasset://avatar/compositing/CompositLeftLegBase.mesh");
    AddEntry("ayaasset://fonts/CompositRightLegBase.mesh", "ayaasset://avatar/compositing/CompositRightLegBase.mesh");
    AddEntry("ayaasset://fonts/CompositShirtTemplate.mesh", "ayaasset://avatar/compositing/CompositShirtTemplate.mesh");
    AddEntry("ayaasset://fonts/CompositPantsTemplate.mesh", "ayaasset://avatar/compositing/CompositPantsTemplate.mesh");
    AddEntry("ayaasset://fonts/CompositTShirt.mesh", "ayaasset://avatar/compositing/CompositTShirt.mesh");
    AddEntry("ayaasset://fonts/R15CompositLeftArmBase.mesh", "ayaasset://avatar/compositing/R15CompositLeftArmBase.mesh");
    AddEntry("ayaasset://fonts/R15CompositRightArmBase.mesh", "ayaasset://avatar/compositing/R15CompositRightArmBase.mesh");
    AddEntry("ayaasset://fonts/R15CompositTorsoBase.mesh", "ayaasset://avatar/compositing/R15CompositTorsoBase.mesh");
    AddEntry("ayaasset://fonts/characterCameraScript.rbxmx", "ayaasset://avatar/scripts/characterCameraScript.rbxmx");
    AddEntry("ayaasset://fonts/characterControlScript.rbxmx", "ayaasset://avatar/scripts/characterControlScript.rbxmx");
    AddEntry("ayaasset://fonts/humanoidSoundNewLocal.rbxmx", "ayaasset://avatar/scripts/humanoidSound.rbxmx");
    AddEntry("ayaasset://fonts/humanoidAnimateLocalKeyframe2.rbxm", "ayaasset://avatar/scripts/humanoidAnimate.rbxmx");
    AddEntry("ayaasset://fonts/head.mesh", "ayaasset://avatar/heads/head.mesh");
    AddEntry("ayaasset://fonts/headA.mesh", "ayaasset://avatar/heads/headA.mesh");
    AddEntry("ayaasset://fonts/headB.mesh", "ayaasset://avatar/heads/headB.mesh");
    AddEntry("ayaasset://fonts/headC.mesh", "ayaasset://avatar/heads/headC.mesh");
    AddEntry("ayaasset://fonts/headD.mesh", "ayaasset://avatar/heads/headD.mesh");
    AddEntry("ayaasset://fonts/headE.mesh", "ayaasset://avatar/heads/headE.mesh");
    AddEntry("ayaasset://fonts/headF.mesh", "ayaasset://avatar/heads/headF.mesh");
    AddEntry("ayaasset://fonts/headG.mesh", "ayaasset://avatar/heads/headG.mesh");
    AddEntry("ayaasset://fonts/headH.mesh", "ayaasset://avatar/heads/headH.mesh");
    AddEntry("ayaasset://fonts/headI.mesh", "ayaasset://avatar/heads/headI.mesh");
    AddEntry("ayaasset://fonts/headJ.mesh", "ayaasset://avatar/heads/headJ.mesh");
    AddEntry("ayaasset://fonts/headK.mesh", "ayaasset://avatar/heads/headK.mesh");
    AddEntry("ayaasset://fonts/headL.mesh", "ayaasset://avatar/heads/headL.mesh");
    AddEntry("ayaasset://fonts/headM.mesh", "ayaasset://avatar/heads/headM.mesh");
    AddEntry("ayaasset://fonts/headN.mesh", "ayaasset://avatar/heads/headN.mesh");
    AddEntry("ayaasset://fonts/headO.mesh", "ayaasset://avatar/heads/headO.mesh");
    AddEntry("ayaasset://fonts/headP.mesh", "ayaasset://avatar/heads/headP.mesh");
    AddEntry("rbxasset://fonts/character3.rbxm", "ayaasset://avatar/character.rbxmx");
    AddEntry("rbxasset://fonts/characterR15.rbxm", "ayaasset://avatar/characterR15.rbxm");
    AddEntry("rbxasset://fonts/CompositExtraSlot0.mesh", "ayaasset://avatar/compositing/CompositExtraSlot0.mesh");
    AddEntry("rbxasset://fonts/CompositExtraSlot1.mesh", "ayaasset://avatar/compositing/CompositExtraSlot1.mesh");
    AddEntry("rbxasset://fonts/CompositExtraSlot2.mesh", "ayaasset://avatar/compositing/CompositExtraSlot2.mesh");
    AddEntry("rbxasset://fonts/CompositExtraSlot3.mesh", "ayaasset://avatar/compositing/CompositExtraSlot3.mesh");
    AddEntry("rbxasset://fonts/CompositExtraSlot4.mesh", "ayaasset://avatar/compositing/CompositExtraSlot4.mesh");
    AddEntry("rbxasset://fonts/CompositFullAtlasBaseTexture.mesh", "ayaasset://avatar/compositing/CompositFullAtlasBaseTexture.mesh");
    AddEntry("rbxasset://fonts/CompositFullAtlasOverlayTexture.mesh", "ayaasset://avatar/compositing/CompositFullAtlasOverlayTexture.mesh");
    AddEntry("rbxasset://fonts/CompositLeftArmBase.mesh", "ayaasset://avatar/compositing/CompositLeftArmBase.mesh");
    AddEntry("rbxasset://fonts/CompositRightArmBase.mesh", "ayaasset://avatar/compositing/CompositRightArmBase.mesh");
    AddEntry("rbxasset://fonts/CompositTorsoBase.mesh", "ayaasset://avatar/compositing/CompositTorsoBase.mesh");
    AddEntry("rbxasset://fonts/CompositLeftLegBase.mesh", "ayaasset://avatar/compositing/CompositLeftLegBase.mesh");
    AddEntry("rbxasset://fonts/CompositRightLegBase.mesh", "ayaasset://avatar/compositing/CompositRightLegBase.mesh");
    AddEntry("rbxasset://fonts/CompositShirtTemplate.mesh", "ayaasset://avatar/compositing/CompositShirtTemplate.mesh");
    AddEntry("rbxasset://fonts/CompositPantsTemplate.mesh", "ayaasset://avatar/compositing/CompositPantsTemplate.mesh");
    AddEntry("rbxasset://fonts/CompositTShirt.mesh", "ayaasset://avatar/compositing/CompositTShirt.mesh");
    AddEntry("rbxasset://fonts/R15CompositLeftArmBase.mesh", "ayaasset://avatar/compositing/R15CompositLeftArmBase.mesh");
    AddEntry("rbxasset://fonts/R15CompositRightArmBase.mesh", "ayaasset://avatar/compositing/R15CompositRightArmBase.mesh");
    AddEntry("rbxasset://fonts/R15CompositTorsoBase.mesh", "ayaasset://avatar/compositing/R15CompositTorsoBase.mesh");
    AddEntry("rbxasset://fonts/characterCameraScript.rbxmx", "ayaasset://avatar/scripts/characterCameraScript.rbxmx");
    AddEntry("rbxasset://fonts/characterControlScript.rbxmx", "ayaasset://avatar/scripts/characterControlScript.rbxmx");
    AddEntry("rbxasset://fonts/humanoidSoundNewLocal.rbxmx", "ayaasset://avatar/scripts/humanoidSound.rbxmx");
    AddEntry("rbxasset://fonts/humanoidAnimateLocalKeyframe2.rbxm", "ayaasset://avatar/scripts/humanoidAnimate.rbxmx");
    AddEntry("rbxasset://fonts/head.mesh", "ayaasset://avatar/heads/head.mesh");
    AddEntry("rbxasset://fonts/headA.mesh", "ayaasset://avatar/heads/headA.mesh");
    AddEntry("rbxasset://fonts/headB.mesh", "ayaasset://avatar/heads/headB.mesh");
    AddEntry("rbxasset://fonts/headC.mesh", "ayaasset://avatar/heads/headC.mesh");
    AddEntry("rbxasset://fonts/headD.mesh", "ayaasset://avatar/heads/headD.mesh");
    AddEntry("rbxasset://fonts/headE.mesh", "ayaasset://avatar/heads/headE.mesh");
    AddEntry("rbxasset://fonts/headF.mesh", "ayaasset://avatar/heads/headF.mesh");
    AddEntry("rbxasset://fonts/headG.mesh", "ayaasset://avatar/heads/headG.mesh");
    AddEntry("rbxasset://fonts/headH.mesh", "ayaasset://avatar/heads/headH.mesh");
    AddEntry("rbxasset://fonts/headI.mesh", "ayaasset://avatar/heads/headI.mesh");
    AddEntry("rbxasset://fonts/headJ.mesh", "ayaasset://avatar/heads/headJ.mesh");
    AddEntry("rbxasset://fonts/headK.mesh", "ayaasset://avatar/heads/headK.mesh");
    AddEntry("rbxasset://fonts/headL.mesh", "ayaasset://avatar/heads/headL.mesh");
    AddEntry("rbxasset://fonts/headM.mesh", "ayaasset://avatar/heads/headM.mesh");
    AddEntry("rbxasset://fonts/headN.mesh", "ayaasset://avatar/heads/headN.mesh");
    AddEntry("rbxasset://fonts/headO.mesh", "ayaasset://avatar/heads/headO.mesh");
    AddEntry("rbxasset://fonts/headP.mesh", "ayaasset://avatar/heads/headP.mesh");
}

void LegacyContentTable::AddEntry(const std::string& path, const std::string& contentId)
{
    std::string url = path;
    normalizeUrl(url);

    mMap[url] = contentId;
}

const std::string& LegacyContentTable::FindEntry(const std::string& path)
{
    std::string url = path;
    normalizeUrl(url);

    UrlMap::const_iterator it = mMap.find(url);

    return (it == mMap.end()) ? mEmpty : it->second;
}
} // namespace Aya