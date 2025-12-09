cmake_minimum_required(VERSION 3.16)

project(kmemleak LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# SlabGrowthDetector executable
add_executable(SlabGrowthDetector
        SlabGrowthDetector/main.c
        SlabGrowthDetector/analysis.h
        SlabGrowthDetector/slabinfolist.h
        SlabGrowthDetector/vmstatlist.h
)

# JSlabLeakDetector executable
add_executable(JSlabLeakDetector
        JSlabLeakDetector/main.c
        JSlabLeakDetector/lists.c
        JSlabLeakDetector/DataStructures.h
        JSlabLeakDetector/slablist.h
)

# SingleFileJSlab executable
add_executable(SingleFileJSlab
        SingleFileJSlab/main.c
)

# Link math library for SingleFileJSlab
target_link_libraries(SingleFileJSlab PRIVATE m)

add_custom_target(qmltests SOURCES SlabGrowthDetector/tst_testcases.qml)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
install(TARGETS SlabGrowthDetector JSlabLeakDetector SingleFileJSlab
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
