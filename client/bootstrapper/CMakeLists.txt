set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(Bootstrapper
  src/main.cpp
  resources/qt.qrc

  src/Bootstrapper.cpp
  src/Bootstrapper.hpp

  ${CLIENT_DIR}/common/AppSettings.cpp
  ${CLIENT_DIR}/common/AppSettings.hpp
)

if(AYA_OS_WINDOWS)
  target_sources(Bootstrapper PRIVATE
    resources/winrc.h
    resources/script.rc
  )

  set_target_properties(Bootstrapper PROPERTIES WIN32_EXECUTABLE TRUE)
  windeployqt(Bootstrapper)
endif()

target_compile_definitions(Bootstrapper PRIVATE SKIP_APP_SETTINGS_LOADING)
target_link_libraries(Bootstrapper ${OPENSSL_CRYPTO_LIBRARIES})
target_include_directories(Bootstrapper PRIVATE src resources)
set_target_properties(Bootstrapper PROPERTIES OUTPUT_NAME "Aya")