project(woincqt VERSION ${WOINC_VERSION} LANGUAGES CXX)

include(woincSetupCompilerOptions)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Charts REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Test)

set(CMAKE_AUTOMOC ON)

set(WOINCQT_SOURCES
    adapter.cc
    controller.cc
    gui.cc
    main.cc
    menu.cc
    model.cc
    model_handler.cc
    tabs_widget.cc
    types.cc
    utils.cc

    dialogs/preferences_dialog.cc
    dialogs/project_properties_dialog.cc
    dialogs/select_computer_dialog.cc
    dialogs/task_properties_dialog.cc

    tabs/delegates.cc
    tabs/disk_tab.cc
    tabs/events_tab.cc
    tabs/notices_tab.cc
    tabs/projects_tab.cc
    tabs/proxy_models.cc
    tabs/statistics_tab.cc
    tabs/tasks_tab.cc
    tabs/transfers_tab.cc
)

add_executable(woincqt ${WOINCQT_SOURCES})

woincSetupCompilerOptions(woincqt)
#target_compile_definitions(woincqt PRIVATE QT_NO_CAST_FROM_ASCII)

target_link_libraries(woincqt PRIVATE $<TARGET_OBJECTS:woinc_ui_common>
    woinc::ui Qt5::Widgets Qt5::Charts Qt5::Network)

install(TARGETS woincqt DESTINATION ${INSTALL_BIN_DIR} PERMISSIONS OWNER_WRITE OWNER_READ
    OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

if(Qt5Test_FOUND)
    add_subdirectory(tests EXCLUDE_FROM_ALL)
endif()
