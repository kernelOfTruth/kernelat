# - Try to find PWW
# Once done this will define
#
#  PWW_FOUND - system has PWW
#  PWW_INCLUDE_DIRS - the PWW include directory
#  PWW_LIBRARIES - Link these to use PWW
#  PWW_DEFINITIONS - Compiler switches required for using PWW
#
#  Copyright (c) 2011 Lee Hambley <lee.hambley@gmail.com>
#  Modified by Oleksandr Natalenko aka post-factum <pfactum@gmail.com>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

if (PWW_LIBRARIES AND PWW_INCLUDE_DIRS)
  # in cache already
  set(PWW_FOUND TRUE)
else (PWW_LIBRARIES AND PWW_INCLUDE_DIRS)

  find_path(PWW_INCLUDE_DIR
    NAMES
      libpww.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
  )

  find_library(PWW_LIBRARY
    NAMES
      libpww.so
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

  set(PWW_INCLUDE_DIRS
    ${PWW_INCLUDE_DIR}
  )

  if (PWW_LIBRARY)
    set(PWW_LIBRARIES
        ${PWW_LIBRARIES}
        ${PWW_LIBRARY}
    )
  endif (PWW_LIBRARY)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(PWW DEFAULT_MSG PWW_LIBRARIES PWW_INCLUDE_DIRS)

  # show the PWW_INCLUDE_DIRS and PWW_LIBRARIES variables only in the advanced view
  mark_as_advanced(PWW_INCLUDE_DIRS PWW_LIBRARIES)

endif (PWW_LIBRARIES AND PWW_INCLUDE_DIRS)

