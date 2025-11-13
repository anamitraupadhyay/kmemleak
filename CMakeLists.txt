cmake_minimum_required(VERSION 3.16)

project(kmemleak LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(kmemleak
        SlabGrowthDetector/main.c
        JSlabLeakDetector/main.c
        JSlabLeakDetector/DataStructures.c
        JSlabLeakDetector/DataStructures.h
)

target_sources(kmemleak PRIVATE
        SlabGrowthDetector/analysis.h
        SlabGrowthDetector/slabinfolist.h
        SlabGrowthDetector/vmstatlist.h
)

add_custom_target(qmltests SOURCES SlabGrowthDetector/tst_testcases.qml)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
install(TARGETS kmemleak
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
