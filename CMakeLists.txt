CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(mmd)
SET(MMD_NAME "libmmd")
SET(MMD_DESCRIPTION "MikuMikuDance importer")
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${mmd_SOURCE_DIR}/CMake/modules)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY test)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY lib)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY lib)

# Build checks
# Abort git submodule init && git submodule update has not been ran.
SET(SUBMODULE_ERROR "Run 'git submodule update --init --recursive' update first.")
IF (NOT EXISTS ${mmd_SOURCE_DIR}/chck)
   SET(SUBMODULE_ERROR_THROW 1)
ENDIF ()

# Throw the error
IF (SUBMODULE_ERROR_THROW)
   MESSAGE(FATAL_ERROR ${SUBMODULE_ERROR})
ENDIF ()

# Warnings
IF (MSVC)
   IF (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
      STRING(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
   ELSE ()
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
   ENDIF ()
ELSEIF (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
   SET(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wall -Wextra -Wno-variadic-macros -Wno-long-long")
   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-variadic-macros -Wno-long-long")
ENDIF ()

IF (UNIX AND (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX))
   SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
   IF (${CMAKE_VERSION} VERSION_LESS 2.8.9)
      ADD_DEFINITIONS(-fPIC)
   ENDIF ()
ENDIF ()

INCLUDE_DIRECTORIES(
   ${mmd_SOURCE_DIR}/chck/buffer
   ${mmd_SOURCE_DIR}/chck/sjis)
SET(MMD_SRC mmd.c chck/buffer/buffer.c chck/sjis/sjis.c)
ADD_LIBRARY(mmd ${MMD_SRC})
TARGET_LINK_LIBRARIES(mmd)

# vim: set ts=8 sw=3 tw=0
