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
        source/camera.cpp
        source/camera.h
        source/secrets.h
        source/pokemon_api.cpp
        source/pokemon_api.h
)

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
        json-c
        curl
        mbedtls
        mbedx509
        mbedcrypto
        z)

if(COMMAND ctr_create_3dsx)
    ctr_create_3dsx(${PROJECT_NAME}
            NAME "3DS Pokedex"
            DESCRIPTION "Browse Pokemon with graphics"
    )
endif()
