cmake_minimum_required(VERSION 3.16)

project(kmemleak LANGUAGES C)

add_executable(kmemleak main.c
    vmstatlist.h
    slabinfolist.h
    analysis.h
    tst_testcases.qml)

include(GNUInstallDirs)
install(TARGETS kmemleak
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
