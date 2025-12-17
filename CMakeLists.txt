cmake_minimum_required(VERSION 3.20)
project(SimplePresenter VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

 if(APPLE)
     if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
         set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum macOS version to target")
     endif()
 endif()

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
    # Optional: Sparkle framework for macOS in-app updates
    set(SPARKLE_FRAMEWORK_SEARCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
    find_library(SPARKLE_FRAMEWORK Sparkle PATHS "${SPARKLE_FRAMEWORK_SEARCH_DIR}" NO_DEFAULT_PATH)
    if(NOT SPARKLE_FRAMEWORK)
        find_library(SPARKLE_FRAMEWORK Sparkle)
    endif()

    set(SIMPLEPRESENTER_BUNDLE_ID "com.psnact.SimplePresenter")
    set(SIMPLEPRESENTER_SPARKLE_FEED_URL "https://psnact.github.io/SimplePresenter/appcast.xml")
    set(SIMPLEPRESENTER_SPARKLE_PUBLIC_KEY "cXl5sb5n1HCFD9fhVV842OWe3s9P1kBANV5eRd9LE78=")
endif()

if(APPLE)
    # Build a proper .app bundle on macOS. Use a custom .icns icon when the
    # file is present; otherwise, build without an application icon instead of
    # failing configuration.
    set(MACOSX_BUNDLE_ICON_FILE SimplePresenter.icns)
    set(APP_ICON_MACOS "${CMAKE_CURRENT_SOURCE_DIR}/resources/SimplePresenter.icns")

    # Sparkle requires SUFeedURL and SUPublicEDKey in Info.plist
    set(INFO_PLIST_IN "${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist.in")
    set(INFO_PLIST_OUT "${CMAKE_CURRENT_BINARY_DIR}/Info.plist")
    if(EXISTS "${INFO_PLIST_IN}")
        configure_file("${INFO_PLIST_IN}" "${INFO_PLIST_OUT}" @ONLY)
    endif()

    if(SPARKLE_FRAMEWORK)
        list(APPEND SOURCES
            src/MacSparkleUpdater.h
            src/MacSparkleUpdater_sparkle.mm
        )
        set_source_files_properties(src/MacSparkleUpdater_sparkle.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
    else()
        list(APPEND SOURCES
            src/MacSparkleUpdater.h
            src/MacSparkleUpdater.cpp
        )
    endif()

    if(EXISTS "${APP_ICON_MACOS}")
        set_source_files_properties(${APP_ICON_MACOS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCES} ${RESOURCES} ${APP_ICON_MACOS})
    else()
        message(WARNING "SimplePresenter macOS icon not found at ${APP_ICON_MACOS}; building without custom .icns icon.")
        add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCES} ${RESOURCES})
    endif()

    if(EXISTS "${INFO_PLIST_OUT}")
        set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${INFO_PLIST_OUT}")
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

if(APPLE)
    if(SPARKLE_FRAMEWORK)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${SPARKLE_FRAMEWORK})
        target_compile_definitions(${PROJECT_NAME} PRIVATE SIMPLEPRESENTER_HAVE_SPARKLE)

        # find_library() may return either .../Sparkle.framework or .../Sparkle.framework/Sparkle
        get_filename_component(SPARKLE_FRAMEWORK_DIR "${SPARKLE_FRAMEWORK}" DIRECTORY)
        if("${SPARKLE_FRAMEWORK}" MATCHES "\\.framework$")
            set(SPARKLE_FRAMEWORK_DIR "${SPARKLE_FRAMEWORK}")
        endif()

        set_target_properties(${PROJECT_NAME} PROPERTIES
            BUILD_RPATH "@loader_path/../Frameworks"
            INSTALL_RPATH "@loader_path/../Frameworks"
        )

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Frameworks"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${SPARKLE_FRAMEWORK_DIR}" "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Frameworks/Sparkle.framework"
            VERBATIM
        )
    else()
        message(WARNING "Sparkle.framework not found. To enable macOS in-app updates, place Sparkle.framework in ${SPARKLE_FRAMEWORK_SEARCH_DIR}.")
    endif()
endif()

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
