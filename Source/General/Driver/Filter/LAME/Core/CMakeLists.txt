cmake_minimum_required(VERSION 3.0)

project (mp3lame)

add_definitions(-DHAVE_STDINT_H)
add_definitions(-DHAVE_MPGLIB)
add_definitions(-DDECODE_ON_THE_FLY)
add_definitions(-DUSE_FAST_LOG)
add_definitions(-DTAKEHIRO_IEEE754_HACK)

if((${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC))
add_definitions(-DUSE_LAYER_2)
add_definitions(-DHAVE_CONFIG_H)
add_definitions(-DSTDC_HEADERS)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-D_CRT_SECURE_NO_DEPRECATED)
configure_file(configMS.h config.h @ONLY)
else()
add_definitions(-DSTDC_HEADERS)
if(NOT ieee754_float32_t)
add_definitions(-Dieee754_float32_t=float)
endif()
endif()

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/libmp3lame SOURCE_LIB)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/libmp3lame/vector/ SOURCE_LIB)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/mpglib SOURCE_LIB)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libmp3lame)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mpglib)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_library(${PROJECT_NAME} STATIC ${SOURCE_LIB})

target_include_directories(${PROJECT_NAME} 
PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)