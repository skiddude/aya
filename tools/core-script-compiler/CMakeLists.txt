add_executable(CoreScriptCompiler src/main.cpp)

target_link_libraries(CoreScriptCompiler
  3D
  AppServer
  Core
  RakNet
  BulletPhysics
  NetworkServer
  Graphics
)

if(AYA_OS_WINDOWS)
  target_sources(CoreScriptCompiler PRIVATE resources/winrc.h resources/script.rc)
endif()

target_include_directories(CoreScriptCompiler PRIVATE src resources)
set_target_properties(CoreScriptCompiler PROPERTIES OUTPUT_NAME "Aya.CoreScriptCompiler")

add_custom_target(CompileCoreScripts 
  $<TARGET_FILE:CoreScriptCompiler> --source "${CLIENT_DIR}/common/content/scripts" --output "${ENGINE_DIR}/app/src/Script/LuaGenCS.inl" 
  DEPENDS CoreScriptCompiler
  COMMENT "Compiling CoreScript bytecode"
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_custom_target(CompileShaders
  python ${CLIENT_DIR}/common/shaders/compile_shaders.py --packs glsl3 glsles3 --bgfx-include "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/include"
  COMMENT "Compiling shaders"
  WORKING_DIRECTORY "${CLIENT_DIR}/common/shaders"
)

add_custom_command(TARGET CompileCoreScripts POST_BUILD
  COMMENT "Copying runtime files to build directory"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${RUNTIME_FILES}"
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)