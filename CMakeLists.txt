cmake_minimum_required(VERSION 3.25)

# Set up project
project(Aya LANGUAGES CXX)

# --- Project Options ---

# Enable Modern C++ standard
set(CMAKE_CXX_STANDARD 17) # Upgraded to C++17 to support dependencies cleanly
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Setup paths
set(THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/third-party")
set(ENGINE_DIR      "${CMAKE_SOURCE_DIR}/engine")
set(CLIENT_DIR      "${CMAKE_SOURCE_DIR}/client")
set(TOOLS_DIR       "${CMAKE_SOURCE_DIR}/tools")

# Version Configuration
set(AYA_VERSION_MAJOR 1)
set(AYA_VERSION_MINOR 0)
set(AYA_VERSION_PATCH 0)
set(AYA_VERSION_TYPE "beta")

set(AYA_PROJECT_NAME "Aya" CACHE STRING "Public-facing project name")

option(AYA_TEST_BUILD "Enable testing utilities" ON)
option(ADDRESS_SANITIZER "Enable address sanitization" OFF)
option(ENABLE_CHROMIUM_FRAMES "Enable ChromiumFrames with CEF integration" OFF)
set(CEF_VERSION "140.1.14+geb1c06e+chromium-140.0.7339.185" CACHE STRING "CEF version to use")
option(BUILD_SERVER "Build the Server target" ON)
option(BUILD_PLAYER "Build the player client" ON)
option(BUILD_STUDIO "Build the Studio target" OFF)
option(ENABLE_VOICE_CHAT "Enable voice chat" OFF)
option(ENABLE_DISCORD_INTEGRATION "Enable Discord Rich Presence integration" OFF)
option(USE_CEF_FIND_PACKAGE "Use find_package for CEF" OFF)

# Create interface library for core compiler options (Modern CMake Target approach)
add_library(AyaCompilerOptions INTERFACE)

# Build Timestamp logic
if(NOT DEFINED AYA_BUILD_TIMESTAMP)
  if(AYA_TEST_BUILD)
    set(_AYA_BUILD_TIMESTAMP "0")
  else()
    string(TIMESTAMP _AYA_BUILD_TIMESTAMP "%s" UTC)
  endif()
  set(AYA_BUILD_TIMESTAMP "${_AYA_BUILD_TIMESTAMP}" CACHE STRING "Aya build timestamp")
endif()

# Platform and Architecture Detection
if("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^(arm64|ARM64|aarch64|AARCH64)$")
  set(AYA_ARCH_ARM64 1)
  target_compile_definitions(AyaCompilerOptions INTERFACE AYA_ARCH_ARM64=1 _ARM64_=1)
  set(AYA_PLATFORM_NAME "(arm64)")
elseif("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^(x86_64|X86_64|amd64|AMD64)$")
  set(AYA_ARCH_X64 1)
  target_compile_definitions(AyaCompilerOptions INTERFACE AYA_ARCH_X64=1 _AMD64_=1)
  set(AYA_PLATFORM_NAME "(x64)")
else()
  message(FATAL_ERROR "Aya: Unsupported architecture ${CMAKE_SYSTEM_PROCESSOR}")
endif()

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  set(AYA_OS_WINDOWS 1)
  target_compile_definitions(AyaCompilerOptions INTERFACE AYA_OS_WINDOWS)
  set(AYA_PLATFORM_NAME "Windows ${AYA_PLATFORM_NAME}")
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
  set(AYA_OS_LINUX 1)
  target_compile_definitions(AyaCompilerOptions INTERFACE AYA_OS_LINUX)
  set(AYA_PLATFORM_NAME "Linux ${AYA_PLATFORM_NAME}")
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
  set(AYA_OS_MACOS 1)
  target_compile_definitions(AyaCompilerOptions INTERFACE AYA_OS_MACOS)
  set(AYA_PLATFORM_NAME "MacOS ${AYA_PLATFORM_NAME}")
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Android")
  set(AYA_OS_ANDROID 1)
  target_compile_definitions(AyaCompilerOptions INTERFACE AYA_OS_ANDROID)
  set(AYA_PLATFORM_NAME "Android ${AYA_PLATFORM_NAME}")
else()
  message(FATAL_ERROR "Aya: Unsupported system ${CMAKE_SYSTEM_NAME}")
endif()

# Global Project Definitions bound to compile options
target_compile_definitions(AyaCompilerOptions INTERFACE
  AYA_PROJECT_NAME="${AYA_PROJECT_NAME}"
  AYA_COMPILER_ID="${CMAKE_CXX_COMPILER_ID}"
  AYA_VERSION_MAJOR=${AYA_VERSION_MAJOR}
  AYA_VERSION_MAJOR_STR="${AYA_VERSION_MAJOR}"
  AYA_VERSION_MINOR=${AYA_VERSION_MINOR}
  AYA_VERSION_MINOR_STR="${AYA_VERSION_MINOR}"
  AYA_VERSION_PATCH=${AYA_VERSION_PATCH}
  AYA_VERSION_PATCH_STR="${AYA_VERSION_PATCH}"
  AYA_VERSION_TYPE="${AYA_VERSION_TYPE}"
  AYA_BUILD_TIMESTAMP=${AYA_BUILD_TIMESTAMP}
  AYA_PLATFORM_NAME="${AYA_PLATFORM_NAME}"
)

if(AYA_TEST_BUILD AND NOT CI)
  target_compile_definitions(AyaCompilerOptions INTERFACE AYA_TEST_BUILD=1)
endif()

# --- Manifest & Feature Flag Mapping ---

# Conditionally feed features to vcpkg.json
if(BUILD_SERVER)
  list(APPEND VCPKG_MANIFEST_FEATURES "server")
endif()

if(BUILD_PLAYER)
  list(APPEND VCPKG_MANIFEST_FEATURES "player")
endif()

if(BUILD_STUDIO)
  list(APPEND VCPKG_MANIFEST_FEATURES "studio" "player" "server") # Implicitly request bundles safely
endif()

if(ENABLE_DISCORD_INTEGRATION)
  list(APPEND VCPKG_MANIFEST_FEATURES "discord" "player")
endif()

if(ENABLE_VOICE_CHAT)
  list(APPEND VCPKG_MANIFEST_FEATURES "voice" "player")
endif()

# Chromium CEF Sanitization on platforms where it's missing
if(AYA_OS_ANDROID OR (AYA_OS_MACOS AND CI))
  set(ENABLE_CHROMIUM_FRAMES OFF)
endif()

if(ENABLE_CHROMIUM_FRAMES)
  target_compile_definitions(AyaCompilerOptions INTERFACE ENABLE_CHROMIUM_FRAMES=1 CEF_VERSION="${CEF_VERSION}")
  set(CEF_CMAKE_LOCALES_OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/cef/locales")
  set(CEF_CMAKE_RESOURCES_OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/cef/resources")
endif()

# Compiler optimization and warning setups linked only to compiler option target
if(NOT AYA_ARCH_ARM64)
  target_compile_options(AyaCompilerOptions INTERFACE -mssse3)
endif()

if(ADDRESS_SANITIZER AND NOT AYA_ARCH_ARM64)
  target_compile_options(AyaCompilerOptions INTERFACE -fsanitize=address)
  target_link_options(AyaCompilerOptions INTERFACE -fsanitize=address)
endif()

if(ENABLE_VOICE_CHAT)
  target_compile_definitions(AyaCompilerOptions INTERFACE ENABLE_VOICE_CHAT=1)
endif()

# Warning suppressions
target_compile_options(AyaCompilerOptions INTERFACE
  -Wnonportable-include-path
  -Wundefined-var-template
  -Wdelete-abstract-non-virtual-dtor
)

# --- Windows/MSVC Target-Based Configurations ---

target_compile_definitions(AyaCompilerOptions INTERFACE
  _HAS_AUTO_PTR_ETC=1
  _ENFORCE_MATCHING_ALLOCATORS=0
  WIN32_LEAN_AND_MEAN=1
)

if(MSVC)
  set(CMAKE_PDB_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
  set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

  target_compile_definitions(AyaCompilerOptions INTERFACE
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
    _WINSOCKAPI_=1
  )

  # Control Flow Guard
  target_compile_options(AyaCompilerOptions INTERFACE /guard:cf)
  target_link_options(AyaCompilerOptions INTERFACE /guard:cf)
endif()

# --- MacOS Configuration ---

if(AYA_OS_MACOS)
  target_compile_options(AyaCompilerOptions INTERFACE -D_GLIBCXX_USE_DEPRECATED)
  target_compile_definitions(AyaCompilerOptions INTERFACE
    __APPLE__
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR
  )
endif()


# --- Dependencies (Using modern config lookups where vcpkg manages it) ---

# find_package(assimp CONFIG REQUIRED)
# find_package(bgfx CONFIG REQUIRED)
# find_package(Boost COMPONENTS filesystem system iostreams thread exception program_options url chrono container math CONFIG REQUIRED)
set(CGAL_DO_NOT_WARN_ABOUT_CMAKE_BUILD_TYPE 1)
# find_package(CGAL CONFIG REQUIRED)
# find_package(CURL REQUIRED)
# find_package(Freetype REQUIRED)
# find_package(imgui CONFIG REQUIRED)
# find_package(implot CONFIG REQUIRED)
# find_package(JPEG REQUIRED)
# find_package(LibArchive REQUIRED)
# find_package(lz4 CONFIG REQUIRED)
# find_package(OpenSSL REQUIRED)
# find_package(pugixml CONFIG REQUIRED)
# find_package(PNG REQUIRED)
# find_package(SDL3 CONFIG REQUIRED)
# find_package(ZLIB REQUIRED)
# find_package(zstd REQUIRED)

# NOTE: Raw finder hacks for libjxl, portaudio, opus, and discord have been fully 
# removed! They are now natively requested by vcpkg.json features.
# find_package(libjxl CONFIG REQUIRED) 

# Discord Rich Presence
if(ENABLE_DISCORD_INTEGRATION)
  find_package(discord-rpc CONFIG REQUIRED)
endif()

# Voice Chat
if(ENABLE_VOICE_CHAT)
  find_package(portaudio CONFIG REQUIRED)
  find_package(Opus CONFIG REQUIRED)
endif()

# Chromium Embedded Framework (CEF)
if(ENABLE_CHROMIUM_FRAMES)
  if(USE_CEF_FIND_PACKAGE)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} $ENV{CEF_ROOT}/cmake)
    set(CEF_ROOT $ENV{CEF_ROOT})
    find_package(CEF REQUIRED)
    include_directories(${CEF_INCLUDE_PATH})
  else()
    include(${THIRD_PARTY_DIR}/cef/cmake/cef_cmake.cmake)
    add_subdirectory(${THIRD_PARTY_DIR}/cef)
  endif()

  if(NOT USE_CEF_FIND_PACKAGE)
    set(CEF_WRAPPER cefdll_wrapper)
  else()
    set(CEF_WRAPPER $ENV{CEF_ROOT}/build/libcef_dll_wrapper/libcef_dll_wrapper.a)
    if(CMAKE_BUILD_TYPE EQUAL "Debug")
    	set(CEF_LIB ${CEF_LIB_DEBUG})
    else()
      set(CEF_LIB ${CEF_LIB_RELEASE})
    endif()
  endif()
  set(CEF_LIBRARIES ${CEF_WRAPPER} ${CEF_LIB})
endif()

# Define clean FMOD targets rather than directory hacking
add_library(fmod SHARED IMPORTED)

if(AYA_ARCH_X64)
  set(FMOD_ARCHITECTURE x64)
elseif(AYA_ARCH_ARM64)
  set(FMOD_ARCHITECTURE arm64)
endif()

if(AYA_OS_WINDOWS)
  set_property(TARGET fmod PROPERTY IMPORTED_LOCATION "${THIRD_PARTY_DIR}/fmod/lib/windows/${FMOD_ARCHITECTURE}/fmod.dll")
  set_property(TARGET fmod PROPERTY IMPORTED_IMPLIB "${THIRD_PARTY_DIR}/fmod/lib/windows/${FMOD_ARCHITECTURE}/fmod_vc.lib")
elseif(AYA_OS_MACOS)
  set_property(TARGET fmod PROPERTY IMPORTED_LOCATION "${THIRD_PARTY_DIR}/fmod/lib/mac/libfmod.dylib")
elseif(AYA_OS_LINUX)
  set_property(TARGET fmod PROPERTY IMPORTED_LOCATION "${THIRD_PARTY_DIR}/fmod/lib/linux/${FMOD_ARCHITECTURE}/libfmod.so")
endif()

# Qt Setup
if(BUILD_STUDIO)
  find_package(Qt6 COMPONENTS Core Widgets Concurrent WebEngineWidgets Gui Xml REQUIRED)
endif()

# --- Compile Subdirectories ---

if(NOT AYA_OS_ANDROID)
  if(BUILD_STUDIO)
    add_subdirectory(${THIRD_PARTY_DIR}/QtitanRibbon)
  endif() 
endif()

add_subdirectory(${THIRD_PARTY_DIR}/BulletPhysics)
add_subdirectory(${THIRD_PARTY_DIR}/RakNet)

add_subdirectory(${ENGINE_DIR}/3d)
add_subdirectory(${ENGINE_DIR}/app)
add_subdirectory(${ENGINE_DIR}/core)
add_subdirectory(${ENGINE_DIR}/network)
add_subdirectory(${ENGINE_DIR}/gfx)

if(NOT AYA_OS_ANDROID)
  if(BUILD_STUDIO AND AYA_OS_WINDOWS)
    include(${CLIENT_DIR}/windeployqt.cmake)
  endif()

  if(BUILD_PLAYER)
    add_subdirectory(${CLIENT_DIR}/player)
  endif()

  if(BUILD_SERVER)
    add_subdirectory(${CLIENT_DIR}/server)
  endif()

  if(BUILD_STUDIO)
    add_subdirectory(${CLIENT_DIR}/studio)
  endif()
else()
  add_subdirectory(${CLIENT_DIR}/mobile)
endif()

if(ENABLE_CHROMIUM_FRAMES)
  add_subdirectory(${TOOLS_DIR}/cef-subprocess)
endif()

if(AYA_OS_WINDOWS)
  add_subdirectory(${TOOLS_DIR}/thumbnail-helper)
endif()

# --- Modern Interface Libraries for Linking ---

add_library(AyaDependencies INTERFACE)
target_link_libraries(AyaDependencies INTERFACE
  AyaCompilerOptions
  assimp::assimp
  bgfx::bx bgfx::bgfx bgfx::bimg bgfx::bimg_decode
  Boost::chrono
  Boost::container
  Boost::exception
  Boost::filesystem
  Boost::iostreams
  Boost::math
  Boost::program_options
  Boost::system
  Boost::thread
  Boost::url
  CGAL::CGAL
  CURL::libcurl
  fmod
  Freetype::Freetype
  imgui::imgui
  implot::implot
  JPEG::JPEG
  lz4::lz4
  OpenSSL::SSL OpenSSL::Crypto
  PNG::PNG
  libjxl::libjxl
  SDL3::SDL3
  ZLIB::ZLIB
  $<IF:$<TARGET_EXISTS:zstd::libzstd_shared>,zstd::libzstd_shared,zstd::libzstd_static>
)

# Manage static/shared pugixml link styles safely
if(VCPKG_LIBRARY_LINKAGE STREQUAL "static" OR AYA_OS_LINUX OR AYA_OS_MACOS)
  target_link_libraries(AyaDependencies INTERFACE pugixml::pugixml)
else()
  target_link_libraries(AyaDependencies INTERFACE pugixml::shared)
endif()

add_library(AyaBootstrapperDependencies INTERFACE)
target_link_libraries(AyaBootstrapperDependencies INTERFACE
  AyaCompilerOptions
  Boost::program_options
  CURL::libcurl
  LibArchive::LibArchive
  ZLIB::ZLIB
  $<IF:$<TARGET_EXISTS:zstd::libzstd_shared>,zstd::libzstd_shared,zstd::libzstd_static>
)

if(ENABLE_VOICE_CHAT)
  target_link_libraries(AyaDependencies INTERFACE 
    Opus::opus
    portaudio
  )
  if(MSVC)
    target_link_options(AyaCompilerOptions INTERFACE /FORCE:MULTIPLE)
  endif()
endif()

if(ENABLE_DISCORD_INTEGRATION)
  target_link_libraries(AyaDependencies INTERFACE discord-rpc)
endif()

# Platform libraries
if(MSVC OR AYA_OS_WINDOWS)
  target_link_libraries(AyaDependencies INTERFACE
    pdh
    Ws2_32
    wininet
    opengl32
    Version
    bcrypt
    Iphlpapi
    d3d11
    dxgi
    Winmm
  )
endif()

# --- Target dependency wiring ---

if(ENABLE_CHROMIUM_FRAMES AND NOT AYA_OS_MACOS)
  if(TARGET Player)
    add_dependencies(Player CefSubprocess)
  endif()
  if(TARGET Studio)
    add_dependencies(Studio CefSubprocess)
  endif()
endif()

if(NOT AYA_TEST_BUILD AND TARGET Player)
  add_dependencies(Player CompileShaders CompileCoreScripts)
endif()

# --- Output user notification ---
message(NOTICE "")
message(NOTICE "-- To compile CoreScripts for Player, run target 'CompileCoreScripts'")
message(NOTICE "-- To compile shaders, run target 'CompileShaders'")
message(NOTICE "-- All build artifacts are available in '${CMAKE_RUNTIME_OUTPUT_DIRECTORY}'")
message(NOTICE "")