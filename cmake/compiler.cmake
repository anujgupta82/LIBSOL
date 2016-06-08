SET(CMAKE_DEBUG_POSTFIX  "d")

if (STATIC_LIB)
    set(LIB_TYPE STATIC)
else()
    set(LIB_TYPE SHARED)
endif()

IF(CMAKE_COMPILER_IS_GNUCXX)
    cmake_policy(SET CMP0022 NEW)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")
    if (NOT USE_STD_THREAD)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_PTHREAD")
    endif()

    #rpath setting
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    #add the automatically determined parts of the rpath
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

    # the RPATH to be used when installing, but only if it's not a system directory
    LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}" isSystemDir)
    #IF("${isSystemDir}" STREQUAL "-1")
    #    SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}")
    #ENDIF("${isSystemDir}" STREQUAL "-1")
    SET(CMAKE_INSTALL_RPATH "$ORIGIN")

    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release)
    endif()
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        set(CMAKE_VERBOSE_MAKEFILE ON)
    endif()

ENDIF()

#copied from opencv
# turns off warnings
macro(ocv_warnings_disable)
    if(NOT ENABLE_NOISY_WARNINGS)
        set(_flag_vars "")
        set(_msvc_warnings "")
        set(_gxx_warnings "")
        foreach(arg ${ARGN})
            if(arg MATCHES "^CMAKE_")
                list(APPEND _flag_vars ${arg})
            elseif(arg MATCHES "^/wd")
                list(APPEND _msvc_warnings ${arg})
            elseif(arg MATCHES "^-W")
                list(APPEND _gxx_warnings ${arg})
            endif()
        endforeach()
        if(MSVC AND _msvc_warnings AND _flag_vars)
            foreach(var ${_flag_vars})
                foreach(warning ${_msvc_warnings})
                    set(${var} "${${var}} ${warning}")
                endforeach()
            endforeach()
        elseif((CMAKE_COMPILER_IS_GNUCXX OR (UNIX AND CV_ICC)) AND _gxx_warnings AND _flag_vars)
            foreach(var ${_flag_vars})
                foreach(warning ${_gxx_warnings})
                    if(NOT warning MATCHES "^-Wno-")
                        string(REPLACE "${warning}" "" ${var} "${${var}}")
                        string(REPLACE "-W" "-Wno-" warning "${warning}")
                    endif()
                    ocv_check_flag_support(${var} "${warning}" _varname)
                    if(${_varname})
                        set(${var} "${${var}} ${warning}")
                    endif()
                endforeach()
            endforeach()
        endif()
        unset(_flag_vars)
        unset(_msvc_warnings)
        unset(_gxx_warnings)
    endif(NOT ENABLE_NOISY_WARNINGS)
endmacro()

IF(MSVC)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_DEPRECATE")
    IF ("${CMAKE_BUILD_TYPE}" STREQUAL "")
        set (CMAKE_BUILD_TYPE "Debug|Release")
    ENDIF()

    if (NOT USE_STD_THREAD)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_WIN_THREAD")
    endif()

    ocv_warnings_disable(CMAKE_CXX_FLAGS /wd4251) # class 'std::XXX' needs to have dll-interface to be used by clients of YYY
    ocv_warnings_disable(CMAKE_CXX_FLAGS /wd4275) # non dll-interface class 'std::XXX' used as base for dll-interface 
ENDIF()


