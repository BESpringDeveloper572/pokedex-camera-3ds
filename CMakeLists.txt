cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 23)

project(3ds-app)

set(CMAKE_OSX_ARCHITECTURES "")

set(SOURCES source/main.cpp
        source/main.h
        source/pokemon_data.h
        source/display_manager.h
        source/display_manager.cpp
        source/input_handler.h
        source/input_handler.cpp
        source/app_state.h
        source/app_state.cpp
        source/text_to_speech.cpp
        source/text_to_speech.h
        source/pokemon_classifier_client.cpp
        source/pokemon_classifier_client.h
        source/secrets.h
        source/pokemon_api.cpp
        source/pokemon_api.h
)

include(FetchContent)

FetchContent_Declare(
        jsmn
        GIT_REPOSITORY https://github.com/zserge/jsmn
        GIT_TAG master # Or a specific version tag
)

if(NOT jsmn_POPULATED)
    FetchContent_MakeAvailable(jsmn)

    # Manually create the target because the repo lacks a CMakeLists.txt
    add_library(jsmn INTERFACE)

    # Point the target to the fetched source directory
    target_include_directories(jsmn INTERFACE ${jsmn_SOURCE_DIR})
endif()

add_executable(${PROJECT_NAME} ${SOURCES})

include_directories($ENV{DEVKITARM}/include/)
include_directories($ENV{DEVKITARM}/arm-none-eabi/include)
include_directories($ENV{DEVKITPRO}/libctru/include)
include_directories($ENV{DEVKITARM}/arm-none-eabi/include/c++/7.1.0)
include_directories($ENV{DEVKITPRO}/portlibs/3ds/include)

# Link against citro2d for graphics
link_directories($ENV{DEVKITPRO}/libctru/lib)
link_directories($ENV{DEVKITPRO}/portlibs/3ds/lib)


target_link_libraries(${PROJECT_NAME} PRIVATE
        citro2d
        citro3d
        ctru
        m
        flite_cmu_us_kal
        flite_cmulex
        flite_usenglish
        flite
        jsmn)

if(COMMAND ctr_create_3dsx)
    ctr_create_3dsx(${PROJECT_NAME}
            NAME "3DS Pokedex"
            DESCRIPTION "Browse Pokemon with graphics"
    )
endif()
