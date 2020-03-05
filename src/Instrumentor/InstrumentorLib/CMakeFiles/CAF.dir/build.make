# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/zys/caf_refactory

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/zys/caf_refactory

# Include any dependencies generated for this target.
include src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/depend.make

# Include the progress variables for this target.
include src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/progress.make

# Include the compile flags for this target's objects.
include src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/flags.make

src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/caflib.cpp.o: src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/flags.make
src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/caflib.cpp.o: src/Instrumentor/InstrumentorLib/caflib.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/zys/caf_refactory/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/caflib.cpp.o"
	cd /home/zys/caf_refactory/src/Instrumentor/InstrumentorLib && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/CAF.dir/caflib.cpp.o -c /home/zys/caf_refactory/src/Instrumentor/InstrumentorLib/caflib.cpp

src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/caflib.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/CAF.dir/caflib.cpp.i"
	cd /home/zys/caf_refactory/src/Instrumentor/InstrumentorLib && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/zys/caf_refactory/src/Instrumentor/InstrumentorLib/caflib.cpp > CMakeFiles/CAF.dir/caflib.cpp.i

src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/caflib.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/CAF.dir/caflib.cpp.s"
	cd /home/zys/caf_refactory/src/Instrumentor/InstrumentorLib && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/zys/caf_refactory/src/Instrumentor/InstrumentorLib/caflib.cpp -o CMakeFiles/CAF.dir/caflib.cpp.s

src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/caflib.cpp.o.requires:

.PHONY : src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/caflib.cpp.o.requires

src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/caflib.cpp.o.provides: src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/caflib.cpp.o.requires
	$(MAKE) -f src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/build.make src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/caflib.cpp.o.provides.build
.PHONY : src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/caflib.cpp.o.provides

src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/caflib.cpp.o.provides.build: src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/caflib.cpp.o


# Object files for target CAF
CAF_OBJECTS = \
"CMakeFiles/CAF.dir/caflib.cpp.o"

# External object files for target CAF
CAF_EXTERNAL_OBJECTS =

lib/libCAF.a: src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/caflib.cpp.o
lib/libCAF.a: src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/build.make
lib/libCAF.a: src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/zys/caf_refactory/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library ../../../lib/libCAF.a"
	cd /home/zys/caf_refactory/src/Instrumentor/InstrumentorLib && $(CMAKE_COMMAND) -P CMakeFiles/CAF.dir/cmake_clean_target.cmake
	cd /home/zys/caf_refactory/src/Instrumentor/InstrumentorLib && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/CAF.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/build: lib/libCAF.a

.PHONY : src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/build

src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/requires: src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/caflib.cpp.o.requires

.PHONY : src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/requires

src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/clean:
	cd /home/zys/caf_refactory/src/Instrumentor/InstrumentorLib && $(CMAKE_COMMAND) -P CMakeFiles/CAF.dir/cmake_clean.cmake
.PHONY : src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/clean

src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/depend:
	cd /home/zys/caf_refactory && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/zys/caf_refactory /home/zys/caf_refactory/src/Instrumentor/InstrumentorLib /home/zys/caf_refactory /home/zys/caf_refactory/src/Instrumentor/InstrumentorLib /home/zys/caf_refactory/src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/Instrumentor/InstrumentorLib/CMakeFiles/CAF.dir/depend

