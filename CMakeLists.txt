cmake_minimum_required(VERSION 3.16)

# Проект использует только C++
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

# ---------------------------------------------------------
# ИСХОДНИКИ ПРОЕКТА
# ---------------------------------------------------------
set(PROJECT_SOURCES
    src/main.cpp
    src/miniaudio_impl.cpp
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/core/PlaylistManager.cpp
    src/core/PlaylistManager.h
    src/core/AudioPlayer.cpp
    src/core/AudioPlayer.h
    src/core/TrackMetadata.cpp
    src/core/TrackMetadata.h
    src/core/RingBuffer.cpp
    src/core/RingBuffer.h
    src/ui/models/FileBrowserModelFactory.cpp
    src/ui/models/CustomSortProxyModel.cpp
    src/ui/ClickableSlider.h
    src/ui/Visualizer.cpp
    src/ui/Visualizer.h
    src/ui/InteractiveBackground.cpp
    src/ui/InteractiveBackground.h
    src/ui/HighlightDelegate.h
    src/ui/LeftWidget.cpp
    src/ui/LeftWidget.h
    src/ui/RightWidget.cpp
    src/ui/RightWidget.h
    src/ui/BottomWidget.cpp
    src/ui/BottomWidget.h
    src/ui/ThemeManager.cpp
    src/ui/ThemeManager.h
    resources.qrc
)

if(WIN32)
    set(PROJECT_SOURCES ${PROJECT_SOURCES} resources.rc)
endif()

add_executable(Mp3Player WIN32 ${PROJECT_SOURCES})

# Добавляем пути к папкам, чтобы компилятор видел заголовочные файлы .h
target_include_directories(Mp3Player PRIVATE 
    ${CMAKE_SOURCE_DIR}/src 
    ${TAGLIB_INCLUDE_DIRS}
)

# Линкуем все библиотеки
target_link_libraries(Mp3Player PRIVATE
    Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets 
    Threads::Threads 
    m 
    ${CMAKE_DL_LIBS}
    ${TAGLIB_LIBRARIES}
)