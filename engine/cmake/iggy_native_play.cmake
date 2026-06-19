find_package(Vulkan QUIET)
find_package(PkgConfig QUIET)

if(PkgConfig_FOUND)
  pkg_check_modules(SDL2 QUIET IMPORTED_TARGET sdl2)
endif()

if(Vulkan_FOUND AND SDL2_FOUND)
  add_executable(iggy_native_play
    apps/native_play/IggyNativePlay.cpp
  )

  target_link_libraries(iggy_native_play PRIVATE
    iggy_engine
    Vulkan::Vulkan
    PkgConfig::SDL2
  )

  target_include_directories(iggy_native_play PRIVATE src)
else()
  if(NOT Vulkan_FOUND)
    message(STATUS "Vulkan not found; skipping iggy_native_play")
  endif()
  if(NOT SDL2_FOUND)
    message(STATUS "SDL2 not found; skipping iggy_native_play")
  endif()
endif()
