add_library(ThumbnailHelper SHARED
  src/thumb_win32.cc
  src/thumb_win32_dll.cc
  src/thumb_win32.def
  resources/winrc.h
  resources/script.rc
)

target_include_directories(ThumbnailHelper PRIVATE src resources)
target_link_libraries(ThumbnailHelper dbghelp.lib Version.lib)
set_target_properties(ThumbnailHelper PROPERTIES OUTPUT_NAME "Aya.ThumbnailHelper")