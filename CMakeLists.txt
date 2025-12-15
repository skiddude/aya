cmake_minimum_required(VERSION 3.25)

project(Aya)

########## Project Options ##########

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Create many helpful paths to have CMake consume it
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/third-party" THIRD_PARTY_DIR)
list(APPEND CMAKE_PREFIX_PATH "${THIRD_PARTY_DIR}")

file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/engine" ENGINE_DIR)
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/client" CLIENT_DIR)
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/tools" TOOLS_DIR)

set(AYA_VERSION_MAJOR 1)
set(AYA_VERSION_MINOR 0)
set(AYA_VERSION_PATCH 0)
set(AYA_VERSION_TYPE beta)

set(AYA_PROJECT_NAME "Aya" CACHE STRING "Public-facing project name")

option(AYA_TEST_BUILD "Enable testing utilities" ON)
option(ADDRESS_SANITIZER "Enable address sanitization" OFF)
option(ENABLE_CHROMIUM_FRAMES "Enable ChromiumFrames with CEF integration" ON)
set(CEF_VERSION "140.1.14+geb1c06e+chromium-140.0.7339.185" CACHE STRING "CEF version to use (if ChromiumFrames are enabled)")
option(ENABLE_VOICE_CHAT "Enable voice chat" ON)
option(ENABLE_DISCORD_INTEGRATION "Enable Discord Rich Presence integration" ON)

if(AYA_TEST_BUILD AND NOT CI)
  add_compile_definitions(AYA_TEST_BUILD=1)
endif()

if(NOT DEFINED AYA_BUILD_TIMESTAMP)
  if(AYA_TEST_BUILD)
    set(_AYA_BUILD_TIMESTAMP "0")
  else()
    string(TIMESTAMP _AYA_BUILD_TIMESTAMP "%s" UTC)
  endif()
  set(AYA_BUILD_TIMESTAMP "${_AYA_BUILD_TIMESTAMP}" CACHE STRING "Aya build timestamp")
endif()

if("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^(arm64|ARM64|aarch64|AARCH64)$")
  set(AYA_ARCH_ARM64 1)
  add_compile_definitions(AYA_ARCH_ARM64=1 _ARM64_=1)
  set(AYA_PLATFORM_NAME "(arm64)")
elseif("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^(x86_64|X86_64|amd64|AMD64)$")
  set(AYA_ARCH_X64 1)
  add_compile_definitions(AYA_ARCH_X64=1 _AMD64_=1)
  set(AYA_PLATFORM_NAME "(x64)")
else()
  message(FATAL_ERROR "Aya: Unsupported architecture ${CMAKE_SYSTEM_PROCESSOR}")
endif()

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  set(AYA_OS_WINDOWS 1)
  add_compile_definitions(AYA_OS_WINDOWS)
  set(AYA_PLATFORM_NAME "Windows ${AYA_PLATFORM_NAME}")
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
  set(AYA_OS_LINUX 1)
  add_compile_definitions(AYA_OS_LINUX)
  set(AYA_PLATFORM_NAME "Linux ${AYA_PLATFORM_NAME}")
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
  set(AYA_OS_MACOS 1)
  add_compile_definitions(AYA_OS_MACOS)
  set(AYA_PLATFORM_NAME "MacOS ${AYA_PLATFORM_NAME}")
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Android")
  set(AYA_OS_ANDROID 1)
  add_compile_definitions(AYA_OS_ANDROID)
  set(AYA_PLATFORM_NAME "Android ${AYA_PLATFORM_NAME}")
else()
  message(FATAL_ERROR "Aya: Unsupported system ${CMAKE_SYSTEM_NAME}")
endif()

add_compile_definitions(
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

########## Compile Options ##########

# ChromiumFrames and related SSE3 options enabled required for rendering optimizations
# Building CEF on nixpkgs for MacOS is non-existent
if(AYA_OS_ANDROID OR (AYA_OS_MACOS AND CI))
  set(ENABLE_CHROMIUM_FRAMES OFF)
endif()

if(ENABLE_CHROMIUM_FRAMES)
  add_compile_definitions(ENABLE_CHROMIUM_FRAMES=1)
  add_compile_definitions(CEF_VERSION="${CEF_VERSION}")
  set(CEF_CMAKE_LOCALES_OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/cef/locales")
  set(CEF_CMAKE_RESOURCES_OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/cef/resources")
endif()

if(NOT AYA_ARCH_ARM64)
  add_compile_options(-mssse3)
endif()

if(ADDRESS_SANITIZER AND NOT AYA_ARCH_ARM64)
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
endif()

if(ENABLE_VOICE_CHAT)
  add_compile_definitions(ENABLE_VOICE_CHAT=1)
endif()

# Disable some annoying warnings
add_compile_options(-w)
add_compile_options(-Wnonportable-include-path)
add_compile_options(-Wundefined-var-template)
add_compile_options(-Wdelete-abstract-non-virtual-dtor)

########## Windows/MSVC Configuration ##########

add_compile_definitions(
  _HAS_AUTO_PTR_ETC=1
  _ENFORCE_MATCHING_ALLOCATORS=0
  WIN32_LEAN_AND_MEAN=1
)

if(MSVC)
  set(CMAKE_PDB_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
  set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

  include_directories($ENV{INCLUDE})

  if(ATL_PATH)
    include_directories("${ATL_PATH}/include")
    link_directories("${ATL_PATH}/lib/x64")
  endif()

  # Windows 10
  # https://learn.microsoft.com/en-us/cpp/porting/modifying-winver-and-win32-winnt?view=msvc-170
  add_compile_definitions(
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
    _WINSOCKAPI_=1 # Prevents winsock.h from defining macros that conflict with winsock2.h
  )

  # Control Flow Guard
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /guard:cf")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /guard:cf")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /guard:cf")
endif()

########## MacOS Configuration ##########

if(AYA_OS_MACOS)
  add_compile_options(-D_GLIBCXX_USE_DEPRECATED)
  add_compile_definitions(
    __APPLE__
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR
  )
endif()

########## Dependencies ##########

# vcpkg dependencies
find_package(assimp CONFIG REQUIRED)
find_package(bgfx CONFIG REQUIRED)
find_package(Boost COMPONENTS filesystem system iostreams thread exception program_options url chrono CONFIG REQUIRED)
set(CGAL_DO_NOT_WARN_ABOUT_CMAKE_BUILD_TYPE 1)
find_package(CGAL CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(Freetype REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(JPEG REQUIRED)
find_package(LibArchive REQUIRED)
find_package(lz4 CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(pugixml CONFIG REQUIRED)
find_package(PNG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(ZLIB REQUIRED)
find_package(zstd REQUIRED)

# libjxl hack
find_path(JXL_INCLUDE_DIR jxl/decode.h)
find_library(JXL_LIBRARY NAMES jxl)
find_library(JXL_CMS_LIBRARY NAMES jxl_cms)
find_library(JXL_THREADS_LIBRARY NAMES jxl_threads)
find_library(BROTLIDEC_LIBRARY NAMES brotlidec)
find_library(BROTLIENC_LIBRARY NAMES brotlienc)
find_library(BROTLICOMMON_LIBRARY NAMES brotlicommon)
find_library(LCMS2_LIBRARY NAMES lcms2)
find_library(HWY_LIBRARY NAMES hwy)

if(NOT JXL_LIBRARY OR NOT JXL_CMS_LIBRARY OR NOT JXL_THREADS_LIBRARY)
    message(FATAL_ERROR "JPEG XL libraries not found")
endif()

include_directories(${JXL_INCLUDE_DIR})

# Boost
add_compile_definitions(BOOST_THREAD_PROVIDES_FUTURE)
add_compile_definitions(BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION)

find_package(Boost CONFIG REQUIRED COMPONENTS
  system
  exception
  container
  thread
  filesystem
  iostreams
  math
  program_options
  url
  chrono
)

# Discord Rich Presence
if(ENABLE_DISCORD_INTEGRATION)
  find_path(DISCORD_RPC_INCLUDE_DIRS "discord_rpc.h")
  find_library(DISCORD_RPC_LIBRARY discord-rpc REQUIRED)
  add_compile_definitions(ENABLE_DISCORD_INTEGRATION)
  include_directories(${DISCORD_RPC_INCLUDE_DIRS})
endif()

# Voice Chat
if(ENABLE_VOICE_CHAT)
  find_package(portaudio CONFIG REQUIRED)
  find_path(OPUS_INCLUDE_DIR opus.h PATH_SUFFIXES opus)
  find_library(OPUS_LIBRARY opus)
  include_directories(${OPUS_INCLUDE_DIR})
endif()

# Chromium Embedded Framework (CEF) integration
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

# FMOD
include_directories(${THIRD_PARTY_DIR}/fmod/include)

option(USE_CEF_FIND_PACKAGE OFF)

if(AYA_ARCH_X64)
  set(FMOD_ARCHITECTURE x64)
elseif(AYA_ARCH_ARM64)
  set(FMOD_ARCHITECTURE arm64)
endif()

if(AYA_OS_WINDOWS)
  link_directories(${THIRD_PARTY_DIR}/fmod/lib/windows/${FMOD_ARCHITECTURE})
  set(RUNTIME_FILES ${THIRD_PARTY_DIR}/fmod/lib/windows/${FMOD_ARCHITECTURE}/fmod.dll)
elseif(AYA_OS_MACOS)
  link_directories(${THIRD_PARTY_DIR}/fmod/lib/mac)
  set(RUNTIME_FILES ${THIRD_PARTY_DIR}/fmod/lib/mac/libfmod.dylib)

  add_library(fmod SHARED IMPORTED)
  set_property(TARGET fmod PROPERTY IMPORTED_LOCATION ${THIRD_PARTY_DIR}/fmod/lib/mac/libfmod.dylib)
elseif(AYA_OS_LINUX)
  link_directories(${THIRD_PARTY_DIR}/fmod/lib/linux/${FMOD_ARCHITECTURE})
  set(RUNTIME_FILES ${THIRD_PARTY_DIR}/fmod/lib/linux/${FMOD_ARCHITECTURE}/libfmod.so.13)

  add_library(fmod SHARED IMPORTED)
  set_property(TARGET fmod PROPERTY IMPORTED_LOCATION ${THIRD_PARTY_DIR}/fmod/lib/linux/${FMOD_ARCHITECTURE}/libfmod.so)
endif()

# Qt
# Arch: pacman -S qt6-base qt6-webengine
# Ubuntu: apt install qt6-base-dev qt6-webengine-dev qt6-webengine-dev-tools libqt6webenginecore6-bin
find_package(Qt6 COMPONENTS Core Widgets Concurrent WebEngineWidgets Gui GuiPrivate Designer Xml REQUIRED)

# QtitanRibbon
if(NOT AYA_OS_ANDROID)
  add_subdirectory(${THIRD_PARTY_DIR}/QtitanRibbon) 
endif()

# Other dependencies
add_subdirectory(${THIRD_PARTY_DIR}/BulletPhysics)
add_subdirectory(${THIRD_PARTY_DIR}/RakNet)

########## Aya Project ##########

include_directories(${CLIENT_DIR}/common)

add_subdirectory(${ENGINE_DIR}/3d)
add_subdirectory(${ENGINE_DIR}/app)
add_subdirectory(${ENGINE_DIR}/core)
add_subdirectory(${ENGINE_DIR}/network)
add_subdirectory(${ENGINE_DIR}/gfx)

if(NOT AYA_OS_ANDROID)
  if(AYA_OS_WINDOWS)
    include(${CLIENT_DIR}/windeployqt.cmake)
  endif()

  add_subdirectory(${TOOLS_DIR}/core-script-compiler)
  add_subdirectory(${CLIENT_DIR}/bootstrapper)

  add_subdirectory(${CLIENT_DIR}/player)
  add_subdirectory(${CLIENT_DIR}/server)
  add_subdirectory(${CLIENT_DIR}/studio)
else()
  add_subdirectory(${CLIENT_DIR}/mobile)
endif()

if(ENABLE_CHROMIUM_FRAMES)
  add_subdirectory(${TOOLS_DIR}/cef-subprocess)
endif()

if(AYA_OS_WINDOWS)
  add_subdirectory(${TOOLS_DIR}/thumbnail-helper)
endif()

set(CLIENT_THIRD_PARTY_LIBRARIES
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
  pugixml::pugixml
  SDL3::SDL3
  Vulkan::Vulkan
  ZLIB::ZLIB
  $<IF:$<TARGET_EXISTS:zstd::libzstd_shared>,zstd::libzstd_shared,zstd::libzstd_static>
  ${JXL_LIBRARY}
  ${JXL_CMS_LIBRARY}
  ${JXL_THREADS_LIBRARY}
  ${BROTLIDEC_LIBRARY}
  ${BROTLIENC_LIBRARY}
  ${BROTLICOMMON_LIBRARY}
  ${LCMS2_LIBRARY}
  ${HWY_LIBRARY}
)

set(BOOTSTRAPPER_THIRD_PARTY_LIBRARIES
  Boost::program_options
  CURL::libcurl
  LibArchive::LibArchive
  ZLIB::ZLIB
  $<IF:$<TARGET_EXISTS:zstd::libzstd_shared>,zstd::libzstd_shared,zstd::libzstd_static>
)

if(ENABLE_VOICE_CHAT)
  set(CLIENT_THIRD_PARTY_LIBRARIES ${CLIENT_THIRD_PARTY_LIBRARIES} ${OPUS_LIBRARY})
  set(CLIENT_THIRD_PARTY_LIBRARIES ${CLIENT_THIRD_PARTY_LIBRARIES} portaudio_static)
  set(CLIENT_THIRD_PARTY_LIBRARIES ${CLIENT_THIRD_PARTY_LIBRARIES} portaudio)

  if(MSVC)
    # annoying hack for PortAudio shittiness
    add_link_options(/FORCE:MULTIPLE)
  endif()
endif()

if(VCPKG_LIBRARY_LINKAGE STREQUAL "static" OR AYA_OS_LINUX OR AYA_OS_MACOS)
  set(CLIENT_THIRD_PARTY_LIBRARIES ${CLIENT_THIRD_PARTY_LIBRARIES} pugixml::pugixml)
else()
  set(CLIENT_THIRD_PARTY_LIBRARIES ${CLIENT_THIRD_PARTY_LIBRARIES} pugixml::shared)
endif()

if(MSVC OR AYA_OS_WINDOWS)
  set(MSVC_LIBRARIES
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

if(ENABLE_CHROMIUM_FRAMES AND NOT AYA_OS_MACOS)
  add_dependencies(Player CefSubprocess)
  add_dependencies(Studio CefSubprocess)
endif()

if(NOT AYA_TEST_BUILD)
  add_dependencies(Player CompileShaders CompileCoreScripts)
endif()

set(QT_LIBRARIES
  Qt6::Core 
  Qt6::Widgets
  Qt6::Concurrent
  Qt6::Gui
  Qt6::GuiPrivate
  Qt6::WebEngineWidgets
  Qt6::Xml
)

target_link_libraries(Player ${CLIENT_THIRD_PARTY_LIBRARIES} ${MSVC_LIBRARIES} ${CEF_LIBRARIES} ${QT_LIBRARIES} ${DISCORD_RPC_LIBRARY})
target_link_libraries(Studio ${CLIENT_THIRD_PARTY_LIBRARIES} ${MSVC_LIBRARIES} ${CEF_LIBRARIES} ${QT_LIBRARIES})
target_link_libraries(Server ${CLIENT_THIRD_PARTY_LIBRARIES} ${MSVC_LIBRARIES} ${QT_LIBRARIES})
target_link_libraries(CoreScriptCompiler ${CLIENT_THIRD_PARTY_LIBRARIES} ${MSVC_LIBRARIES})
target_link_libraries(Bootstrapper ${BOOTSTRAPPER_THIRD_PARTY_LIBRARIES} ${MSVC_LIBRARIES} ${QT_LIBRARIES})

message(NOTICE "")
message(NOTICE "-- To compile CoreScripts for Player, run target 'CompileCoreScripts'")
message(NOTICE "-- To compile shaders, run target 'CompileShaders'")
message(NOTICE "-- All build artifacts are available in '${CMAKE_RUNTIME_OUTPUT_DIRECTORY}'")
message(NOTICE "")
