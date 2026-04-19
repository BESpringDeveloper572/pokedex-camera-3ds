cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 11)

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
        source/app_state.cpp)

include_directories($ENV{DEVKITARM}/include/)
include_directories($ENV{DEVKITARM}/arm-none-eabi/include)
include_directories($ENV{DEVKITPRO}/libctru/include)
include_directories($ENV{DEVKITARM}/arm-none-eabi/include/c++/7.1.0)

# Link against citro2d for graphics
link_directories($ENV{DEVKITPRO}/libctru/lib)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE citro2d citro3d ctru m)

if(COMMAND ctr_create_3dsx)
    ctr_create_3dsx(${PROJECT_NAME}
            NAME "3DS Pokedex"
            DESCRIPTION "Browse Pokemon with graphics"
    )
endif()