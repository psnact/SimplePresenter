cmake_minimum_required(VERSION 3.20)
project(SimplePresenter VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt 6 core packages (required)
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Multimedia
    MultimediaWidgets
    Network
    WebSockets
    Xml
)

# Optional: WebEngine for YouTube browser
find_package(Qt6 COMPONENTS WebEngineWidgets QUIET)
if (Qt6WebEngineWidgets_FOUND)
    message(STATUS "SimplePresenter: Qt6 WebEngineWidgets FOUND (YouTube enabled)")
else()
    message(STATUS "SimplePresenter: Qt6 WebEngineWidgets NOT found (YouTube disabled)")
endif()

# ActiveQt (AxContainer) is optional – used for PowerPoint integration if present.
# It is a Windows-only module, so skip lookup on other platforms to avoid
# noisy warnings when the component does not exist.
if(WIN32)
    find_package(Qt6 COMPONENTS AxContainer QUIET)
endif()

find_package(Qt6 COMPONENTS Pdf QUIET)

find_package(Qt6 COMPONENTS Svg QUIET)


# Source files
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/BibleManager.cpp
    src/BibleManager.h
    src/SongManager.cpp
    src/SongManager.h
    src/PlaylistManager.cpp
    src/PlaylistManager.h
    src/CanvasWidget.cpp
    src/CanvasWidget.h
    src/ProjectionCanvas.cpp
    src/ProjectionCanvas.h
    src/BiblePanel.cpp
    src/BiblePanel.h
    src/SongPanel.cpp
    src/SongPanel.h
    src/PlaylistPanel.cpp
    src/PlaylistPanel.h
    src/SettingsDialog.cpp
    src/SettingsDialog.h
    src/SongEditorDialog.cpp
    src/SongEditorDialog.h
    src/MediaPanel.cpp
    src/MediaPanel.h
    src/PowerPointPanel.cpp
    src/PowerPointPanel.h
    src/OverlayServer.cpp
    src/OverlayServer.h
    src/UpdateChecker.cpp
    src/UpdateChecker.h
    src/AdblockManager.cpp
    src/AdblockManager.h
)

# Resources
set(RESOURCES
    resources/resources.qrc
)

if(APPLE)
    # Build a proper .app bundle on macOS. Use a custom .icns icon when the
    # file is present; otherwise, build without an application icon instead of
    # failing configuration.
    set(MACOSX_BUNDLE_ICON_FILE SimplePresenter.icns)
    set(APP_ICON_MACOS "${CMAKE_CURRENT_SOURCE_DIR}/resources/SimplePresenter.icns")
    if(EXISTS "${APP_ICON_MACOS}")
        set_source_files_properties(${APP_ICON_MACOS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCES} ${RESOURCES} ${APP_ICON_MACOS})
    else()
        message(WARNING "SimplePresenter macOS icon not found at ${APP_ICON_MACOS}; building without custom .icns icon.")
        add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCES} ${RESOURCES})
    endif()
elseif(WIN32)
    # Windows GUI executable with embedded icon
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${RESOURCES} resources/app_icon.rc)
else()
    # Other platforms: normal executable
    add_executable(${PROJECT_NAME} ${SOURCES} ${RESOURCES})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::Network
    Qt6::WebSockets
    Qt6::Xml
)

if (Qt6Svg_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Svg)
endif()

if (Qt6WebEngineWidgets_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::WebEngineWidgets)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SIMPLEPRESENTER_HAVE_WEBENGINE)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE SIMPLEPRESENTER_VERSION="${PROJECT_VERSION}")

if (TARGET Qt6::Pdf)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Pdf)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SIMPLEPRESENTER_HAVE_QTPDF)
endif()

if (TARGET Qt6::AxContainer)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::AxContainer)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SIMPLEPRESENTER_HAVE_ACTIVEQT)
endif()

# Windows-specific settings
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
endif()

# Install rules
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
