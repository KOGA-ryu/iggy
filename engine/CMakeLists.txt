cmake_minimum_required(VERSION 3.20)

project(IggyEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(IGGY_BUILD_TESTS "Build iggy test executables" ON)

set(IGGY_ENGINE_SOURCES)

include(cmake/iggy_core_sources.cmake)
include(cmake/iggy_modules_sources.cmake)
include(cmake/iggy_runtime_sources.cmake)
include(cmake/iggy_scene_sources.cmake)
include(cmake/iggy_servers_sources.cmake)

add_library(iggy_engine ${IGGY_ENGINE_SOURCES})

target_include_directories(iggy_engine PUBLIC src)

if(IGGY_BUILD_TESTS)
  enable_testing()

  function(iggy_add_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE iggy_engine)
    add_test(NAME ${name} COMMAND ${name})

    set(labels ${IGGY_TEST_DEFAULT_LABELS})
    set(extra_labels ${ARGN})
    if(extra_labels)
      list(GET extra_labels 0 first_extra_label)
      if(first_extra_label STREQUAL "LABELS")
        list(REMOVE_AT extra_labels 0)
      endif()
      list(APPEND labels ${extra_labels})
    endif()

    if(name MATCHES "^ai_map_" OR name MATCHES "^npc_ai_" OR name MATCHES "^npc_map_" OR
      name MATCHES "^npc_(charisma|constitution|dexterity|fold|hand|intelligence|read|strength|tell|trait|wisdom|play)")
      list(APPEND labels scene-ai)
    endif()

    if(name MATCHES "^npc_actor_" OR name MATCHES "^npc_behavior_state" OR
      name MATCHES "^npc_move_mode" OR name MATCHES "^npc_objective")
      list(APPEND labels scene-npc)
    endif()

    if(name MATCHES "^(camera_|draft_|interaction_|inventory_|item_|level_|pickup_|player_|scene_ascii_|sprite_|ui_)")
      list(APPEND labels scene-level)
    endif()

    if(name MATCHES "(save|snapshot|slot|metadata|chunk_archive|file_envelope)")
      list(APPEND labels save-load)
    endif()

    set(has_kind_label FALSE)
    if(name MATCHES "acceptance")
      list(APPEND labels acceptance)
      set(has_kind_label TRUE)
    endif()
    if(name MATCHES "pipeline")
      list(APPEND labels pipeline)
      set(has_kind_label TRUE)
    endif()
    if(name MATCHES "(frame|runner|queue|scenario|orchestrated|save_load|snapshot|slot|codec)")
      list(APPEND labels integration)
      set(has_kind_label TRUE)
    endif()
    if(NOT has_kind_label)
      list(APPEND labels unit)
    endif()

    list(REMOVE_DUPLICATES labels)
    if(labels)
      set_tests_properties(${name} PROPERTIES LABELS "${labels}")
    endif()
  endfunction()

  set(IGGY_TEST_DEFAULT_LABELS core)
  include(cmake/iggy_core_tests.cmake)
  set(IGGY_TEST_DEFAULT_LABELS modules)
  include(cmake/iggy_modules_tests.cmake)
  set(IGGY_TEST_DEFAULT_LABELS runtime)
  include(cmake/iggy_runtime_tests.cmake)
  unset(IGGY_TEST_DEFAULT_LABELS)
  include(cmake/iggy_scene_tests.cmake)
  set(IGGY_TEST_DEFAULT_LABELS servers)
  include(cmake/iggy_servers_tests.cmake)
  unset(IGGY_TEST_DEFAULT_LABELS)
endif()

include(cmake/iggy_qt_shell.cmake)
