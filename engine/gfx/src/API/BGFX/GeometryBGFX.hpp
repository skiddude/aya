#pragma once

#include "Core/Geometry.hpp"
#include <bgfx/bgfx.h>

namespace Aya
{
namespace Graphics
{

class DeviceBGFX;

class VertexLayoutBGFX : public VertexLayout
{
public:
    VertexLayoutBGFX(Device* device, const std::vector<Element>& elements);
    ~VertexLayoutBGFX();

    const bgfx::VertexLayout& getBGFXLayout() const
    {
        return layout;
    }

    static bgfx::Attrib::Enum getAttribType(unsigned int semanticId);

private:
    bgfx::VertexLayout layout;
};

template<typename Base>
class GeometryBufferBGFX : public Base
{
public:
    GeometryBufferBGFX(Device* device, size_t elementSize, size_t elementCount, GeometryBuffer::Usage usage, bool isIndex);
    ~GeometryBufferBGFX();

    virtual void* lock(GeometryBuffer::LockMode mode);
    virtual void unlock();

    virtual void upload(unsigned int offset, const void* data, unsigned int size);

    void createVertexBuffer(const bgfx::VertexLayout& layout);

    bgfx::DynamicVertexBufferHandle getVertexHandle() const
    {
        return vertexHandle;
    }
    bgfx::DynamicIndexBufferHandle getIndexHandle() const
    {
        return indexHandle;
    }

    const bgfx::Memory* getMemory() const
    {
        return memory;
    }

protected:
    void create();

private:
    bool isIndex;
    bgfx::DynamicVertexBufferHandle vertexHandle;
    bgfx::DynamicIndexBufferHandle indexHandle;
    const bgfx::Memory* memory;
    void* locked;
};

class VertexBufferBGFX : public GeometryBufferBGFX<VertexBuffer>
{
public:
    VertexBufferBGFX(Device* device, size_t elementSize, size_t elementCount, Usage usage);
    ~VertexBufferBGFX();
};

class IndexBufferBGFX : public GeometryBufferBGFX<IndexBuffer>
{
public:
    IndexBufferBGFX(Device* device, size_t elementSize, size_t elementCount, Usage usage);
    ~IndexBufferBGFX();
};

class GeometryBGFX : public Geometry
{
public:
    GeometryBGFX(Device* device, const shared_ptr<VertexLayout>& layout, const std::vector<shared_ptr<VertexBuffer>>& vertexBuffers,
        const shared_ptr<IndexBuffer>& indexBuffer, unsigned int baseVertexIndex);
    ~GeometryBGFX();

    void bindBuffers(unsigned int offset, unsigned int count);
    void draw(bgfx::ViewId view, Primitive primitive, unsigned int offset, unsigned int count);

private:
    unsigned int indexElementSize;

    static uint64_t convertPrimitive(Primitive primitive);
};

} // namespace Graphics
} // namespace Aya
