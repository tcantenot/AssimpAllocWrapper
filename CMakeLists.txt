cmake_minimum_required(VERSION 3.21)

project("Assimp Wrapper" CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(DELAY_LOAD_ASSIMP_WRAPPER_SHARED_LIB "Delay the loading of the Assimp wrapper shared lib" ON)

# Assimp
# https://github.com/assimp/assimp/blob/master/Build.md
set(ASSIMP_BUILD_ZLIB           ON  CACHE STRING "" FORCE)
set(ENABLE_BOOST_WORKAROUND     ON  CACHE STRING "" FORCE)
set(BUILD_SHARED_LIBS           OFF CACHE STRING "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS   OFF CACHE STRING "" FORCE)
set(ASSIMP_BUILD_TESTS          OFF CACHE STRING "" FORCE)
set(ASSIMP_INSTALL              OFF CACHE STRING "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS   OFF CACHE STRING "" FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE STRING "" FORCE)
add_subdirectory(external/assimp EXCLUDE_FROM_ALL)
set_target_properties(assimp PROPERTIES FOLDER "external")
set_target_properties(zlibstatic PROPERTIES FOLDER "external")
add_library(ext::assimp ALIAS assimp)

# Assimp wrapper library
set(LIB_FOLDER src/AssimpWrapper)
set(LIB_SRC_FILES ${LIB_FOLDER}/AssimpWrapper.h ${LIB_FOLDER}/AssimpWrapper.cpp ${LIB_FOLDER}/AssimpWrapperAllocHooks.cpp)
add_library(AssimpWrapperLib SHARED ${LIB_SRC_FILES})
target_compile_definitions(AssimpWrapperLib PRIVATE "ASSIMP_WRAPPER_EXPORT")
target_include_directories(AssimpWrapperLib PUBLIC ${LIB_FOLDER})
target_link_libraries(AssimpWrapperLib PUBLIC ext::assimp)

# Main app
add_executable(AssimpWrapperMain src/main.cpp)
target_link_libraries(AssimpWrapperMain AssimpWrapperLib)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT AssimpWrapperMain)

# Delay load of Assimp wrapper DLL to improve startup time (Assimp performs a lot of static initializations)
if(DELAY_LOAD_ASSIMP_WRAPPER_SHARED_LIB)
	if(MSVC)
		target_link_options(AssimpWrapperMain PRIVATE "/DELAYLOAD:AssimpWrapperLib.dll")
	endif()
endif()
