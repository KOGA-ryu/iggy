find_package(Qt6 QUIET COMPONENTS Widgets)

if(Qt6Widgets_FOUND)
  add_executable(iggy_qt_shell
    apps/qt_shell/IggyQtShell.cpp
    apps/qt_shell/IggyQtShellUi.cpp
    apps/qt_shell/IggyQtShellWindow.cpp
  )
  target_link_libraries(iggy_qt_shell PRIVATE
    iggy_engine
    Qt6::Widgets
  )
  target_include_directories(iggy_qt_shell PRIVATE src)
else()
  message(STATUS "Qt6 Widgets not found; skipping iggy_qt_shell")
endif()
