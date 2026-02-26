# add_hierarchical_library - Create a library from a hierarchical source tree with recursive include paths
#
# Usage (similar to add_library):
#   add_hierarchical_library(<target> <STATIC|SHARED>
#     [SOURCE_EXTENSIONS ext1 ext2 ...]   # default: *.c *.cpp *.cc
#     [INCLUDE_EXTENSIONS ext1 ext2 ...]  # default: *.h *.hpp
#     [LINK_LIBRARIES lib1 lib2 ...]
#   )
#
# Recursively globs sources and headers from the current directory, collects all
# directories containing headers as include paths, and creates the library target.
#
function(add_hierarchical_library target type)
  cmake_parse_arguments(ARG "" "" "SOURCE_EXTENSIONS;INCLUDE_EXTENSIONS;LINK_LIBRARIES" ${ARGN})

  if(NOT ARG_SOURCE_EXTENSIONS)
    set(ARG_SOURCE_EXTENSIONS "*.c" "*.cpp" "*.cc")
  endif()
  if(NOT ARG_INCLUDE_EXTENSIONS)
    set(ARG_INCLUDE_EXTENSIONS "*.h" "*.hpp")
  endif()

  file(GLOB_RECURSE SRCS ${ARG_SOURCE_EXTENSIONS})
  file(GLOB_RECURSE HEADERS ${ARG_INCLUDE_EXTENSIONS})

  set(INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
  foreach(H ${HEADERS})
    get_filename_component(DIR ${H} DIRECTORY)
    list(APPEND INCLUDE_DIRS ${DIR})
  endforeach()
  if(INCLUDE_DIRS)
    list(REMOVE_DUPLICATES INCLUDE_DIRS)
  endif()

  add_library(${target} ${type} ${SRCS})
  target_include_directories(${target} PRIVATE ${INCLUDE_DIRS})
  if(ARG_LINK_LIBRARIES)
    target_link_libraries(${target} PRIVATE ${ARG_LINK_LIBRARIES})
  endif()
endfunction()
