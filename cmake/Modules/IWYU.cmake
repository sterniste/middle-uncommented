#.rst:
# include-what-you-use (iwyu)
# ----------------------------
#
# Allows to run the static code analyzer `include-what-you-use (iwyu)
# <http://include-what-you-use.org>`_ as a custom target with the build system
# `CMake <http://cmake.org>`_.
#
# .. topic:: Dependencies
#
#  This module requires the following *CMake* modules:
#
#  * ``FindPythonInterp``
#
# .. topic:: Contributors
#
#  * Florian Wolters <wolters.fl@gmail.com>

#===============================================================================
# Copyright 2015 Florian Wolters
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#===============================================================================

# ------------------------------------------------------------------------------
# Include guard for this file.
# ------------------------------------------------------------------------------

if(iwyu_included)
  return()
endif()

set(iwyu_included TRUE)

option(BUILD_IWYU
       "Run the include-what-you-use static analyzer on the source code of the project."
       OFF)

function(iwyu_enable)
  set(iwyu_EXECUTABLE_NAME include-what-you-use)
  find_program(iwyu_EXECUTABLE ${iwyu_EXECUTABLE_NAME})

  if(iwyu_EXECUTABLE)
    # This is not exactly the same behavior as with CMake v3.3, since here all
    # compiled targets are analyzed.
    set(iwyu_tool_EXECUTABLE_NAME iwyu_tool.py)

    find_package(PythonInterp)
    find_program(iwyu_tool_EXECUTABLE ${iwyu_tool_EXECUTABLE_NAME})

    if(PYTHONINTERP_FOUND AND iwyu_tool_EXECUTABLE)
      set(CMAKE_EXPORT_COMPILE_COMMANDS ON PARENT_SCOPE)

      add_custom_target(iwyu
                        ALL
                        COMMAND "${PYTHON_EXECUTABLE}" "${iwyu_tool_EXECUTABLE}" -p "${CMAKE_BINARY_DIR}" -- --max_line_length=999 ${iwyu_OPTIONS}
                        COMMENT "Running the ${iwyu_tool_EXECUTABLE_NAME} compilation database driver"
                        VERBATIM)
    else()
      message(STATUS
              "Unable to find the Python interpreter and/or the ${iwyu_tool_EXECUTABLE_NAME} script")
    endif()
  else()
    message(STATUS "Unable to find the ${iwyu_EXECUTABLE_NAME} executable")
  endif()
endfunction()
