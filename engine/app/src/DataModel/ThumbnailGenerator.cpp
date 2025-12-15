#include "ThumbnailGenerator.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <memory>

#ifdef __linux
#include <SDL3/SDL.h>
#endif
#include <boost/algorithm/string.hpp>
#include "DataModel/Lighting.hpp"
#include "DataModel/ContentProvider.hpp"
#include "DataModel/TextureContentProvider.hpp"
#include "DataModel/MeshContentProvider.hpp"
#include "DataModel/SolidModelContentProvider.hpp"
#include "DataModel/DataModel.hpp"
#include "DataModel/Workspace.hpp"
#include "BinaryOutput.hpp"
#include "GImage.hpp"
#include "Base/ViewBase.hpp"
#include "Base/RenderSettings.hpp"
#include "Utility/base64.hpp"
#include "Utility/StandardOut.hpp"

using namespace Aya;


const char* const sThumbnailGenerator = "ThumbnailGenerator";

struct ThumbnailRenderSettings : public CRenderSettings
{
    ThumbnailRenderSettings()
    {
        antialiasingMode = AntialiasingOn;
        eagerBulkExecution = true;
        enableFRM = false;
    }
};

struct ThumbnailRenderRequest
{
    ThumbnailGenerator* generator;
    std::string fileType;
    int cx, cy;
    bool hideSky, crop;
    std::string* strOutput;
    std::string* errorOutput;
    CEvent* doneEvent;
};

static safe_queue<ThumbnailRenderRequest> gThumbRenderQueue;
static CEvent gThumbRenderQueueNotEmpty(false);
static boost::scoped_ptr<boost::thread> gThumbRenderThread;
static boost::once_flag gThumbRenderThreadInit = BOOST_ONCE_INIT;

static void thumbRenderWorker()
{
    // Create a dummy 1x1 window that holds the GL context and render state alive
    ThumbnailRenderSettings settings;
    OSContext context;
    context.height = 1;
    context.width = 1;
    context.hWnd = nullptr;

    ViewBase::InitPluginModules();
    boost::scoped_ptr<ViewBase> view(ViewBase::CreateView(CRenderSettings::OpenGL, &context, &settings));

    while (true)
    {
        gThumbRenderQueueNotEmpty.Wait();

        ThumbnailRenderRequest request;
        while (gThumbRenderQueue.pop_if_present(request))
        {
            AYAASSERT(request.generator && request.strOutput && request.errorOutput && request.doneEvent);

            try
            {
                // The requesting thread holds a lock for us and waits for doneEvent to be set
                DataModel::scoped_write_transfer writeTransfer(request.generator);

                if (boost::algorithm::to_lower_copy(request.fileType) == "obj")
                {
                    request.generator->exportScene(view.get(), request.strOutput);
                }
                else
                {
                    request.generator->renderThumb(
                        view.get(), request.fileType, request.cx, request.cy, request.hideSky, request.crop, request.strOutput);
                }

                request.errorOutput->clear();
            }
            catch (std::exception& e)
            {
                *request.errorOutput = "renderThumb failed: " + std::string(e.what());
            }

            request.doneEvent->Set();
        }
    }
}

static void thumbRenderInit()
{
    AYAASSERT(!gThumbRenderThread);
    gThumbRenderThread.reset(new boost::thread(thumbRenderWorker));
}

static Reflection::BoundFuncDesc<ThumbnailGenerator, shared_ptr<const Reflection::Tuple>(std::string, int, int, bool, bool)> clickFunction(
    &ThumbnailGenerator::click, "Click", "fileType", "width", "height", "hideSky", "crop", false, Security::LocalUser);

static Reflection::BoundFuncDesc<ThumbnailGenerator, shared_ptr<const Reflection::Tuple>(std::string, std::string, int, int)> clickTextureFunction(
    &ThumbnailGenerator::clickTexture, "ClickTexture", "textureId", "fileType", "width", "height", Security::LocalUser);

static Reflection::BoundFuncDesc<ThumbnailGenerator, shared_ptr<const Reflection::Tuple>(std::string, int, int, bool, bool)> saveToFileFunction(
    &ThumbnailGenerator::saveToFile, "SaveToFile", "filePath", "width", "height", "hideSky", "crop", false, Security::LocalUser);

volatile long ThumbnailGenerator::totalCount = 0;

ThumbnailGenerator::ThumbnailGenerator() {}

ThumbnailGenerator::~ThumbnailGenerator() {}

void ThumbnailGenerator::configureCaches()
{
    auto configureProvider = [this](auto* provider)
    {
        if (provider)
        {
            provider->setCacheSize(INT_MAX);
            if constexpr (requires { provider->setImmediateMode(); })
            {
                provider->setImmediateMode();
            }
        }
    };

    configureProvider(ServiceProvider::create<ContentProvider>(this));
    configureProvider(ServiceProvider::create<TextureContentProvider>(this));
    configureProvider(ServiceProvider::create<MeshContentProvider>(this));
    configureProvider(ServiceProvider::create<SolidModelContentProvider>(this));
}

shared_ptr<const Reflection::Tuple> ThumbnailGenerator::clickTexture(std::string textureId, std::string fileType, int cx, int cy)
{
    totalCount++;

    try
    {
        StandardOut::singleton()->printf(
            MESSAGE_INFO, "ThumbnailGenerator::clickTexture(%s, %s, %d, %d)", textureId.c_str(), fileType.c_str(), cx, cy);

        configureCaches();
        ContentProvider* contentProvider = ServiceProvider::create<ContentProvider>(this);

        G3D::BinaryOutput binaryOutput;
        binaryOutput.setEndian(G3D::G3D_LITTLE_ENDIAN);

        std::transform(fileType.begin(), fileType.end(), fileType.begin(), ::toupper);
        const G3D::GImage::Format format = G3D::GImage::stringToFormat(fileType);

        G3D::GImage image;
        boost::shared_ptr<const std::string> content = contentProvider->getContentString(ContentId(textureId));

        G3D::BinaryInput binaryInput(reinterpret_cast<const unsigned char*>(content->data()), content->size(), G3D::G3D_LITTLE_ENDIAN, false, false);
        image.decode(binaryInput, G3D::GImage::resolveFormat("", binaryInput.getCArray(), binaryInput.size(), G3D::GImage::AUTODETECT));

        if (format != G3D::GImage::PNG)
        {
            // alpha channel not requested, strip it.
            image = image.stripAlpha();
        }
        else
        {
            image.setColorAlphaTest(G3D::Color4uint8(255, 255, 255, 0));
        }

        if (image.width() > 0 && image.height() > 0)
        {
            image = image.bilinearStretchBlt(cx, cy);
        }
        else
        {
            // Create a dummy 1x1 image that we can save (PNG encode can't handle empty images)
            image.resize(1, 1, 4);
        }

        image.encode(format, binaryOutput);

        std::string strOut;
        base64<char>::encode((const char*)binaryOutput.getCArray(), binaryOutput.length(), strOut, base64<>::noline());

        StandardOut::singleton()->print(MESSAGE_INFO, "ThumbnailGenerator::clickTexture() success");

        shared_ptr<Reflection::Tuple> tuple(new Reflection::Tuple(2));
        tuple->values[0] = strOut;
        tuple->values[1] = contentProvider->getRequestedUrls();

        return tuple;
    }
    catch (const G3D::GImage::Error& e)
    {
        StandardOut::singleton()->print(MESSAGE_ERROR, e.reason);
        throw std::runtime_error(e.reason);
    }
    catch (std::exception& exp)
    {
        StandardOut::singleton()->print(MESSAGE_ERROR, exp);
        throw;
    }
}

shared_ptr<const Reflection::Tuple> ThumbnailGenerator::click(std::string fileType, int cx, int cy, bool hideSky, bool crop)
{
    totalCount++;

    try
    {
        std::string strOutput, errorOutput;
        ContentProvider* contentProvider = ServiceProvider::create<ContentProvider>(this);
        configureCaches();

        {
            // We need to reuse graphics state between runs; this means reusing Mesa GL context.
            // There are two reasons why we'd like *all* render requests to come off a single thread:
            // 1. The window that's created in a thread dies, and DC dies with it - we need to keep the main window alive
            // 2. Mesa GL context is bound to a thread that created it; if a new thread is created, graphics can't reuse old GL context.
            CEvent doneEvent(true);
            ThumbnailRenderRequest request{this, fileType, cx, cy, hideSky, crop, &strOutput, &errorOutput, &doneEvent};

            boost::call_once(thumbRenderInit, gThumbRenderThreadInit);

            gThumbRenderQueue.push(request);
            gThumbRenderQueueNotEmpty.Set();
            doneEvent.Wait();

            if (!errorOutput.empty())
            {
                // render request threw an exception; "marshal" it here (we lose the stack, but the exception gets converted to Lua error anyway)
                throw std::runtime_error(errorOutput);
            }
        }

        shared_ptr<Reflection::Tuple> tuple(new Reflection::Tuple(2));
        tuple->values[0] = strOutput;
        tuple->values[1] = contentProvider->getRequestedUrls();

        return tuple;
    }
    catch (std::exception& exp)
    {
        StandardOut::singleton()->print(MESSAGE_ERROR, exp);
        throw;
    }
}

shared_ptr<const Reflection::Tuple> ThumbnailGenerator::saveToFile(std::string filePath, int cx, int cy, bool hideSky, bool crop)
{
    // extract fileType from filePath extension
    std::string fileType = boost::filesystem::path(filePath).extension().string();
    if (!fileType.empty() && fileType[0] == '.')
        fileType = fileType.substr(1);
    if (fileType.empty())
        throw std::runtime_error("ThumbnailGenerator::saveToFile: filePath has no extension");

    try
    {
        StandardOut::singleton()->printf(
            MESSAGE_INFO, "ThumbnailGenerator::saveToFile(%s, %d, %d, %s)", filePath.c_str(), cx, cy, hideSky ? "true" : "false");

        // Generate the thumbnail using click()
        auto tuple = click(fileType, cx, cy, hideSky, crop);
        std::string base64Data = tuple->values[0].cast<std::string>();

        // Decode and save to file
        std::string decoded;
        base64<char> decoder;
        int state = 0;
        decoder.get(base64Data.begin(), base64Data.end(), std::back_inserter(decoded), state);

        std::ofstream outFile(filePath, std::ios::binary);
        if (!outFile.is_open())
            throw std::runtime_error("Failed to open file for writing: " + filePath);
        outFile.write(decoded.data(), decoded.size());

        return tuple;
    }
    catch (std::exception& exp)
    {
        StandardOut::singleton()->print(MESSAGE_ERROR, exp);
        throw;
    }
}

void ThumbnailGenerator::exportScene(ViewBase* view, std::string* outStr)
{
    Workspace* workspace = ServiceProvider::find<Workspace>(this);
    // just fill it with a bunch of dummies
    bool allowDolly = workspace->setImageServerView(false);

    DataModel* dataModel = boost::polymorphic_downcast<DataModel*>(getParent());
    view->bindWorkspace(shared_from(dataModel));
    view->exportSceneThumbJSON(ExporterSaveType_Everything, ExporterFormat_Obj, true, *outStr);
    view->bindWorkspace(shared_ptr<DataModel>());
}

void ThumbnailGenerator::renderThumb(ViewBase* view, std::string fileType, int cx, int cy, bool hideSky, bool crop, std::string* strOutput)
{
    const G3D::GImage::Format format = G3D::GImage::stringToFormat(fileType);
    bool alphaChannel = false;

    // TODO: put this code into an API to Lighting???
    if (hideSky)
    {
        Lighting* lighting = ServiceProvider::create<Lighting>(this);
        lighting->suppressSky(true);
        if (format == G3D::GImage::PNG)
        {
            alphaChannel = true;
            lighting->setClearAlpha(0);
        }
        else
        {
            lighting->setClearAlpha(1);
        }
    }

    // TODO: put this code into an API to Camera
    Workspace* workspace = ServiceProvider::find<Workspace>(this);

    // Camera Adjustments:
    // use camera named "ThumbnailCamera" else:
    //	for a model / bunch of models - look from -Z
    //  for a place: not much.
    bool allowDolly = workspace->setImageServerView(!hideSky /* = isAPlace*/);

    {
        DataModel* dataModel = boost::polymorphic_downcast<DataModel*>(getParent());
        G3D::GImage image(cx, cy, 4);

        view->bindWorkspace(shared_from(dataModel));
        view->renderThumb(image.byte(), cx, cy, crop, allowDolly);
        view->bindWorkspace(shared_ptr<DataModel>());

        // causes crash. Don't want to deal with it. BYE BYE!
        // view->garbageCollect();

        if (!alphaChannel)
            image.convertToRGB();

        if (strOutput)
        {
            G3D::BinaryOutput binaryOutput;
            binaryOutput.setEndian(G3D::G3D_LITTLE_ENDIAN);
            image.encode(format, binaryOutput);
            base64<char>::encode((const char*)binaryOutput.getCArray(), binaryOutput.length(), *strOutput, base64<>::noline());
        }

        if (hideSky)
        {
            // Hack to avoid double-toggle
            workspace->setImageServerView(false);
        }
    }
}