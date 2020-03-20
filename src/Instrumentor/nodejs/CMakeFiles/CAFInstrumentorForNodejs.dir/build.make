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
include src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/depend.make

# Include the progress variables for this target.
include src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/progress.make

# Include the compile flags for this target's objects.
include src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/flags.make

src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/CAFCodeGenerator.cpp.o: src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/flags.make
src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/CAFCodeGenerator.cpp.o: src/Instrumentor/nodejs/CAFCodeGenerator.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/zys/caf_refactory/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/CAFCodeGenerator.cpp.o"
	cd /home/zys/caf_refactory/src/Instrumentor/nodejs && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/CAFInstrumentorForNodejs.dir/CAFCodeGenerator.cpp.o -c /home/zys/caf_refactory/src/Instrumentor/nodejs/CAFCodeGenerator.cpp

src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/CAFCodeGenerator.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/CAFInstrumentorForNodejs.dir/CAFCodeGenerator.cpp.i"
	cd /home/zys/caf_refactory/src/Instrumentor/nodejs && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/zys/caf_refactory/src/Instrumentor/nodejs/CAFCodeGenerator.cpp > CMakeFiles/CAFInstrumentorForNodejs.dir/CAFCodeGenerator.cpp.i

src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/CAFCodeGenerator.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/CAFInstrumentorForNodejs.dir/CAFCodeGenerator.cpp.s"
	cd /home/zys/caf_refactory/src/Instrumentor/nodejs && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/zys/caf_refactory/src/Instrumentor/nodejs/CAFCodeGenerator.cpp -o CMakeFiles/CAFInstrumentorForNodejs.dir/CAFCodeGenerator.cpp.s

src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/CAFCodeGenerator.cpp.o.requires:

.PHONY : src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/CAFCodeGenerator.cpp.o.requires

src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/CAFCodeGenerator.cpp.o.provides: src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/CAFCodeGenerator.cpp.o.requires
	$(MAKE) -f src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/build.make src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/CAFCodeGenerator.cpp.o.provides.build
.PHONY : src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/CAFCodeGenerator.cpp.o.provides

src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/CAFCodeGenerator.cpp.o.provides.build: src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/CAFCodeGenerator.cpp.o


# Object files for target CAFInstrumentorForNodejs
CAFInstrumentorForNodejs_OBJECTS = \
"CMakeFiles/CAFInstrumentorForNodejs.dir/CAFCodeGenerator.cpp.o"

# External object files for target CAFInstrumentorForNodejs
CAFInstrumentorForNodejs_EXTERNAL_OBJECTS =

lib/libCAFInstrumentorForNodejs.a: src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/CAFCodeGenerator.cpp.o
lib/libCAFInstrumentorForNodejs.a: src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/build.make
lib/libCAFInstrumentorForNodejs.a: src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/zys/caf_refactory/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library ../../../lib/libCAFInstrumentorForNodejs.a"
	cd /home/zys/caf_refactory/src/Instrumentor/nodejs && $(CMAKE_COMMAND) -P CMakeFiles/CAFInstrumentorForNodejs.dir/cmake_clean_target.cmake
	cd /home/zys/caf_refactory/src/Instrumentor/nodejs && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/CAFInstrumentorForNodejs.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/build: lib/libCAFInstrumentorForNodejs.a

.PHONY : src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/build

src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/requires: src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/CAFCodeGenerator.cpp.o.requires

.PHONY : src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/requires

src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/clean:
	cd /home/zys/caf_refactory/src/Instrumentor/nodejs && $(CMAKE_COMMAND) -P CMakeFiles/CAFInstrumentorForNodejs.dir/cmake_clean.cmake
.PHONY : src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/clean

src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/depend:
	cd /home/zys/caf_refactory && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/zys/caf_refactory /home/zys/caf_refactory/src/Instrumentor/nodejs /home/zys/caf_refactory /home/zys/caf_refactory/src/Instrumentor/nodejs /home/zys/caf_refactory/src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/Instrumentor/nodejs/CMakeFiles/CAFInstrumentorForNodejs.dir/depend

