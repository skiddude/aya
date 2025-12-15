#include "GeometryBGFX.hpp"
#include "DeviceBGFX.hpp"

#include <bgfx/bgfx.h>

namespace Aya
{
namespace Graphics
{

bgfx::Attrib::Enum VertexLayoutBGFX::getAttribType(unsigned int semanticId)
{
    switch (semanticId)
    {
    case 0: // Position
        return bgfx::Attrib::Position;
    case 1: // Normal
        return bgfx::Attrib::Normal;
    case 2: // Color0
        return bgfx::Attrib::Color0;
    case 3: // Color1
        return bgfx::Attrib::Color1;
    case 4: // TexCoord0
        return bgfx::Attrib::TexCoord0;
    case 5: // TexCoord1
        return bgfx::Attrib::TexCoord1;
    case 6: // TexCoord2
        return bgfx::Attrib::TexCoord2;
    case 7: // TexCoord3
        return bgfx::Attrib::TexCoord3;
    case 8: // TexCoord4
        return bgfx::Attrib::TexCoord4;
    case 9: // TexCoord5
        return bgfx::Attrib::TexCoord5;
    case 10: // TexCoord6
        return bgfx::Attrib::TexCoord6;
    case 11: // TexCoord7
        return bgfx::Attrib::TexCoord7;
    default:
        return bgfx::Attrib::Count;
    }
}

static bgfx::AttribType::Enum getAttribFormat(VertexLayout::Format format)
{
    switch (format)
    {
    case VertexLayout::Format_Float1:
    case VertexLayout::Format_Float2:
    case VertexLayout::Format_Float3:
    case VertexLayout::Format_Float4:
        return bgfx::AttribType::Float;
    case VertexLayout::Format_Short2:
    case VertexLayout::Format_Short4:
        return bgfx::AttribType::Int16;
    case VertexLayout::Format_UByte4:
    case VertexLayout::Format_Color:
        return bgfx::AttribType::Uint8;
    default:
        return bgfx::AttribType::Count;
    }
}

static uint8_t getAttribCount(VertexLayout::Format format)
{
    switch (format)
    {
    case VertexLayout::Format_Float1:
        return 1;
    case VertexLayout::Format_Float2:
    case VertexLayout::Format_Short2:
        return 2;
    case VertexLayout::Format_Float3:
        return 3;
    case VertexLayout::Format_Float4:
    case VertexLayout::Format_Short4:
    case VertexLayout::Format_UByte4:
    case VertexLayout::Format_Color:
        return 4;
    default:
        return 0;
    }
}

static bool isNormalized(VertexLayout::Format format)
{
    return format == VertexLayout::Format_Color;
}

static unsigned int getVertexAttributeId(VertexLayout::Semantic semantic, unsigned int index)
{
    switch (semantic)
    {
    case VertexLayout::Semantic_Position:
        AYAASSERT(index == 0);
        return 0;
    case VertexLayout::Semantic_Normal:
        AYAASSERT(index == 0);
        return 1;
    case VertexLayout::Semantic_Color:
        AYAASSERT(index < 2);
        return 2 + index;
    case VertexLayout::Semantic_Texture:
        AYAASSERT(index < 8);
        return 4 + index;
    default:
        AYAASSERT(false);
        return 0;
    }
}

VertexLayoutBGFX::VertexLayoutBGFX(Device* device, const std::vector<Element>& elements)
    : VertexLayout(device, elements)
{
    layout.begin();

    for (size_t i = 0; i < elements.size(); ++i)
    {
        const Element& e = elements[i];

        unsigned int attribId = getVertexAttributeId(e.semantic, e.semanticIndex);
        bgfx::Attrib::Enum attrib = getAttribType(attribId);
        bgfx::AttribType::Enum type = getAttribFormat(e.format);
        uint8_t count = getAttribCount(e.format);
        bool normalized = isNormalized(e.format);

        layout.add(attrib, count, type, normalized);
    }

    layout.end();
}

VertexLayoutBGFX::~VertexLayoutBGFX() {}

template<typename Base>
GeometryBufferBGFX<Base>::GeometryBufferBGFX(Device* device, size_t elementSize, size_t elementCount, GeometryBuffer::Usage usage, bool isIndex)
    : Base(device, elementSize, elementCount, usage)
    , isIndex(isIndex)
    , memory(nullptr)
    , locked(nullptr)
{
    vertexHandle = BGFX_INVALID_HANDLE;
    indexHandle = BGFX_INVALID_HANDLE;
}

template<typename Base>
void GeometryBufferBGFX<Base>::create()
{
    uint32_t size = this->elementSize * this->elementCount;

    if (isIndex)
    {
        uint16_t flags = (this->usage == GeometryBuffer::Usage_Dynamic) ? BGFX_BUFFER_NONE : BGFX_BUFFER_NONE;
        indexHandle = bgfx::createDynamicIndexBuffer(this->elementCount, flags);
    }
    else
    {
        // Vertex buffers need a layout - we'll create the handle later in GeometryBGFX
        // when we have access to the actual layout
        vertexHandle = BGFX_INVALID_HANDLE;
    }
}

template<typename Base>
void GeometryBufferBGFX<Base>::createVertexBuffer(const bgfx::VertexLayout& layout)
{
    AYAASSERT(!isIndex);
    AYAASSERT(!bgfx::isValid(vertexHandle));

    uint16_t flags = (this->usage == GeometryBuffer::Usage_Dynamic) ? BGFX_BUFFER_NONE : BGFX_BUFFER_NONE;
    vertexHandle = bgfx::createDynamicVertexBuffer(this->elementCount, layout, flags);
}

template<typename Base>
GeometryBufferBGFX<Base>::~GeometryBufferBGFX()
{
    AYAASSERT(!locked);

    if (bgfx::isValid(vertexHandle))
    {
        bgfx::destroy(vertexHandle);
    }

    if (bgfx::isValid(indexHandle))
    {
        bgfx::destroy(indexHandle);
    }
}

template<typename Base>
void* GeometryBufferBGFX<Base>::lock(GeometryBuffer::LockMode mode)
{
    AYAASSERT(!locked);

    unsigned int size = this->elementSize * this->elementCount;

    // Allocate temporary buffer
    locked = new char[size];

    AYAASSERT(locked);

    return locked;
}

template<typename Base>
void GeometryBufferBGFX<Base>::unlock()
{
    AYAASSERT(locked);

    // Upload data to BGFX
    upload(0, locked, this->elementSize * this->elementCount);

    delete[] static_cast<char*>(locked);
    locked = nullptr;
}

template<typename Base>
void GeometryBufferBGFX<Base>::upload(unsigned int offset, const void* data, unsigned int size)
{
    AYAASSERT(!locked);
    AYAASSERT(offset + size <= this->elementSize * this->elementCount);

    const bgfx::Memory* mem = bgfx::copy(static_cast<const char*>(data) + offset, size);

    if (isIndex)
    {
        if (bgfx::isValid(indexHandle))
        {
            bgfx::update(indexHandle, 0, mem);
        }
    }
    else
    {
        if (bgfx::isValid(vertexHandle))
        {
            bgfx::update(vertexHandle, 0, mem);
        }
    }
}

VertexBufferBGFX::VertexBufferBGFX(Device* device, size_t elementSize, size_t elementCount, Usage usage)
    : GeometryBufferBGFX<VertexBuffer>(device, elementSize, elementCount, usage, false)
{
    create();
}

VertexBufferBGFX::~VertexBufferBGFX() {}

IndexBufferBGFX::IndexBufferBGFX(Device* device, size_t elementSize, size_t elementCount, Usage usage)
    : GeometryBufferBGFX<IndexBuffer>(device, elementSize, elementCount, usage, true)
{
    if (elementSize != 2 && elementSize != 4)
        throw Aya::runtime_error("Invalid element size: %d", (int)elementSize);

    create();
}

IndexBufferBGFX::~IndexBufferBGFX() {}

GeometryBGFX::GeometryBGFX(Device* device, const shared_ptr<VertexLayout>& layout, const std::vector<shared_ptr<VertexBuffer>>& vertexBuffers,
    const shared_ptr<IndexBuffer>& indexBuffer, unsigned int baseVertexIndex)
    : Geometry(device, layout, vertexBuffers, indexBuffer, baseVertexIndex)
    , indexElementSize(0)
{
    if (indexBuffer)
    {
        indexElementSize = indexBuffer->getElementSize();
    }

    // Create vertex buffers with proper layout
    VertexLayoutBGFX* layoutBGFX = static_cast<VertexLayoutBGFX*>(layout.get());
    const bgfx::VertexLayout& bgfxLayout = layoutBGFX->getBGFXLayout();

    for (size_t i = 0; i < vertexBuffers.size(); ++i)
    {
        VertexBufferBGFX* vb = static_cast<VertexBufferBGFX*>(vertexBuffers[i].get());

        // Create the vertex buffer handle with the layout if not already created
        if (!bgfx::isValid(vb->getVertexHandle()))
        {
            vb->createVertexBuffer(bgfxLayout);
        }
    }
}

GeometryBGFX::~GeometryBGFX() {}

void GeometryBGFX::bindBuffers(unsigned int offset, unsigned int count)
{
    // Set vertex buffers
    for (size_t i = 0; i < vertexBuffers.size(); ++i)
    {
        VertexBufferBGFX* vb = static_cast<VertexBufferBGFX*>(vertexBuffers[i].get());

        if (bgfx::isValid(vb->getVertexHandle()))
        {
            bgfx::setVertexBuffer(i, vb->getVertexHandle(), baseVertexIndex, vb->getElementCount());
        }
    }

    // Set index buffer if present
    if (indexBuffer)
    {
        IndexBufferBGFX* ib = static_cast<IndexBufferBGFX*>(indexBuffer.get());

        if (bgfx::isValid(ib->getIndexHandle()))
        {
            bgfx::setIndexBuffer(ib->getIndexHandle(), offset, count);
        }
    }
}

uint64_t GeometryBGFX::convertPrimitive(Primitive primitive)
{
    // This function is not used in BGFX - primitives are set via state flags
    // Keeping for potential future use
    switch (primitive)
    {
    case Primitive_Triangles:
        return BGFX_STATE_PT_TRISTRIP;
    case Primitive_Lines:
        return BGFX_STATE_PT_LINES;
    case Primitive_Points:
        return BGFX_STATE_PT_POINTS;
    case Primitive_TriangleStrip:
        return BGFX_STATE_PT_TRISTRIP;
    default:
        return BGFX_STATE_PT_TRISTRIP;
    }
}

void GeometryBGFX::draw(bgfx::ViewId view, Primitive primitive, unsigned int offset, unsigned int count)
{
    // Legacy method - not used anymore
    // Drawing is now handled by bindBuffers() + bgfx::submit() in DeviceContextBGFX::drawImpl()
}

// Explicit template instantiation
template class GeometryBufferBGFX<VertexBuffer>;
template class GeometryBufferBGFX<IndexBuffer>;

} // namespace Graphics
} // namespace Aya
