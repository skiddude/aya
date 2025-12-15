set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(Server
  resources/qt.qrc
  src/main.cpp
  src/server.cpp
  src/server.hpp
  src/grid.cpp
  src/grid.hpp
  src/AppView.hpp
  src/AppView.cpp
  src/MainWindow.cpp
  src/MainWindow.hpp

  src/linenoise.hpp
  ${CLIENT_DIR}/common/AppSettings.cpp
  ${CLIENT_DIR}/common/AppSettings.hpp
  ${CLIENT_DIR}/common/SDLGameController.cpp
  ${CLIENT_DIR}/common/SDLGameController.hpp
)

target_link_libraries(Server
  3D
  AppServer
  Core
  RakNet
  BulletPhysics
  NetworkServer
  Graphics
)

if(AYA_OS_WINDOWS)
  target_sources(Server PRIVATE
    resources/winrc.h
    resources/script.rc
  )

  set_target_properties(Server PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

target_compile_definitions(Server PRIVATE QT_NO_KEYWORDS)

target_include_directories(Server PRIVATE src resources)
set_target_properties(Server PROPERTIES OUTPUT_NAME "Aya.Server")

add_custom_command(TARGET Server POST_BUILD
  COMMENT "Copying runtime files to build directory"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${RUNTIME_FILES}"
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)
