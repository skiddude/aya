find_package(Qt6Core REQUIRED)

# get absolute path to qmake, then use it to find windeployqt executable
get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)

function(windeployqt target)
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(_config "--debug")
  else()
    set(_config "--release")
  endif()

  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND "${_qt_bin_dir}/windeployqt.exe"
        ${_config}
        --pdb
        --verbose 0
        "$<TARGET_FILE:${target}>"
    COMMENT "Deploying Qt libraries using windeployqt for compilation target '${target}'..."
  )
endfunction()

# don't delete me ;-(