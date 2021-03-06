
#
# CMake config
#
CMAKE_MINIMUM_REQUIRED(VERSION 2.4 FATAL_ERROR)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# If the Source dir and the Binary dir are the same, we are building in-source, which we will disallow due to Autotools being there (but only on non-Windows)
if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR} AND NOT WIN32)
  message(FATAL_ERROR "You can not use CMake to build ANT from the root of it's source tree! Remove the CMakeCache.txt file from this directory, then create a separate directory (either below this directory or elsewhere), and then re-run CMake from there.")
endif(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR} AND NOT WIN32)

#
# Project name
#
PROJECT(ant CXX)
enable_language(C)

# Include our navn.cmake here
SET(CMAKE_MODULE_PATH ${ant_SOURCE_DIR}/cmake)
include(navn)

# Project version
SET(${PROJECT_NAME}_MAJOR_VERSION 1)
SET(${PROJECT_NAME}_MINOR_VERSION 0)
SET(${PROJECT_NAME}_PATCH_LEVEL 0)
SET(${PROJECT_NAME}_VERSION_EXTRA -Dachau-Core)

# Get the git revision location for the branch we're on
if(EXISTS "${ant_SOURCE_DIR}/.git/HEAD")
  read_from_file(${ant_SOURCE_DIR}/.git/HEAD ".*" GIT_HEAD_LOC)
  string(LENGTH ${GIT_HEAD_LOC} HEAD_LEN)
  math(EXPR LEN "${HEAD_LEN} - 5")
  string(SUBSTRING ${GIT_HEAD_LOC} 5 ${LEN} GIT_HEAD)
endif(EXISTS "${ant_SOURCE_DIR}/.git/HEAD")

# Get the git revision we're on for the version string
if(EXISTS "${ant_SOURCE_DIR}/.git/${GIT_HEAD}")
  read_from_file(${ant_SOURCE_DIR}/.git/${GIT_HEAD} ".*" VERSION_STR)
  # Get the length of the string
  string(LENGTH ${VERSION_STR} VERSION_LEN)
  # Subtract 7 from the string's length
  math(EXPR VERSION_NUM_LEN "${VERSION_LEN} - ${VERSION_LEN} + 7")
  # Extract the value from the string
  string(SUBSTRING ${VERSION_STR} 0 ${VERSION_NUM_LEN} VERSION_GIT)
endif(EXISTS "${ant_SOURCE_DIR}/.git/${GIT_HEAD}")

# Set our version strings
SET(VERSION_SIMPLE "${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION}.${${PROJECT_NAME}_PATCH_LEVEL}")
SET(VERSION_FULL "${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION}.${ant_PATCH_LEVEL}-${VERSION_GIT}${${PROJECT_NAME}_VERSION_EXTRA}")

# because some of this is copypasta we set this var here
SET(PROGRAM_NAME ${PROJECT_NAME})

# Start with empty defaults for library and include directories, to be used by GNU compilers only
set(DEFAULT_LIBRARY_DIRS)
set(DEFAULT_INCLUDE_DIRS)

# If we are using a GNU compiler (have to use CXX because it seems to fail on C), we will be able to determine it's default paths for libraries and includes
if(CMAKE_COMPILER_IS_GNUCXX)
  # First look for the compiler's default library directories
  execute_process(COMMAND ${CMAKE_C_COMPILER} -print-search-dirs OUTPUT_VARIABLE LINES OUTPUT_STRIP_TRAILING_WHITESPACE)
  # Find only the part after "libraries: "
  string(REGEX REPLACE ".*\nlibraries: (.*)$" "\\1" LINE "${LINES}")
  # Replace the colons in the list with semicolons (only when not on MinGW, which uses semicolons already), and if on MinGW, just copy the line
  if(NOT MINGW)
    string(REGEX REPLACE ":" ";" LIBRARIES ${LINE})
  else(NOT MINGW)
    set(LIBRARIES "${LINE}")
  endif(NOT MINGW)
  # Iterate through the libraries
  foreach(LIBRARY ${LIBRARIES})
    # Check if the first character is an equal sign, and skip that library directory as it is (I believe) the primary default and shows up later in the list anyways
    string(SUBSTRING ${LIBRARY} 0 1 FIRST_CHAR)
    if(NOT FIRST_CHAR STREQUAL "=")
      # If the directory had no = in front of it, make sure it's absolute and add it to the list of default library directories
      get_filename_component(LIBRARY ${LIBRARY} ABSOLUTE)
      append_to_list(DEFAULT_LIBRARY_DIRS ${LIBRARY})
    endif(NOT FIRST_CHAR STREQUAL "=")
  endforeach(LIBRARY)
  # Remove duplicate entries from the list
  if(DEFAULT_LIBRARY_DIRS)
    remove_list_duplicates(DEFAULT_LIBRARY_DIRS)
  endif(DEFAULT_LIBRARY_DIRS)
  # Create a temporary file to test for the default include directories
  FILE(WRITE empty.cpp "")
  # Next, we look for the compiler's default include directories
  # Run the command to find the default include directories
  execute_process(COMMAND ${CMAKE_C_COMPILER} -v -x c++ -E ${CMAKE_CURRENT_SOURCE_DIR}/empty.cpp ERROR_VARIABLE LINES OUTPUT_QUIET ERROR_STRIP_TRAILING_WHITESPACE)
  # Remove the empty file, it is no longer needed
  FILE(REMOVE empty.cpp)
  # Convert the new lines to semicolons
  string(REGEX REPLACE "\n" ";" LINES ${LINES})
  # Temporary variable saying if we are in the search list or not
  set(IN_SEARCH_LIST FALSE)
  # Iterate through the lines
  foreach(LINE ${LINES})
    # If the line has the following on it, the next lines will contain directory names
    if(LINE STREQUAL "#include <...> search starts here:")
      set(IN_SEARCH TRUE)
    else(LINE STREQUAL "#include <...> search starts here:")
      # If the line has the following on it, we hit the end of the list
      if(LINE STREQUAL "End of search list.")
        set(IN_SEARCH FALSE)
      else(LINE STREQUAL "End of search list.")
        # If we are within the block between the above two lines...
        if(IN_SEARCH)
          # Get everything but the first character of the line
          string(LENGTH ${LINE} LINE_LENGTH)
          math(EXPR LINE_LENGTH "${LINE_LENGTH} - 1")
          string(SUBSTRING ${LINE} 1 ${LINE_LENGTH} INCLUDE)
          # Convert the path to an absolute one, just in case it wasn't
          get_filename_component(INCLUDE ${INCLUDE} ABSOLUTE)
          # Add that directory to the list of default include directories
          append_to_list(DEFAULT_INCLUDE_DIRS ${INCLUDE})
        endif(IN_SEARCH)
      endif(LINE STREQUAL "End of search list.")
    endif(LINE STREQUAL "#include <...> search starts here:")
  endforeach(LINE)
  # Remove duplicate entries from the list
  if(DEFAULT_INCLUDE_DIRS)
    remove_list_duplicates(DEFAULT_INCLUDE_DIRS)
  endif(DEFAULT_INCLUDE_DIRS)
endif(CMAKE_COMPILER_IS_GNUCXX)

# If the user specifies -DCMAKE_BUILD_TYPE on the command line, take their definition
# and dump it in the cache along with proper documentation, otherwise set CMAKE_BUILD_TYPE
# to Debug
# Only do this if not using Visual Studio
if(NOT MSVC)
  if(CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
  else(CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE DEBUG CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
  endif(CMAKE_BUILD_TYPE)
endif(NOT MSVC)

# Add an optional variable for using run-cc.pl for building, Perl will be checked later regardless of this setting
option(USE_RUN_CC_PL "Use run-cc.pl for building" OFF)
# Add an optional variable for using compile options that only a paranoid person would use
option(USE_PARANOID "Use paranoid compile options" OFF)

INCLUDE_DIRECTORIES(${ant_SOURCE_DIR}/src ${ant_SOURCE_DIR}/include ${CMAKE_INSTALL_PREFIX}/include)

set(ENV{LC_ALL} C)
SOURCE_GROUP(${PROJECT_NAME} FILES ${ant_source})

include(CheckFunctionExists)
include(CheckCXXSourceCompiles)
include(CheckIncludeFile)
include(CheckTypeSize)
include(CheckLibraryExists)
if(CMAKE244_OR_BETTER)
  include(CheckCXXCompilerFlag)
else(CMAKE244_OR_BETTER)
  include(TestCXXAcceptsFlag)
endif(CMAKE244_OR_BETTER)

# If using Visual Studio, set the C++ flags accordingly
if(MSVC)
  # Remove the default exception handling flags, also remove default warning level flag
  string(REPLACE "/EHsc " "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  string(REPLACE "/GX " "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  string(REPLACE "/W3 " "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  # Set the compile flags to have warnings on the max setting (but disable a few annoying ones), exception handling turned on, the proper defines
  set(CXXFLAGS "${CXXFLAGS} /W4 /wd4100 /wd4251 /wd4706 /wd4800 /wd4996 /EHs")
  add_definitions(-DMSVCPP -D_CRT_SECURE_NO_WARNINGS)
# Otherwise, we're not using Visual Studio
else(MSVC)
  # Set the compile flags to have all warnings on (including shadowed variables)
  set(CXXFLAGS "${CXXFLAGS} -Wall -Wshadow")
  if(USE_PARANOID)
    set(CXXFLAGS "${CXXFLAGS} -O -W -Weffc++ -pedantic-errors -Wextra -Waggregate-return -Wcast-align -Wcast-qual -Wchar-subscripts -Wcomment -Wconversion -Wdisabled-optimization -Wfloat-equal -Wformat -Wformat=2 -Wformat-nonliteral -Wformat-security -Wformat-y2k -Wimport -Winit-self -Winline -Winvalid-pch -Wunsafe-loop-optimizations -Wlong-long -Wmissing-braces -Wmissing-field-initializers -Wmissing-format-attribute -Wmissing-include-dirs -Wmissing-noreturn -Wpacked -Wpadded -Wparentheses -Wpointer-arith -Wredundant-decls -Wreturn-type -Wsequence-point -Wsign-compare -Wstack-protector -Wstrict-aliasing -Wstrict-aliasing=2 -Wswitch  -Wswitch-default -Wswitch-enum -Wtrigraphs -Wuninitialized -Wunknown-pragmas -Wunreachable-code -Wunused -Wunused-function -Wunused-label -Wunused-parameter -Wunused-value -Wunused-variable -Wvariadic-macros -Wvolatile-register-var -Wwrite-strings")
  endif(USE_PARANOID)
  # If on a *nix system, also set the compile flags to remove GNU extensions (favor ISO C++) as well as reject non-ISO C++ code, also remove all leading underscores in exported symbols (only on GNU compiler)
  if(UNIX)
    set(CXXFLAGS "${CXXFLAGS} -ansi -pedantic")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      	check_cxx_source_compiles(
		"#if __GNUC__ < 4
		#error
		#endif
		#if __GNUC_MINOR__ < 5
		#error
		#endif
		int main() {}"
		GCC_IS_GCC4)
      if(NOT GCC_IS_GCC4)
	message(FATAL_ERROR "You MUST use GNU C++ compiler version 4.5 and above to compile! (or a compiler that supports C++11)")
      endif(NOT GCC_IS_GCC4)
      set(CXXFLAGS "${CXXFLAGS} -fno-leading-underscore -Wno-uninitialized")

	# GCC 4.8 added clang++ style error messages which include a caret showing what's wrong,
	# personally I hate this so I am disabling it.
	if(CMAKE244_OR_BETTER)
	    check_cxx_compiler_flag(-fno-diagnostics-show-caret HAVE_NO_DIAGNOSTICS_CARET)
	else(CMAKE244_OR_BETTER)
	    check_cxx_accepts_flag(-fno-diagnostics-show-caret HAVE_NO_DIAGNOSTICS_CARET)
	endif(CMAKE244_OR_BETTER)

	if(HAVE_NO_DIAGNOSTICS_CARET)
	    set(CXXFLAGS "${CXXFLAGS} -fno-diagnostics-show-caret")
	endif(HAVE_NO_DIAGNOSTICS_CARET)

    endif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # If we aren't on a *nix system, we are using MinGW
  else(UNIX)
    # Also, if we are building under MinGW, add another define for MinGW
    if(MINGW)
      add_definitions(-DMINGW)
    endif(MINGW)
  endif(UNIX)
endif(MSVC)

# If extra directories were specified, tell cmake about them.
if(EXTRA_INCLUDE)
  include_directories(${EXTRA_INCLUDE})
  link_directories(${EXTRA_INCLUDE})
endif(EXTRA_INCLUDE)

# If running under MinGW, we have to force the resource compiler settings (hopefully this will be fixed in a later version of CMake)
if(MINGW)
  set(CMAKE_RC_COMPILER_INIT windres)
  enable_language(RC)
  set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> <DEFINES> -o <OBJECT> <SOURCE>")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS}")
endif(MINGW)

# If we are not using Visual Studio, we'll run the following checks
if(NOT MSVC)
  # Check if the C++ compiler can accept the -pipe flag, and add it to the compile flags if it works
  if(CMAKE244_OR_BETTER)
    # If using CMake 2.4.4 or better, we can use check_cxx_compiler_flag
    check_cxx_compiler_flag(-pipe HAVE_PIPE_FLAG)
    check_cxx_compiler_flag(-std=gnu++0x HAVE_CXX_11)
  else(CMAKE244_OR_BETTER)
    # If using CMake 2.4.3 or older, we will use check_cxx_accepts_flags instead
    check_cxx_accepts_flag(-pipe HAVE_PIPE_FLAG)
    check_cxx_accepts_flag(-std=gnu++0x HAVE_CXX_11)
  endif(CMAKE244_OR_BETTER)
  # If the flag was accepted, add it to the list of flags
  if(HAVE_CXX_11)
    set(CXXFLAGS "${CXXFLAGS} -std=gnu++0x")
  else(HAVE_CXX_11)
    message(FATAL_ERROR "Your compiler MUST support C++11 (at least gnu C++ version 4.5)")
  endif(HAVE_CXX_11)
  if(HAVE_PIPE_FLAG)
    set(CXXFLAGS "${CXXFLAGS} -pipe")
  endif(HAVE_PIPE_FLAG)

  # The following are additional library checks, they are not required for Windows
  if(NOT WIN32)
    # Check if socket is within the socket library (if the library exists), and add it to the linker flags if needed
    check_library_exists(socket socket "" HAVE_SOCKET_LIB)
    if(HAVE_SOCKET_LIB)
      append_to_list(LINK_LIBS socket)
    endif(HAVE_SOCKET_LIB)

    # Check if we have CPPCMS Framework for the web frontend.
    # FIXME: This should find the damn library files not programs
    find_program(CPPCMS_TMPL_CC cppcms_tmpl_cc)
    find_file(CPPCMS_FOUND forwarder.h ${CMAKE_INSTALL_PREFIX}/include/cppcms/)
    message("-- Looking for CPPCMS library")
    if(CPPCMS_TMPL_CC AND CPPCMS_FOUND)
      message("-- Looking for CPPCMS library - found")
      append_to_list(LINK_LIBS cppcms)
    else(CPPCMS_TMPL_CC AND CPPCMS_FOUND)
      message("-- Looking for CPPCMS library - not found")
      message(FATAL_ERROR "CPPCMS is required for compile! Either CPPCMS is not installed or installed incorrectly, look in libraries directory for library package")
    endif(CPPCMS_TMPL_CC AND CPPCMS_FOUND)
    # Check if we have CPPDB Database Backend for accounts, metadata storage, etc.
    # FIXME: Does this even work??
    find_file(CPPDB_FOUND errors.h frontend.h ${CMAKE_INSTALL_PREFIX}/include/cppdb/)
    message("-- Looking for CPPDB library")
    if(CPPDB_FOUND)
      message("-- Looking for CPPDB library - found")
      append_to_list(LINK_LIBS cppdb)
    else(CPPDB_FOUND)
      message("-- Looking for CPPDB library - not found")
      message("CPPDB is required for future compile! look in libraries directory for library package")
    endif(CPPDB_FOUND)

    # Check if inet_addr is within the nsl library (if the library exists), and add it to the linker flags if needed
    check_library_exists(nsl inet_addr "" HAVE_NSL_LIB)
    if(HAVE_NSL_LIB)
      append_to_list(LINK_LIBS nsl)
    endif(HAVE_NSL_LIB)
    # Check if pthread_create is within the pthread library (if the library exists), and add it to the linker flags if needed
    check_library_exists(pthread pthread_create "" HAVE_PTHREAD)
    if(HAVE_PTHREAD)
      set(LDFLAGS "${LDFLAGS} -pthread")
    else(HAVE_PTHREAD)
      message(FATAL_ERROR "The pthread library is required to build ANT")
    endif(HAVE_PTHREAD)
  endif(NOT WIN32)
endif(NOT MSVC)

# Set the DEBUG_BUILD for sysconf.h
if(CMAKE_BUILD_TYPE STREQUAL "DEBUG" OR CMAKE_BUILD_TYPE STREQUAL "RELWITHDEBINFO")
  set(DEBUG_BUILD TRUE)
  if(GCC_IS_GCC4)
    set(CXXFLAGS "${CXXFLAGS} -g3 -O0 -ggdb")
  endif(GCC_IS_GCC4)
endif(CMAKE_BUILD_TYPE STREQUAL "DEBUG" OR CMAKE_BUILD_TYPE STREQUAL "RELWITHDEBINFO")

check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(mysql/mysql.h HAVE_MYSQL_MYSQL_H)
check_include_file(strings.h HAVE_STRINGS_H)
check_include_file(regex HAVE_REGEX)
check_include_file(thread HAVE_THREAD)
check_include_file(atomic HAVE_ATOMIC)
check_include_file(sys/select.h HAVE_SYS_SELECT_H)
check_include_file(sys/epoll.h HAVE_SYS_EPOLL_H)
check_include_file(setjmp.h HAVE_SETJMP_H)
check_include_file(dlfcn.h HAVE_FCNTL_H)
check_include_file(execinfo.h HAVE_EXECINFO_H)

# Check for the existance of the following functions
check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
check_function_exists(setgrent HAVE_SETGRENT)
check_function_exists(strcasecmp HAVE_STRCASECMP)
check_function_exists(stricmp HAVE_STRICMP)
check_function_exists(strlcat HAVE_STRLCAT)
check_function_exists(strlcpy HAVE_STRLCPY)
check_function_exists(umask HAVE_UMASK)
check_function_exists(backtrace HAVE_BACKTRACE)
check_function_exists(eventfd HAVE_EVENTFD)

check_type_size(uint8_t UINT8_T)
check_type_size(u_int8_t U_INT8_T)
check_type_size(int16_t INT16_T)
check_type_size(uint16_t UINT16_T)
check_type_size(u_int16_t U_INT16_T)
check_type_size(int32_t INT32_T)
check_type_size(uint32_t UINT32_T)
check_type_size(u_int32_t U_INT32_T)

# If CMake has found that the given system requires a special library for dl* calls, include it with the linker flags
if(CMAKE_DL_LIBS)
  append_to_list(LINK_LIBS ${CMAKE_DL_LIBS})
endif(CMAKE_DL_LIBS)

# Search for the following programs
find_program(GREP grep)
find_program(SH sh)
find_program(PERL perl)
find_program(PYTHON NAMES python2 python)
if(NOT PYTHON)
	message(FATAL "Can't build without Python interpreter")
endif(NOT PYTHON)

# Strip the leading and trailing spaces from the compile flags
if(CXXFLAGS)
  strip_string(${CXXFLAGS} CXXFLAGS)
endif(CXXFLAGS)
# Strip the leading and trailing spaces from the linker flags
if(LDFLAGS)
  strip_string(${LDFLAGS} LDFLAGS)
endif(LDFLAGS)

if(PERL AND USE_RUN_CC_PL)
  set(CMAKE_CXX_COMPILE_OBJECT "${PERL} ${ant_SOURCE_DIR}/run-cc.pl -q ${CMAKE_CXX_COMPILE_OBJECT}")
  set(CMAKE_CXX_LINK_EXECUTABLE "${PERL} ${ant_SOURCE_DIR}/run-cc.pl -q ${CMAKE_CXX_LINK_EXECUTABLE}")
  set(CMAKE_CXX_CREATE_SHARED_MODULE "${PERL} ${ant_SOURCE_DIR}/run-cc.pl -q ${CMAKE_CXX_CREATE_SHARED_MODULE}")
endif(PERL AND USE_RUN_CC_PL)

# If a INSTDIR was passed in to CMake, use it as the install prefix, otherwise set the default install prefix to the bots directory under the user's home directory
if(INSTDIR)
  set(CMAKE_INSTALL_PREFIX "${INSTDIR}")
else(INSTDIR)
  set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/ANT")
endif(INSTDIR)

ADD_SUBDIRECTORY(modules)
ADD_SUBDIRECTORY(web)
ADD_SUBDIRECTORY(src)

set(CONFIG_INSTALL bot.example.conf README.md)
install(FILES ${CONFIG_INSTALL}
  DESTINATION bin
)

get_target_property(BOT_BINARY ${PROGRAM_NAME} LOCATION)
get_filename_component(BOT_BINARY ${BOT_BINARY} NAME)
