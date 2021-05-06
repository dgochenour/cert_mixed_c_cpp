# (c) Copyright, Real-Time Innovations, 2021.  All rights reserved.
# RTI grants Licensee a license to use, modify, compile, and create derivative
# works of the software solely for use with RTI Connext DDS. Licensee may
# redistribute copies of the software provided that all such copies are subject
# to this license. The software is provided "as is", with no warranty of any
# type, including any warranty for fitness for any purpose. RTI is under no
# obligation to maintain or support the software. RTI shall not be liable for
# any incidental or consequential damages arising out of the use or inability
# to use the software.

cmake_minimum_required(VERSION 3.1)

PROJECT(example CXX C)
set(SOURCE_EXTENSION_CPP cxx)
set(SOURCE_EXTENSION_C c)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    SET(PLATFORM_LIBS dl nsl m pthread rt)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    SET(PLATFORM_LIBS dl m pthread )
endif()

set_property(GLOBAL PROPERTY RULE_MESSAGES OFF)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/objs/${RTIME_TARGET_NAME})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/objs/${RTIME_TARGET_NAME})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/objs/${RTIME_TARGET_NAME})

set(RTI_LIB_PREFIX rti_me)
set(RTI_LIB_SUFFIX)

if (${CMAKE_BUILD_TYPE} MATCHES "[Rr]elease")
    set(RTI_LIB_SUFFIX "z")
    set(RTI_DEBUG_BUILD FALSE)
    add_definitions(-DNDEBUG)
else()
    set(RTI_LIB_SUFFIX "zd")
    set(RTI_DEBUG_BUILD TRUE)
endif()

set(IDL_GEN_C
    ${CMAKE_CURRENT_SOURCE_DIR}/example.${SOURCE_EXTENSION_C}
    ${CMAKE_CURRENT_SOURCE_DIR}/examplePlugin.${SOURCE_EXTENSION_C}
    ${CMAKE_CURRENT_SOURCE_DIR}/exampleSupport.${SOURCE_EXTENSION_C}
)
set(IDL_GEN_H
    ${CMAKE_CURRENT_SOURCE_DIR}/example.h
    ${CMAKE_CURRENT_SOURCE_DIR}/examplePlugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/exampleSupport.h
)

SET(MICRO_C_LIBS rti_me${RTI_LIB_SUFFIX})
ADD_DEFINITIONS(-DRTI_CERT)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    $ENV{RTIMEHOME}/include 
)
link_directories($ENV{RTIMEHOME}/lib/${RTIME_TARGET_NAME})

################################################################################
# example_subscriber
################################################################################
add_executable(example_subscriber 
    ${CMAKE_CURRENT_SOURCE_DIR}/example_subscriber.${SOURCE_EXTENSION_CPP}
    ${IDL_GEN_C}
    ${IDL_GEN_H}
)

target_link_libraries(example_subscriber ${MICRO_C_LIBS} ${PLATFORM_LIBS})

set_target_properties(example_subscriber PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES 
    CXX_EXTENSIONS NO 
)

################################################################################
# example_publisher
################################################################################
add_executable(example_publisher
    ${CMAKE_CURRENT_SOURCE_DIR}/example_publisher.${SOURCE_EXTENSION_CPP}
    ${IDL_GEN_C}
    ${IDL_GEN_H}
)

target_link_libraries(example_publisher  ${MICRO_C_LIBS} ${PLATFORM_LIBS})

set_target_properties(example_publisher PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES 
    CXX_EXTENSIONS NO 
)


