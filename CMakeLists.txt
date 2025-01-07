cmake_minimum_required(VERSION 3.5)

project(intelligence_shoepad_app VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# add -g flag for debug
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")
set(CMAKE_CFLAGS_DEBUG "${CMAKE_CFLAGS_DEBUG} -g")
set(CMAKE_CFLAGS_RELEASE "${CMAKE_CFLAGS_RELEASE} -g")
set(CMAKE_LFLAGS_RELEASE "${CMAKE_LFLAGS_RELEASE} -mthreads -Wl")

# QMAKE_CXXFLAGS += -fexceptions
# QMAKE_LFLAGS_EXCEPTIONS = -Wl,-enable-stdcall-fixup
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-enable-stdcall-fixup")

# set output directory to bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools Core Charts Network Mqtt)

add_compile_definitions("DEBUG=$<CONFIG:Debug>")

# include libbroker.h and add libbroker.a static library
# execute_process(COMMAND go build -buildmode=c-archive -o ./lib/libbroker.a WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/broker)
add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/broker/lib/libbroker.a
    COMMAND go build -buildmode=c-archive -o ./lib/libbroker.a
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/broker
    DEPENDS ${CMAKE_SOURCE_DIR}/broker/main.go
    COMMENT "Building libbroker.a"
)
add_custom_target(broker_lib ALL DEPENDS ${CMAKE_SOURCE_DIR}/broker/lib/libbroker.a)

# add library that depands on libbroker.a
add_library(broker STATIC IMPORTED GLOBAL)
add_dependencies(broker broker_lib)
set_target_properties(broker PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/broker/lib/libbroker.a)
target_include_directories(broker INTERFACE ${CMAKE_SOURCE_DIR}/broker/lib)

set(SRC_DIR src)
set(INC_DIR inc)

set(TS_FILES ${SRC_DIR}/intelligence_shoepad_app_en_HK.ts)

file(GLOB SRC_FILES ${SRC_DIR}/*.cpp ${SRC_DIR}/*.ui)
file(GLOB HEADERS ${INC_DIR}/*.h ${INC_DIR}/*.hpp)

set(PROJECT_SOURCES
    ${SRC_FILES}
    ${HEADERS}
    ${TS_FILES}
)

include_directories(${INC_DIR})

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )

# Define target properties for Android with Qt 6 as:
# set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
# ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(${PROJECT_NAME} SHARED
            ${PROJECT_SOURCES}
        )

    # Define properties for Android with Qt 5 after find_package() calls as:
    # set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(${PROJECT_NAME}
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Charts)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Network)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Mqtt)

target_link_libraries(${PROJECT_NAME} PRIVATE broker)

# link dbghelp.lib for stack trace
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE dbghelp.lib)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

# run windeployqt to bin folder
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND windeployqt --verbose 1 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

# copy images/ to bin folder if different
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/images $<TARGET_FILE_DIR:${PROJECT_NAME}>/images)
