# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

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
CMAKE_SOURCE_DIR = /home/bill/degradation/src/frontend

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/bill/degradation/src/frontend/build

# Include any dependencies generated for this target.
include patch/CMakeFiles/apply-patch.dir/depend.make

# Include the progress variables for this target.
include patch/CMakeFiles/apply-patch.dir/progress.make

# Include the compile flags for this target's objects.
include patch/CMakeFiles/apply-patch.dir/flags.make

patch/CMakeFiles/apply-patch.dir/ApplyPatch.cpp.o: patch/CMakeFiles/apply-patch.dir/flags.make
patch/CMakeFiles/apply-patch.dir/ApplyPatch.cpp.o: ../patch/ApplyPatch.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/bill/degradation/src/frontend/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object patch/CMakeFiles/apply-patch.dir/ApplyPatch.cpp.o"
	cd /home/bill/degradation/src/frontend/build/patch && /usr/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/apply-patch.dir/ApplyPatch.cpp.o -c /home/bill/degradation/src/frontend/patch/ApplyPatch.cpp

patch/CMakeFiles/apply-patch.dir/ApplyPatch.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/apply-patch.dir/ApplyPatch.cpp.i"
	cd /home/bill/degradation/src/frontend/build/patch && /usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bill/degradation/src/frontend/patch/ApplyPatch.cpp > CMakeFiles/apply-patch.dir/ApplyPatch.cpp.i

patch/CMakeFiles/apply-patch.dir/ApplyPatch.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/apply-patch.dir/ApplyPatch.cpp.s"
	cd /home/bill/degradation/src/frontend/build/patch && /usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bill/degradation/src/frontend/patch/ApplyPatch.cpp -o CMakeFiles/apply-patch.dir/ApplyPatch.cpp.s

# Object files for target apply-patch
apply__patch_OBJECTS = \
"CMakeFiles/apply-patch.dir/ApplyPatch.cpp.o"

# External object files for target apply-patch
apply__patch_EXTERNAL_OBJECTS =

patch/apply-patch: patch/CMakeFiles/apply-patch.dir/ApplyPatch.cpp.o
patch/apply-patch: patch/CMakeFiles/apply-patch.dir/build.make
patch/apply-patch: patch/CMakeFiles/apply-patch.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/bill/degradation/src/frontend/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable apply-patch"
	cd /home/bill/degradation/src/frontend/build/patch && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/apply-patch.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
patch/CMakeFiles/apply-patch.dir/build: patch/apply-patch

.PHONY : patch/CMakeFiles/apply-patch.dir/build

patch/CMakeFiles/apply-patch.dir/clean:
	cd /home/bill/degradation/src/frontend/build/patch && $(CMAKE_COMMAND) -P CMakeFiles/apply-patch.dir/cmake_clean.cmake
.PHONY : patch/CMakeFiles/apply-patch.dir/clean

patch/CMakeFiles/apply-patch.dir/depend:
	cd /home/bill/degradation/src/frontend/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/bill/degradation/src/frontend /home/bill/degradation/src/frontend/patch /home/bill/degradation/src/frontend/build /home/bill/degradation/src/frontend/build/patch /home/bill/degradation/src/frontend/build/patch/CMakeFiles/apply-patch.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : patch/CMakeFiles/apply-patch.dir/depend

