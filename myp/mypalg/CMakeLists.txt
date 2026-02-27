cmake_minimum_required(VERSION 3.14)

# ---------------------------------------------------------------------------
# Package: mypalg
# ---------------------------------------------------------------------------

set(COMPONENT mypalg_twosum)

# Library target
add_library(${COMPONENT} ${COMPONENT}.cpp)

target_include_directories(${COMPONENT} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

set_target_properties(${COMPONENT} PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

# Test driver target
add_executable(${COMPONENT}.t ${COMPONENT}.t.cpp)

target_link_libraries(${COMPONENT}.t PRIVATE ${COMPONENT})

set_target_properties(${COMPONENT}.t PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

# Register test cases 1-5 with CTest
foreach(CASE RANGE 1 5)
    add_test(
        NAME "${COMPONENT}.t.case${CASE}"
        COMMAND ${COMPONENT}.t ${CASE} v
    )
endforeach()
