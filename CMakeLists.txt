cmake_minimum_required(VERSION 3.16)
project(Cairn VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Svg Gui)

# Everything except main() lives here, so the offscreen preview tool can render
# the real windows instead of a reimplementation of them.
set(CAIRN_SOURCES
    src/theme.h
    src/theme.cpp
    src/systeminfo.h
    src/systeminfo.cpp
    src/icons.h
    src/icons.cpp
    src/chrome.h
    src/chrome.cpp
    src/sessionmodel.h
    src/sessionmodel.cpp
    src/desktoppositions.h
    src/desktoppositions.cpp
    src/sessionstore.h
    src/sessionstore.cpp
    src/entryicon.h
    src/filterchip.h
    src/filterchip.cpp
    src/folderwatch.h
    src/folderwatch.cpp
    src/layoutview.h
    src/layoutview.cpp
    src/inspector.h
    src/inspector.cpp
    src/onboarding.h
    src/onboarding.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/sessionwindow.h
    src/sessionwindow.cpp
)

add_library(cairn_core STATIC ${CAIRN_SOURCES})
target_include_directories(cairn_core PUBLIC src)

# COM (real desktop icon positions), DWM (dark caption bars) and the shell APIs
# for resolving shortcuts. Public, so every tool that links the core inherits it.
if(WIN32)
    target_link_libraries(cairn_core PUBLIC
        Qt6::Widgets Qt6::Svg Qt6::Gui dwmapi ole32 oleaut32 uuid shell32 shlwapi)
else()
    target_link_libraries(cairn_core PUBLIC Qt6::Widgets Qt6::Svg Qt6::Gui)
endif()

add_executable(cairn src/main.cpp)
target_link_libraries(cairn PRIVATE cairn_core)
if(WIN32)
    set_target_properties(cairn PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# Development tools. None of them ship, and none of them belong beside the app:
# their binaries and autogen folders go into build\tools, so build\ stays the
# one-folder picture of what the app actually is. Use check.bat to run them.
#
# Optional on purpose: a published copy of the source may contain only the app,
# and it still has to configure and build on its own.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools/e2e.cpp")
    set(CAIRN_TOOLS
        preview:tools/preview.cpp            # renders the screens to PNGs
        capturecheck:tools/capturecheck.cpp  # prints a real capture of this machine
        e2e:tools/e2e.cpp                    # capture -> save -> reload -> lay out
        windowprobe:tools/windowprobe.cpp    # clicks every title-bar control
        regress:tools/regress.cpp            # regression checks for fixed flaws
        filtercheck:tools/filtercheck.cpp    # kind-filter chips
        keycheck:tools/keycheck.cpp          # keyboard navigation
        sortcheck:tools/sortcheck.cpp        # list sorting
        geomcheck:tools/geomcheck.cpp        # window geometry persistence
        auditcheck:tools/auditcheck.cpp      # bug-fix regressions
        livecheck:tools/livecheck.cpp        # live folder updates
        liveshot:tools/liveshot.cpp          # before/after live screenshots
        movecheck:tools/movecheck.cpp        # moving/renaming while open
        deskcheck:tools/deskcheck.cpp        # desktop icon-position polling
        debugcheck:tools/debugcheck.cpp      # bugs found in the debug pass
        headershot:tools/headershot.cpp      # renders the header for inspection
        popupshot:tools/popupshot.cpp        # reproduces the sort popup
        scrollcheck:tools/scrollcheck.cpp    # scrolling, list hit-testing, hover
    )
    foreach(entry IN LISTS CAIRN_TOOLS)
        string(REPLACE ":" ";" parts "${entry}")
        list(GET parts 0 name)
        list(GET parts 1 source)
        add_executable(cairn_${name} EXCLUDE_FROM_ALL ${source})
        target_link_libraries(cairn_${name} PRIVATE cairn_core)
        # Keep the binary and the MOC output out of the build root.
        set_target_properties(cairn_${name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools"
            AUTOGEN_BUILD_DIR "${CMAKE_BINARY_DIR}/tools/${name}_autogen")
    endforeach()

    # capturecheck samples a real folder to exercise the computed-grid path.
    # Point it at src\ explicitly: guessing a relative path from the binary's
    # location silently captured an empty folder once the tools moved.
    target_compile_definitions(cairn_capturecheck PRIVATE
        CAIRN_SAMPLE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/src")
endif()
