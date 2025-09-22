# ClipboardHub

ClipboardHub is a cross-platform clipboard manager with a terminal user interface (TUI). It stores your clipboard history, lets you search, favorite, transform, and re-use past entries quickly.

## Prerequisites

Make sure you have the following installed:

* **CMake** (≥ 3.16)
* **Ninja** (or `make`)
* **GCC/Clang** or **MSVC** (C17 support required)
* **ncursesw** (wide character curses library)
* **SQLite3**
* **Git**

On Windows (MSYS2/MinGW64):

```bash
pacman -S --needed base-devel mingw-w64-x86_64-toolchain \
                 mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja \
                 mingw-w64-x86_64-ncurses mingw-w64-x86_64-sqlite3
```

On Ubuntu/Debian:

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build libncursesw5-dev libsqlite3-dev git
```

## Building

Clone the repository:

```bash
git clone https://github.com/yourname/clipboardhub.git
cd clipboardhub
```

Create a release build:

```bash
cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
```

The compiled binary will be in:

```
build/release/chub
```

## Running

Run from the project root:

```bash
./build/release/chub
```

## Functionality

ClipboardHub provides a text-based UI with three panes:

* **List Pane** (left): shows your clipboard history.

  * Each entry displays an ID and whether it is favorited.
  * Navigate with arrow keys or PageUp/PageDown.
* **Preview Pane** (right): shows full text of the selected entry, with wrapping.
* **Status Line** (bottom): displays search/filter state and available actions.

### Controls

* **Arrow Up/Down**: Move selection
* **PageUp/PageDown**: Scroll faster
* **Enter**: Copy selected entry back into the system clipboard
* **f**: Toggle favorite
* **d**: Delete entry
* **t**: Apply a transform (Trim, ToggleCase, URL-Decode)
* **/**: Search/filter clipboard entries
* **q**: Quit

### Features

1. **Clipboard History**
   Captures and stores your clipboard entries in a local SQLite database.

2. **Search**
   Quickly filter entries using `/` and typing a search term.

3. **Favorites**
   Mark frequently used entries and access them easily.

4. **Transforms**
   Modify text before copying:

   * Trim whitespace
   * Toggle case
   * URL decode

5. **Persistence**
   Entries are stored persistently in the SQLite database.

6. **Cross-Platform**
   Works on Linux and Windows (MSYS2/MinGW).


