cmake_minimum_required(VERSION 3.10)
project(TiffToRaw LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动处理 Qt5 moc, uic, rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 设置二进制可执行文件输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

# MSVC 编译选项配置
if(MSVC)
    add_compile_options(/W4 /utf-8 /wd4251 /wd4275)
endif()

# 1. GDAL 寻找与配置逻辑
find_package(GDAL QUIET)
set(GDAL_DIR "" CACHE PATH "GDAL 根安装目录路径")
set(GDAL_INCLUDE_DIR "" CACHE PATH "GDAL 头文件目录路径 (包含 gdal_priv.h)")
set(GDAL_LIBRARY "" CACHE FILEPATH "GDAL 库文件路径 (例如 gdal_i.lib)")

if(NOT GDAL_FOUND)
    if(GDAL_DIR AND NOT GDAL_INCLUDE_DIR)
        if(EXISTS "${GDAL_DIR}/include")
            set(GDAL_INCLUDE_DIR "${GDAL_DIR}/include")
        endif()
    endif()
    
    if(GDAL_DIR AND NOT GDAL_LIBRARY)
        if(EXISTS "${GDAL_DIR}/lib/vc140/x64/Release/gdal_i.lib")
            set(GDAL_LIBRARY "${GDAL_DIR}/lib/vc140/x64/Release/gdal_i.lib")
        elseif(EXISTS "${GDAL_DIR}/lib/gdal_i.lib")
            set(GDAL_LIBRARY "${GDAL_DIR}/lib/gdal_i.lib")
        elseif(EXISTS "${GDAL_DIR}/lib/gdal.lib")
            set(GDAL_LIBRARY "${GDAL_DIR}/lib/gdal.lib")
        endif()
    endif()

    if(GDAL_INCLUDE_DIR AND GDAL_LIBRARY)
        set(GDAL_FOUND TRUE)
        set(GDAL_INCLUDE_DIRS ${GDAL_INCLUDE_DIR})
        set(GDAL_LIBRARIES ${GDAL_LIBRARY})
    endif()
endif()

# 2. Qt5 寻找与配置逻辑
if(NOT DEFINED QT5_INSTALL_DIR OR QT5_INSTALL_DIR STREQUAL "")
    if(DEFINED ENV{QT_DIR})
        set(QT5_INSTALL_DIR "$ENV{QT_DIR}" CACHE PATH "Qt5 根目录路径")
    elseif(DEFINED ENV{Qt5_DIR})
        set(QT5_INSTALL_DIR "$ENV{Qt5_DIR}" CACHE PATH "Qt5 根目录路径")
    else()
        set(QT5_INSTALL_DIR "D:/HR/DEV/ThirdParty/Qt5.11/vc140/x64" CACHE PATH "Qt5 根目录路径")
    endif()
endif()

if(EXISTS "${QT5_INSTALL_DIR}")
    list(APPEND CMAKE_PREFIX_PATH "${QT5_INSTALL_DIR}")
endif()

find_package(Qt5 COMPONENTS Widgets Core Gui REQUIRED)

# -------------------------------------------------------------
# 统一二合一可执行目标: TiffToRawGui (GUI + CLI 模式)
# -------------------------------------------------------------
add_executable(TiffToRawGui WIN32
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/ConversionWorker.cpp
    src/ConversionWorker.h
    src/GeoTiffConverter.cpp
    src/GeoTiffConverter.h
    src/RawTerrainReader.cpp
    src/RawTerrainReader.h
    src/PointPickerWidget.cpp
    src/PointPickerWidget.h
    src/GeoGazetteer.cpp
    src/GeoGazetteer.h
    resources/resources.qrc
)

target_include_directories(TiffToRawGui PRIVATE ${GDAL_INCLUDE_DIRS})
target_link_libraries(TiffToRawGui PRIVATE ${GDAL_LIBRARIES} Qt5::Widgets Qt5::Core Qt5::Gui)

# 自动拷贝样式表至输出路径
add_custom_command(TARGET TiffToRawGui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/themestyle.qss"
        $<TARGET_FILE_DIR:TiffToRawGui>
)

# 若存在本地 Qt5 运行时二进制，自动拷贝 Qt 运行时 DLL 驱动库及平台插件
if(EXISTS "${QT5_INSTALL_DIR}/bin/Qt5Core.dll")
    add_custom_command(TARGET TiffToRawGui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT5_INSTALL_DIR}/bin/Qt5Core.dll"
            "${QT5_INSTALL_DIR}/bin/Qt5Gui.dll"
            "${QT5_INSTALL_DIR}/bin/Qt5Widgets.dll"
            $<TARGET_FILE_DIR:TiffToRawGui>
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TiffToRawGui>/platforms
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT5_INSTALL_DIR}/plugins/platforms/qwindows.dll"
            $<TARGET_FILE_DIR:TiffToRawGui>/platforms
    )
endif()
