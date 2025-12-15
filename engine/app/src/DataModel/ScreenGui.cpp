

#include "DataModel/GuiObject.hpp"
#include "DataModel/ScreenGui.hpp"
#include "DataModel/PlayerGui.hpp"
#include "DataModel/GuiService.hpp"
#include "Base/Adorn.hpp"
#include "FastLog.hpp"

namespace Aya
{
const char* const sScreenGui = "ScreenGui";


static const Reflection::PropDescriptor<ScreenGui, Vector2int16> prop_ReplicateAbsoluteSize("ReplicatingAbsoluteSize", category_Data,
    &ScreenGui::getAbsoluteSize, &ScreenGui::setReplicatingAbsoluteSize, Reflection::PropertyDescriptor::REPLICATE_ONLY);
static const Reflection::PropDescriptor<ScreenGui, Vector2int16> prop_ReplicateAbsolutePosition("ReplicatingAbsolutePosition", category_Data,
    &ScreenGui::getAbsolutePosition, &ScreenGui::setReplicatingAbsolutePosition, Reflection::PropertyDescriptor::REPLICATE_ONLY);
const Reflection::PropDescriptor<ScreenGui, bool> prop_Enabled("Enabled", category_Data, &ScreenGui::getEnabled, &ScreenGui::setEnabled);
const Reflection::PropDescriptor<ScreenGui, bool> prop_IgnoreGuiInset(
    "IgnoreGuiInset", category_Data, &ScreenGui::getIgnoreGuiInset, &ScreenGui::setIgnoreGuiInset);
static const Reflection::PropDescriptor<ScreenGui, int> prop_DisplayOrder(
    "DisplayOrder", category_Data, &ScreenGui::getDisplayOrder, &ScreenGui::setDisplayOrder);
static const Reflection::PropDescriptor<ScreenGui, bool> prop_ResetOnSpawn(
    "ResetOnSpawn", category_Data, &ScreenGui::getResetOnSpawn, &ScreenGui::setResetOnSpawn);
REFLECTION_END();

ScreenGui::ScreenGui()
    : DescribedCreatable<ScreenGui, GuiLayerCollector, sScreenGui>(sScreenGui)
    , renderable(false)
    , enabled(true)
    , bufferedViewport(Rect2D::xywh(0.0f, 0.0f, 800.0f, 600.0f))
    , ignoreGuiInset(false)
    , displayOrder(0)
    , resetOnSpawn(true)
{
}

ScreenGui::ScreenGui(const char* name)
    : DescribedCreatable<ScreenGui, GuiLayerCollector, sScreenGui>(name)
    , renderable(false)
    , enabled(true)
    , displayOrder(0)
    , resetOnSpawn(true)
{
}

ScreenGui::~ScreenGui()
{
    modalGuiObjects.clear();
    connections.clear();
}

void ScreenGui::setEnabled(bool value)
{
    if (enabled != value)
    {
        enabled = value;
        renderable = value;
        raisePropertyChanged(prop_Enabled);

        if (!enabled)
        {
            modalGuiObjects.clear();
            return;
        }

        shouldRenderSetDirty();
    }
}

void ScreenGui::setIgnoreGuiInset(bool value)
{
    if (ignoreGuiInset != value)
    {
        ignoreGuiInset = value;
        raisePropertyChanged(prop_IgnoreGuiInset);
    }
}

void ScreenGui::setDisplayOrder(int value)
{
    if (displayOrder != value)
    {
        displayOrder = value;
        raisePropertyChanged(prop_DisplayOrder);
        shouldRenderSetDirty();
    }
}

void ScreenGui::setResetOnSpawn(bool value)
{
    if (resetOnSpawn != value)
    {
        resetOnSpawn = value;
        raisePropertyChanged(prop_ResetOnSpawn);
    }
}

void ScreenGui::setReplicatingAbsoluteSize(Vector2int16 value)
{
    // We do need to handle a resize since it came in over the replication engine.
    handleResize(getRect2D(), false);
}
void ScreenGui::setReplicatingAbsolutePosition(Vector2int16 value)
{
    // We do need to handle a resize since it came in over the replication engine.
    handleResize(getRect2D(), false);
}

void ScreenGui::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
    if (oldProvider)
    {
        modalGuiObjects.clear();
        connections.clear();
    }


    Super::onServiceProvider(oldProvider, newProvider);

    if (newProvider)
    {
        // if we have core gui set up, get it's absolute size and use that (instead of 800x600)
        if (CoreGuiService* coreGui = ServiceProvider::create<CoreGuiService>(newProvider))
        {
            if (shared_ptr<Aya::ScreenGui> coreScreenGui = coreGui->getRobloxScreenGui())
            {
                if (coreScreenGui.get() != this)
                {
                    setBufferedViewport(coreScreenGui->getViewport());
                }
            }
        }
    }
}

void ScreenGui::onPropertyChanged(const Reflection::PropertyDescriptor& descriptor)
{
    Super::onPropertyChanged(descriptor);

    if (descriptor == Super::prop_AbsoluteSize)
    {
        handleResize(bufferedViewport, false);
        raisePropertyChanged(prop_ReplicateAbsoluteSize);
    }
    else if (descriptor == Super::prop_AbsolutePosition)
    {
        handleResize(bufferedViewport, false);
        raisePropertyChanged(prop_ReplicateAbsolutePosition);
    }
}

// The main windows parent can be any kind of non-GUI instance
bool ScreenGui::askSetParent(const Instance* instance) const
{
    return (Instance::fastDynamicCast<GuiBase2d>(instance) == NULL);
}

bool ScreenGui::isAncestorRenderableGui() const
{
    const Instance* ancestor = getParent();
    while (ancestor != NULL)
    {
        if (ancestor->fastDynamicCast<BasePlayerGui>())
            return true;
        ancestor = ancestor->getParent();
    }
    return false;
}

bool ScreenGui::canProcessMeAndDescendants() const
{
    // Each ScreenGui is processed individually so we have to stop the hierarchy traversal
    // to eliminate duplicate processing of elements in nested ScreenGuis
    return false;
}

void ScreenGui::onAncestorChanged(const AncestorChanged& event)
{
    Super::onAncestorChanged(event);

    if (!enabled)
    {
    }

    bool newRenderable = isAncestorRenderableGui();

    if (newRenderable != renderable)
    {
        renderable = newRenderable;
        shouldRenderSetDirty();
    }

    handleResize(bufferedViewport, false);
}

void ScreenGui::render2d(Adorn* adorn)
{
    render2dContext(adorn, NULL);
}

void ScreenGui::render2dContext(Adorn* adorn, const Instance* context)
{
    setBufferedViewport(adorn->getUserGuiRect());

    Super::render2dContext(adorn, context);
}

void ScreenGui::setBufferedViewport(Rect2D newViewport)
{
    if (newViewport != bufferedViewport)
    {
        bufferedViewport = newViewport;
        handleResize(bufferedViewport, false);
    }
}

Vector2 ScreenGui::getAbsolutePosition() const
{
    if (ignoreGuiInset)
        return absolutePosition;

    if (GuiService* guiService = Aya::ServiceProvider::find<GuiService>(this))
    {
        Vector4 guiInset = guiService->getGlobalGuiInset();
        return Vector2(absolutePosition.x - guiInset.x, absolutePosition.y - guiInset.y);
    }
    return absolutePosition;
}

GuiResponse ScreenGui::process(const shared_ptr<InputObject>& event)
{
    // N.B.: HUD mouse message processing blowing us up on Mac; hack around it for now
    return Super::process(event);
}

GuiResponse ScreenGui::processGesture(
    const UserInputService::Gesture gesture, shared_ptr<const Aya::Reflection::ValueArray> touchPositions, shared_ptr<const Reflection::Tuple> args)
{
    return Super::processGesture(gesture, touchPositions, args);
}

bool ScreenGui::removeModalButton(Aya::GuiButton* guiButton)
{
    for (std::vector<GuiButton*>::iterator iter = modalGuiObjects.begin(); iter != modalGuiObjects.end(); ++iter)
    {
        if ((*iter) == guiButton)
        {
            modalGuiObjects.erase(iter);
            return true;
        }
    }
    return false;
}

bool ScreenGui::insertModalButton(Aya::GuiButton* guiButton)
{
    for (std::vector<GuiButton*>::iterator iter = modalGuiObjects.begin(); iter != modalGuiObjects.end(); ++iter)
        if ((*iter) == guiButton)
            return false;

    modalGuiObjects.push_back(guiButton);
    return true;
}

void ScreenGui::onModalButtonChanged(const Aya::Reflection::PropertyDescriptor* desc, Aya::GuiButton* guiButton)
{
    if (guiButton->getModal())
        insertModalButton(guiButton);
    else
        removeModalButton(guiButton);
}

void ScreenGui::onDescendantAdded(Instance* instance)
{
    Super::onDescendantAdded(instance);

    if (Aya::GuiButton* guiButton = Instance::fastDynamicCast<Aya::GuiButton>(instance))
    {
        connections[instance] = guiButton->propertyChangedSignal.connect(boost::bind(&ScreenGui::onModalButtonChanged, this, _1, guiButton));

        if (guiButton->getModal())
            insertModalButton(guiButton);
    }
}
void ScreenGui::onDescendantRemoving(const shared_ptr<Instance>& instance)
{
    Super::onDescendantRemoving(instance);

    if (Aya::GuiButton* guiButton = Instance::fastDynamicCast<Aya::GuiButton>(instance.get()))
    {
        removeModalButton(guiButton);
        connections.erase(instance.get());
    }
}

bool ScreenGui::hasModalDialog()
{
    for (std::vector<GuiButton*>::iterator iter = modalGuiObjects.begin(); iter != modalGuiObjects.end(); ++iter)
    {
        if ((*iter)->isCurrentlyVisible())
            return true;
    }
    return false;
}

const char* const sGuiMain = "GuiMain";
GuiMain::GuiMain()
    : DescribedCreatable<GuiMain, ScreenGui, sGuiMain>(sGuiMain)
{
}

} // namespace Aya
