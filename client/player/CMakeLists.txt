set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(Player
  resources/qt.qrc

  src/main.cpp

  src/Application.cpp
  src/Application.hpp
  src/AvatarViewService.cpp
  src/AvatarViewService.hpp
  src/Document.cpp
  src/Document.hpp
  src/FunctionMarshaller.cpp
  src/FunctionMarshaller.hpp
  src/GameVerbs.cpp
  src/GameVerbs.hpp
  src/LauncherView.cpp
  src/LauncherView.hpp
  src/RenderJob.cpp
  src/RenderJob.hpp
  src/View.cpp
  src/View.hpp
  src/Window.cpp
  src/Window.hpp
  ${CLIENT_DIR}/common/AppSettings.cpp
  ${CLIENT_DIR}/common/AppSettings.hpp
  ${CLIENT_DIR}/common/GrayChatBar.hpp
  ${CLIENT_DIR}/common/GrayChatBar.cpp
  ${CLIENT_DIR}/common/SDLGameController.cpp
  ${CLIENT_DIR}/common/SDLGameController.hpp
)

target_link_libraries(Player
  3D
  AppPlayer
  Core
  RakNet
  BulletPhysics
  NetworkPlayer
  Graphics
)

if(AYA_OS_WINDOWS)
  target_sources(Player PRIVATE
    resources/winrc.h
    resources/script.rc
  )

  set_target_properties(Player PROPERTIES WIN32_EXECUTABLE TRUE)
  windeployqt(Player)
endif()

target_compile_definitions(Player PRIVATE QT_NO_KEYWORDS)

target_include_directories(Player PRIVATE src resources)
set_target_properties(Player PROPERTIES OUTPUT_NAME "Aya.Player")

add_custom_command(TARGET Player POST_BUILD
  COMMENT "Copying runtime files to build directory"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${RUNTIME_FILES}"
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)
