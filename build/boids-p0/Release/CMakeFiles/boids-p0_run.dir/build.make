# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.13

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
CMAKE_SOURCE_DIR = /home/rondo/Software/sandbox/allolib_playground

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/rondo/Software/sandbox/allolib_playground/HeatWaves/build/boids-p0/Release

# Utility rule file for boids-p0_run.

# Include the progress variables for this target.
include CMakeFiles/boids-p0_run.dir/progress.make

CMakeFiles/boids-p0_run: ../../../bin/boids-p0
	cd /home/rondo/Software/sandbox/allolib_playground/HeatWaves/bin && ./boids-p0

boids-p0_run: CMakeFiles/boids-p0_run
boids-p0_run: CMakeFiles/boids-p0_run.dir/build.make

.PHONY : boids-p0_run

# Rule to build all files generated by this target.
CMakeFiles/boids-p0_run.dir/build: boids-p0_run

.PHONY : CMakeFiles/boids-p0_run.dir/build

CMakeFiles/boids-p0_run.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/boids-p0_run.dir/cmake_clean.cmake
.PHONY : CMakeFiles/boids-p0_run.dir/clean

CMakeFiles/boids-p0_run.dir/depend:
	cd /home/rondo/Software/sandbox/allolib_playground/HeatWaves/build/boids-p0/Release && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/rondo/Software/sandbox/allolib_playground /home/rondo/Software/sandbox/allolib_playground /home/rondo/Software/sandbox/allolib_playground/HeatWaves/build/boids-p0/Release /home/rondo/Software/sandbox/allolib_playground/HeatWaves/build/boids-p0/Release /home/rondo/Software/sandbox/allolib_playground/HeatWaves/build/boids-p0/Release/CMakeFiles/boids-p0_run.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/boids-p0_run.dir/depend

