add_library(Graphics STATIC)

set(SOURCES
  # GfxBase
  src/Base/Adorn.cpp
  src/Base/Adorn.hpp
  src/Base/AdornBillboarder.cpp
  src/Base/AdornBillboarder.hpp
  src/Base/AdornBillboarder2D.cpp
  src/Base/AdornBillboarder2D.hpp
  src/Base/AdornSurface.cpp
  src/Base/AdornSurface.hpp
  src/Base/AsyncResult.hpp
  src/Base/FileMeshData.cpp
  src/Base/FileMeshData.hpp
  src/Base/FrameRateManager.cpp
  src/Base/FrameRateManager.hpp
  src/Base/GfxPart.cpp
  src/Base/GfxPart.hpp
  src/Base/IAdornable.hpp
  src/Base/IAdornableCollector.cpp
  src/Base/IAdornableCollector.hpp
  src/Base/Image.hpp
  src/Base/MeshFileStructs.hpp
  src/Base/MeshGen.hpp
  src/Base/ObjLoader.hpp
  src/Base/Part.hpp
  src/Base/PartIdentifier.cpp
  src/Base/PartIdentifier.hpp
  src/Base/RenderCaps.cpp
  src/Base/RenderCaps.hpp
  src/Base/RenderSettings.cpp
  src/Base/RenderSettings.hpp
  src/Base/RenderStats.cpp
  src/Base/RenderStats.hpp
  src/Base/TextureProxyBase.hpp
  src/Base/Type.hpp
  src/Base/Typesetter.hpp
  src/Base/ViewBase.cpp
  src/Base/ViewBase.hpp
  src/Base/ViewportBillboarder.cpp
  src/Base/ViewportBillboarder.hpp

  # GfxCore
  src/Core/Device.cpp
  src/Core/Device.hpp
  src/Core/DeviceCreate.cpp
  src/Core/Framebuffer.cpp
  src/Core/Framebuffer.hpp
  src/Core/Geometry.cpp
  src/Core/Geometry.hpp
  src/Core/Pix.cpp
  src/Core/Pix.hpp
  src/Core/Resource.cpp
  src/Core/Resource.hpp
  src/Core/Shader.cpp
  src/Core/Shader.hpp
  src/Core/States.cpp
  src/Core/States.hpp
  src/Core/Texture.cpp
  src/Core/Texture.hpp

  # GfxRender
  src/Render/AdornRender.cpp
  src/Render/AdornRender.hpp
  src/Render/CullableSceneNode.cpp
  src/Render/CullableSceneNode.hpp
  src/Render/CustomEmitter.cpp
  src/Render/CustomEmitter.hpp
  src/Render/Emitter.cpp
  src/Render/Emitter.hpp
  src/Render/EmitterShared.hpp
  src/Render/EnvMap.cpp
  src/Render/EnvMap.hpp
  src/Render/ExplosionEmitter.cpp
  src/Render/ExplosionEmitter.hpp
  src/Render/FastCluster.cpp
  src/Render/FastCluster.hpp
  src/Render/GeometryGenerator.cpp
  src/Render/GeometryGenerator.hpp
  src/Render/GlobalShaderData.cpp
  src/Render/GlobalShaderData.hpp
  src/Render/Image.cpp
  src/Render/Image.hpp
  src/Render/ImageInfo.hpp
  src/Render/LightGrid.cpp
  src/Render/LightGrid.hpp
  src/Render/LightObject.cpp
  src/Render/LightObject.hpp
  src/Render/Material.cpp
  src/Render/Material.hpp
  src/Render/MaterialGenerator.cpp
  src/Render/MaterialGenerator.hpp
  src/Render/MegaCluster.cpp
  src/Render/MegaCluster.hpp
  src/Render/ObjectExporter.cpp
  src/Render/ObjectExporter.hpp
  src/Render/ParticleEmitter.cpp
  src/Render/ParticleEmitter.hpp
  src/Render/RenderCamera.cpp
  src/Render/RenderCamera.hpp
  src/Render/RenderNode.cpp
  src/Render/RenderNode.hpp
  src/Render/RenderQueue.cpp
  src/Render/RenderQueue.hpp
  src/Render/RenderView.cpp
  src/Render/RenderView.hpp
  src/Render/SceneManager.cpp
  src/Render/SceneManager.hpp
  src/Render/SceneUpdater.cpp
  src/Render/SceneUpdater.hpp
  src/Render/ScreenSpaceEffect.cpp
  src/Render/ScreenSpaceEffect.hpp
  src/Render/ShaderManager.cpp
  src/Render/ShaderManager.hpp
  src/Render/Sky.cpp
  src/Render/Sky.hpp
  src/Render/SmoothCluster.cpp
  src/Render/SmoothCluster.hpp
  src/Render/SpatialGrid.hpp
  src/Render/SpatialHashedScene.cpp
  src/Render/SpatialHashedScene.hpp
  src/Render/SSAO.cpp
  src/Render/SSAO.hpp
  src/Render/SuperCluster.cpp
  src/Render/SuperCluster.hpp
  src/Render/TextureAtlas.cpp
  src/Render/TextureAtlas.hpp
  src/Render/TextureCompositor.cpp
  src/Render/TextureCompositor.hpp
  src/Render/TextureManager.cpp
  src/Render/TextureManager.hpp
  src/Render/TextureRef.cpp
  src/Render/TextureRef.hpp
  src/Render/TypesetterDynamic.cpp
  src/Render/TypesetterDynamic.hpp
  src/Render/Util.hpp
  src/Render/VertexStreamer.cpp
  src/Render/VertexStreamer.hpp
  src/Render/VisualEngine.cpp
  src/Render/VisualEngine.hpp
  src/Render/Water.cpp
  src/Render/Water.hpp
)

# BGFX API integration
list(APPEND SOURCES
  src/API/BGFX/DeviceContextBGFX.cpp
  src/API/BGFX/DeviceBGFX.cpp
  src/API/BGFX/DeviceBGFX.hpp
  src/API/BGFX/FramebufferBGFX.cpp
  src/API/BGFX/FramebufferBGFX.hpp
  src/API/BGFX/GeometryBGFX.cpp
  src/API/BGFX/GeometryBGFX.hpp
  src/API/BGFX/HeadersBGFX.hpp
  src/API/BGFX/ShaderBGFX.cpp
  src/API/BGFX/ShaderBGFX.hpp
  src/API/BGFX/TextureBGFX.cpp
  src/API/BGFX/TextureBGFX.hpp
)

# OpenGL API integration
list(APPEND SOURCES
  src/API/GL/ContextGL.hpp
  src/API/GL/DeviceContextGL.cpp
  src/API/GL/DeviceGL.cpp
  src/API/GL/DeviceGL.hpp
  src/API/GL/FramebufferGL.cpp
  src/API/GL/FramebufferGL.hpp
  src/API/GL/GeometryGL.cpp
  src/API/GL/GeometryGL.hpp
  src/API/GL/HeadersGL.hpp
  src/API/GL/ShaderGL.cpp
  src/API/GL/ShaderGL.hpp
  src/API/GL/TextureGL.cpp
  src/API/GL/TextureGL.hpp
  src/API/GL/glad/include/glad/gl.h
  src/API/GL/glad/src/gl.c
)

if(AYA_OS_WINDOWS)
  list(APPEND SOURCES 
    src/API/GL/glad/include/glad/wgl.h
    src/API/GL/glad/src/wgl.c
    src/API/GL/ContextGLWin32.cpp
  )
elseif(AYA_OS_LINUX)
  list(APPEND SOURCES 
    src/API/GL/glad/include/EGL/eglplatform.h
    src/API/GL/glad/include/glad/egl.h
    src/API/GL/glad/src/egl.c
    src/API/GL/ContextEGL.cpp
  )
elseif(AYA_OS_MACOS)
  list(APPEND SOURCES src/API/GL/ContextGLMac.mm)
elseif(AYA_OS_ANDROID)
  list(APPEND SOURCES src/API/GL/ContextGLAndroid.cpp)
endif()

target_sources(Graphics PRIVATE ${SOURCES})

target_include_directories(Graphics
  PUBLIC
    ${ENGINE_DIR}/3d/src
    src
  PRIVATE
    src/API/GL/glad/include
    ${THIRD_PARTY_DIR}/BulletPhysics/src
    ${ENGINE_DIR}/app/src
    ${ENGINE_DIR}/core/src
)

target_link_libraries(Graphics $<TARGET_OBJECTS:Core> $<TARGET_OBJECTS:3D>)