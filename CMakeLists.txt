cmake_minimum_required(VERSION 3.16)

project(kmemleak LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(kmemleak
    main.c
)

target_sources(kmemleak PRIVATE
    analysis.h
    slabinfolist.h
    vmstatlist.h
)

add_custom_target(qmltests SOURCES tst_testcases.qml)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
install(TARGETS kmemleak
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
