cmake_minimum_required(VERSION 3.24)
project(clipboardhub C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(SQLite3 REQUIRED)

# --- Force wide-char ncurses (ncursesw) on MSYS2/MinGW ---
# Try manual detection first (more reliable on MSYS2)
find_path(NCURSESW_INCLUDE_DIR
  NAMES ncursesw/curses.h
  PATHS /mingw64/include /usr/include
)
find_library(NCURSESW_LIBRARY
  NAMES ncursesw
  PATHS /mingw64/lib /usr/lib
)
find_library(FORMW_LIBRARY
  NAMES formw
  PATHS /mingw64/lib /usr/lib
)
find_library(PANELW_LIBRARY
  NAMES panelw
  PATHS /mingw64/lib /usr/lib
)

if (NCURSESW_INCLUDE_DIR AND NCURSESW_LIBRARY)
  message(STATUS "Using ncursesw: ${NCURSESW_LIBRARY}")
  set(CURSES_INCLUDE_DIRS "${NCURSESW_INCLUDE_DIR}")
  set(CURSES_LIBRARIES "${NCURSESW_LIBRARY}")
  if (FORMW_LIBRARY)
    list(APPEND CURSES_LIBRARIES "${FORMW_LIBRARY}")
  endif()
  if (PANELW_LIBRARY)
    list(APPEND CURSES_LIBRARIES "${PANELW_LIBRARY}")
  endif()
else()
  # Fallback to CMake’s FindCurses (may pick non-wide lib)
  set(CURSES_NEED_WIDE TRUE)
  find_package(Curses REQUIRED)
  set(CURSES_INCLUDE_DIRS "${CURSES_INCLUDE_DIRS}" "${CURSES_INCLUDE_DIR}")
  set(CURSES_LIBRARIES "${CURSES_LIBRARIES}")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Warnings OPTIONAL)

set(CHUB_SOURCES
    src/main.c
    src/tui.c
    src/db.c
    src/clip.c
    src/transform.c
    src/util.c
    src/platform_win.c
)

add_executable(chub ${CHUB_SOURCES})

target_include_directories(chub PRIVATE
  include
  ${CURSES_INCLUDE_DIRS}
)

target_link_libraries(chub PRIVATE
  ${CURSES_LIBRARIES}
  SQLite::SQLite3
  kernel32 user32 gdi32
)

target_compile_definitions(chub PRIVATE
  CHUB_VERSION=\"0.1.0\"
  _CRT_SECURE_NO_WARNINGS
)

if (COMMAND chub_set_warnings)
  chub_set_warnings(chub)
endif()
