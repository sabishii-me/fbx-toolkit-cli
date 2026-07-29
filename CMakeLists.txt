cmake_minimum_required(VERSION 3.20)
project(FBXToolkit VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(USE_SYSTEM_FBX_SDK "Use system-installed FBX SDK" OFF)

# Architecture detection
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(FBX_ARCH x64)
else()
    set(FBX_ARCH x86)
endif()

# Find or download FBX SDK
if(USE_SYSTEM_FBX_SDK)
    # Allow specifying FBX SDK path via environment variable or CMake variable
    if(NOT DEFINED FBX_SDK_ROOT AND DEFINED ENV{FBX_SDK_ROOT})
        set(FBX_SDK_ROOT $ENV{FBX_SDK_ROOT})
    endif()

    if(NOT DEFINED FBX_SDK_ROOT)
        if(WIN32)
            set(FBX_SDK_ROOT "C:/Program Files/Autodesk/FBX/FBX SDK/2020.3.7")
        elseif(APPLE)
            set(FBX_SDK_ROOT "/Applications/Autodesk/FBX SDK/2020.3.7")
        else()
            set(FBX_SDK_ROOT "/usr/local/fbx")
        endif()
    endif()

    message(STATUS "Using FBX SDK from: ${FBX_SDK_ROOT}")

    # Create imported target
    add_library(fbxsdk SHARED IMPORTED)

    if(WIN32)
        # Try multiple possible paths (vs2022, vs2019, or direct x64)
        if(EXISTS "${FBX_SDK_ROOT}/lib/${FBX_ARCH}/release/libfbxsdk.lib")
            set(FBX_LIB_PATH "${FBX_SDK_ROOT}/lib/${FBX_ARCH}")
        elseif(EXISTS "${FBX_SDK_ROOT}/lib/vs2022/${FBX_ARCH}/release/libfbxsdk.lib")
            set(FBX_LIB_PATH "${FBX_SDK_ROOT}/lib/vs2022/${FBX_ARCH}")
        else()
            set(FBX_LIB_PATH "${FBX_SDK_ROOT}/lib/vs2019/${FBX_ARCH}")
        endif()

        set_target_properties(fbxsdk PROPERTIES
            IMPORTED_LOCATION "${FBX_LIB_PATH}/release/libfbxsdk.dll"
            IMPORTED_IMPLIB "${FBX_LIB_PATH}/release/libfbxsdk.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${FBX_SDK_ROOT}/include"
        )
    elseif(APPLE)
        set_target_properties(fbxsdk PROPERTIES
            IMPORTED_LOCATION "${FBX_SDK_ROOT}/lib/clang/release/libfbxsdk.dylib"
            INTERFACE_INCLUDE_DIRECTORIES "${FBX_SDK_ROOT}/include"
        )
    else()
        # Try multiple possible Linux paths
        if(EXISTS "${FBX_SDK_ROOT}/lib/gcc/${FBX_ARCH}/release/libfbxsdk.so")
            set(FBX_LIB_PATH "${FBX_SDK_ROOT}/lib/gcc/${FBX_ARCH}/release/libfbxsdk.so")
        elseif(EXISTS "${FBX_SDK_ROOT}/lib/gcc4/${FBX_ARCH}/release/libfbxsdk.so")
            set(FBX_LIB_PATH "${FBX_SDK_ROOT}/lib/gcc4/${FBX_ARCH}/release/libfbxsdk.so")
        else()
            # Search for the library
            file(GLOB_RECURSE FBX_LIB_SEARCH "${FBX_SDK_ROOT}/lib/*/libfbxsdk.so")
            if(FBX_LIB_SEARCH)
                list(GET FBX_LIB_SEARCH 0 FBX_LIB_PATH)
                message(STATUS "Found FBX SDK library at: ${FBX_LIB_PATH}")
            else()
                message(FATAL_ERROR "Could not find libfbxsdk.so in ${FBX_SDK_ROOT}/lib")
            endif()
        endif()

        set_target_properties(fbxsdk PROPERTIES
            IMPORTED_LOCATION "${FBX_LIB_PATH}"
            INTERFACE_INCLUDE_DIRECTORIES "${FBX_SDK_ROOT}/include"
        )
    endif()
else()
    # Use vcpkg or FetchContent to get FBX SDK (if available)
    # For now, this will attempt to use find_package
    message(STATUS "Attempting to find FBX SDK via package manager...")

    # Try vcpkg first
    find_package(fbxsdk CONFIG QUIET)

    if(NOT fbxsdk_FOUND)
        message(FATAL_ERROR
            "FBX SDK not found. Please either:\n"
            "  1. Set USE_SYSTEM_FBX_SDK=ON and install FBX SDK manually\n"
            "  2. Set FBX_SDK_ROOT to your FBX SDK installation path\n"
            "  3. Set environment variable FBX_SDK_ROOT\n"
            "Download from: https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk-2020-3-7"
        )
    endif()
endif()

# Main executable
add_executable(fbx-toolkit
    src/main.cpp
    src/commands/bone_reset.cpp
    src/commands/axis_mender.cpp
)

# Include directories
target_include_directories(fbx-toolkit PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Link FBX SDK
target_link_libraries(fbx-toolkit PRIVATE fbxsdk)

# Platform-specific compile definitions
if(WIN32)
    target_compile_definitions(fbx-toolkit PRIVATE
        FBXSDK_SHARED
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# Set output directory
set_target_properties(fbx-toolkit PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Installation rules
install(TARGETS fbx-toolkit
    RUNTIME DESTINATION bin
)

# Copy FBX SDK libraries to output directory
if(USE_SYSTEM_FBX_SDK)
    if(WIN32)
        add_custom_command(TARGET fbx-toolkit POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${FBX_LIB_PATH}/release/libfbxsdk.dll"
                "$<TARGET_FILE_DIR:fbx-toolkit>"
            COMMENT "Copying FBX SDK DLL to output directory"
        )
    elseif(APPLE)
        add_custom_command(TARGET fbx-toolkit POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${FBX_SDK_ROOT}/lib/clang/release/libfbxsdk.dylib"
                "$<TARGET_FILE_DIR:fbx-toolkit>"
            COMMENT "Copying FBX SDK dylib to output directory"
        )
    endif()
endif()
