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
include src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/depend.make

# Include the progress variables for this target.
include src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/progress.make

# Include the compile flags for this target's objects.
include src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/flags.make

src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/CAFCodeGenerator.cpp.o: src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/flags.make
src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/CAFCodeGenerator.cpp.o: src/Instrumentor/CAFCodeGenerator.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/zys/caf_refactory/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/CAFCodeGenerator.cpp.o"
	cd /home/zys/caf_refactory/src/Instrumentor && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/CAFInstrumentor.dir/CAFCodeGenerator.cpp.o -c /home/zys/caf_refactory/src/Instrumentor/CAFCodeGenerator.cpp

src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/CAFCodeGenerator.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/CAFInstrumentor.dir/CAFCodeGenerator.cpp.i"
	cd /home/zys/caf_refactory/src/Instrumentor && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/zys/caf_refactory/src/Instrumentor/CAFCodeGenerator.cpp > CMakeFiles/CAFInstrumentor.dir/CAFCodeGenerator.cpp.i

src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/CAFCodeGenerator.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/CAFInstrumentor.dir/CAFCodeGenerator.cpp.s"
	cd /home/zys/caf_refactory/src/Instrumentor && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/zys/caf_refactory/src/Instrumentor/CAFCodeGenerator.cpp -o CMakeFiles/CAFInstrumentor.dir/CAFCodeGenerator.cpp.s

src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/CAFCodeGenerator.cpp.o.requires:

.PHONY : src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/CAFCodeGenerator.cpp.o.requires

src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/CAFCodeGenerator.cpp.o.provides: src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/CAFCodeGenerator.cpp.o.requires
	$(MAKE) -f src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/build.make src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/CAFCodeGenerator.cpp.o.provides.build
.PHONY : src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/CAFCodeGenerator.cpp.o.provides

src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/CAFCodeGenerator.cpp.o.provides.build: src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/CAFCodeGenerator.cpp.o


src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/InstrumentorPass.cpp.o: src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/flags.make
src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/InstrumentorPass.cpp.o: src/Instrumentor/InstrumentorPass.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/zys/caf_refactory/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/InstrumentorPass.cpp.o"
	cd /home/zys/caf_refactory/src/Instrumentor && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/CAFInstrumentor.dir/InstrumentorPass.cpp.o -c /home/zys/caf_refactory/src/Instrumentor/InstrumentorPass.cpp

src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/InstrumentorPass.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/CAFInstrumentor.dir/InstrumentorPass.cpp.i"
	cd /home/zys/caf_refactory/src/Instrumentor && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/zys/caf_refactory/src/Instrumentor/InstrumentorPass.cpp > CMakeFiles/CAFInstrumentor.dir/InstrumentorPass.cpp.i

src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/InstrumentorPass.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/CAFInstrumentor.dir/InstrumentorPass.cpp.s"
	cd /home/zys/caf_refactory/src/Instrumentor && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/zys/caf_refactory/src/Instrumentor/InstrumentorPass.cpp -o CMakeFiles/CAFInstrumentor.dir/InstrumentorPass.cpp.s

src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/InstrumentorPass.cpp.o.requires:

.PHONY : src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/InstrumentorPass.cpp.o.requires

src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/InstrumentorPass.cpp.o.provides: src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/InstrumentorPass.cpp.o.requires
	$(MAKE) -f src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/build.make src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/InstrumentorPass.cpp.o.provides.build
.PHONY : src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/InstrumentorPass.cpp.o.provides

src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/InstrumentorPass.cpp.o.provides.build: src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/InstrumentorPass.cpp.o


# Object files for target CAFInstrumentor
CAFInstrumentor_OBJECTS = \
"CMakeFiles/CAFInstrumentor.dir/CAFCodeGenerator.cpp.o" \
"CMakeFiles/CAFInstrumentor.dir/InstrumentorPass.cpp.o"

# External object files for target CAFInstrumentor
CAFInstrumentor_EXTERNAL_OBJECTS =

lib/libCAFInstrumentor.so: src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/CAFCodeGenerator.cpp.o
lib/libCAFInstrumentor.so: src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/InstrumentorPass.cpp.o
lib/libCAFInstrumentor.so: src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/build.make
lib/libCAFInstrumentor.so: lib/libCAFBasicLLVM.a
lib/libCAFInstrumentor.so: lib/libCAFExtractorStatic.a
lib/libCAFInstrumentor.so: lib/libCAFBasicLLVM.a
lib/libCAFInstrumentor.so: src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/zys/caf_refactory/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX shared module ../../lib/libCAFInstrumentor.so"
	cd /home/zys/caf_refactory/src/Instrumentor && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/CAFInstrumentor.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/build: lib/libCAFInstrumentor.so

.PHONY : src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/build

src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/requires: src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/CAFCodeGenerator.cpp.o.requires
src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/requires: src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/InstrumentorPass.cpp.o.requires

.PHONY : src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/requires

src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/clean:
	cd /home/zys/caf_refactory/src/Instrumentor && $(CMAKE_COMMAND) -P CMakeFiles/CAFInstrumentor.dir/cmake_clean.cmake
.PHONY : src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/clean

src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/depend:
	cd /home/zys/caf_refactory && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/zys/caf_refactory /home/zys/caf_refactory/src/Instrumentor /home/zys/caf_refactory /home/zys/caf_refactory/src/Instrumentor /home/zys/caf_refactory/src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/Instrumentor/CMakeFiles/CAFInstrumentor.dir/depend
