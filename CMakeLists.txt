cmake_minimum_required(VERSION 3.16)
project(Mp3Player VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Threads REQUIRED)

# Find the PkgConfig tool
find_package(PkgConfig REQUIRED)
# Use PkgConfig to find the taglib library
pkg_check_modules(TAGLIB REQUIRED taglib)



set(CMAKE_BUILD_TYPE Debug)
# Enable automatic compilation of Qt resource files
set(CMAKE_AUTORCC ON)

# Define source files common to all platforms
set(PROJECT_SOURCES
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/core/PlaylistManager.cpp
    src/core/PlaylistManager.h
    src/core/AudioPlayer.cpp
    src/core/AudioPlayer.h
    src/core/TrackMetadata.cpp
    src/core/TrackMetadata.h
    src/ui/models/FileBrowserModelFactory.cpp
    src/ui/models/CustomSortProxyModel.cpp
    src/ui/ClickableSlider.h
    src/ui/InteractiveBackground.cpp
    src/ui/InteractiveBackground.h
    resources.qrc
    src/ui/HighlightDelegate.h
    src/ui/LeftWidget.cpp
    src/ui/LeftWidget.h
    src/ui/RightWidget.cpp
    src/ui/RightWidget.h
    src/ui/BottomWidget.cpp
    src/ui/BottomWidget.h
)

# Append the resource file only when compiling on a Windows system
if(WIN32)
    set(PROJECT_SOURCES ${PROJECT_SOURCES} resources.rc)
endif()

# Create the executable file
add_executable(Mp3Player WIN32 ${PROJECT_SOURCES})

# Include project and TagLib headers
target_include_directories(Mp3Player PRIVATE ${CMAKE_SOURCE_DIR}/src ${TAGLIB_INCLUDE_DIRS})

# Link all libraries including TagLib
target_link_libraries(Mp3Player PRIVATE
    Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets 
    Threads::Threads 
    m 
    ${CMAKE_DL_LIBS}
    ${TAGLIB_LIBRARIES}
)
