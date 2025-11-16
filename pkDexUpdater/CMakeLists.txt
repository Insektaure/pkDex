# pkDexUpdater CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

# Project info
project(pkDexUpdater)
set(VERSION_MAJOR "1")
set(VERSION_MINOR "0")
set(VERSION_ALTER "1")
set(VERSION_BUILD "0")
set(PACKAGE_NAME "com.switch.pkdexupdater")
set(PROJECT_AUTHOR "Insektaure")
set(PROJECT_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/img/icon.jpg)
set(PROJECT_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources)

# Find required packages
find_package(Threads REQUIRED)
list(APPEND APP_PLATFORM_LIB ${CMAKE_THREAD_LIBS_INIT})

# Setting src and include
list(APPEND APP_PLATFORM_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(GLOB_RECURSE UPDATER_SRC src/*.cpp)

# Platform-specific settings for Switch
if (PLATFORM_SWITCH)
    set(APP_PLATFORM_LIB
        # base lib
        nx m
    )
endif ()

# Building target
program_target(${PROJECT_NAME} "${UPDATER_SRC}")
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

# Building NRO for Switch
if (PLATFORM_SWITCH)
    add_custom_target(${PROJECT_NAME}.nro DEPENDS ${PROJECT_NAME}
        COMMAND ${NX_NACPTOOL_EXE} --create "${PROJECT_NAME}" "${PROJECT_AUTHOR}" "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_ALTER}" ${PROJECT_NAME}.nacp --titleid=${PROJECT_TITLEID}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_RESOURCES} ${CMAKE_BINARY_DIR}/pkDexUpdater/resources
        COMMAND ${NX_ELF2NRO_EXE} ${PROJECT_NAME}.elf ${PROJECT_NAME}.nro --icon=${PROJECT_ICON} --nacp=${PROJECT_NAME}.nacp --romfsdir=${CMAKE_BINARY_DIR}/pkDexUpdater/resources
    )
endif ()

# Link libraries
target_include_directories(${PROJECT_NAME} PRIVATE ${APP_PLATFORM_INCLUDE})
target_compile_options(${PROJECT_NAME} PRIVATE -ffunction-sections -fdata-sections)
target_link_libraries(${PROJECT_NAME} PRIVATE ${APP_PLATFORM_LIB})
