add_executable(CefSubprocess src/main.cpp)

if(AYA_OS_WINDOWS)
  target_sources(CefSubprocess PRIVATE
    resources/winrc.h
    resources/script.rc
  )

  set_target_properties(CefSubprocess PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

target_include_directories(CefSubprocess PRIVATE src resources)
set_target_properties(CefSubprocess PROPERTIES OUTPUT_NAME "${AYA_PROJECT_NAME}.CefSubprocess")
target_link_libraries(CefSubprocess PRIVATE ${CEF_LIBRARIES})
