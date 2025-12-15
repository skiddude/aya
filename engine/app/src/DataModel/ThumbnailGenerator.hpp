#pragma once

#include "Tree/Instance.hpp"
#include "Tree/Service.hpp"

namespace G3D
{
class BinaryOutput;
};

namespace Aya
{
class ContentProvider;
class ViewBase;
} // namespace Aya

extern const char* const sThumbnailGenerator;
class ThumbnailGenerator
    : public Aya::DescribedCreatable<ThumbnailGenerator, Aya::Instance, sThumbnailGenerator>
    , public Aya::Service
{
public:
    static volatile long totalCount;

    ThumbnailGenerator(void);
    ~ThumbnailGenerator(void);

    shared_ptr<const Aya::Reflection::Tuple> click(std::string fileType, int cx, int cy, bool hideSky, bool crop);
    shared_ptr<const Aya::Reflection::Tuple> clickTexture(std::string textureId, std::string fileType, int cx, int cy);
    shared_ptr<const Aya::Reflection::Tuple> saveToFile(std::string filePath, int cx, int cy, bool hideSky, bool crop);

    void renderThumb(Aya::ViewBase* view, std::string fileType, int cx, int cy, bool hideSky, bool crop, std::string* strOutput);
    void exportScene(Aya::ViewBase* view, std::string* outStr);

private:
    void configureCaches();
};
