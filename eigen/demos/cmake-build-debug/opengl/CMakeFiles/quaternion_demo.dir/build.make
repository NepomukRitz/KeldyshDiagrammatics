# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.14

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
CMAKE_COMMAND = /home/s/Sa.Aguirre/clion-2019.1.2/bin/cmake/linux/bin/cmake

# The command to remove a file.
RM = /home/s/Sa.Aguirre/clion-2019.1.2/bin/cmake/linux/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug

# Include any dependencies generated for this target.
include opengl/CMakeFiles/quaternion_demo.dir/depend.make

# Include the progress variables for this target.
include opengl/CMakeFiles/quaternion_demo.dir/progress.make

# Include the compile flags for this target's objects.
include opengl/CMakeFiles/quaternion_demo.dir/flags.make

opengl/quaternion_demo.moc: ../opengl/quaternion_demo.h
opengl/quaternion_demo.moc: opengl/quaternion_demo.moc_parameters
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Generating quaternion_demo.moc"
	cd /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/opengl && /usr/lib/x86_64-linux-gnu/qt4/bin/moc @/home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/opengl/quaternion_demo.moc_parameters

opengl/CMakeFiles/quaternion_demo.dir/gpuhelper.o: opengl/CMakeFiles/quaternion_demo.dir/flags.make
opengl/CMakeFiles/quaternion_demo.dir/gpuhelper.o: ../opengl/gpuhelper.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object opengl/CMakeFiles/quaternion_demo.dir/gpuhelper.o"
	cd /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/opengl && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/quaternion_demo.dir/gpuhelper.o -c /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/opengl/gpuhelper.cpp

opengl/CMakeFiles/quaternion_demo.dir/gpuhelper.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/quaternion_demo.dir/gpuhelper.i"
	cd /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/opengl && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/opengl/gpuhelper.cpp > CMakeFiles/quaternion_demo.dir/gpuhelper.i

opengl/CMakeFiles/quaternion_demo.dir/gpuhelper.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/quaternion_demo.dir/gpuhelper.s"
	cd /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/opengl && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/opengl/gpuhelper.cpp -o CMakeFiles/quaternion_demo.dir/gpuhelper.s

opengl/CMakeFiles/quaternion_demo.dir/icosphere.o: opengl/CMakeFiles/quaternion_demo.dir/flags.make
opengl/CMakeFiles/quaternion_demo.dir/icosphere.o: ../opengl/icosphere.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object opengl/CMakeFiles/quaternion_demo.dir/icosphere.o"
	cd /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/opengl && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/quaternion_demo.dir/icosphere.o -c /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/opengl/icosphere.cpp

opengl/CMakeFiles/quaternion_demo.dir/icosphere.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/quaternion_demo.dir/icosphere.i"
	cd /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/opengl && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/opengl/icosphere.cpp > CMakeFiles/quaternion_demo.dir/icosphere.i

opengl/CMakeFiles/quaternion_demo.dir/icosphere.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/quaternion_demo.dir/icosphere.s"
	cd /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/opengl && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/opengl/icosphere.cpp -o CMakeFiles/quaternion_demo.dir/icosphere.s

opengl/CMakeFiles/quaternion_demo.dir/camera.o: opengl/CMakeFiles/quaternion_demo.dir/flags.make
opengl/CMakeFiles/quaternion_demo.dir/camera.o: ../opengl/camera.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object opengl/CMakeFiles/quaternion_demo.dir/camera.o"
	cd /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/opengl && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/quaternion_demo.dir/camera.o -c /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/opengl/camera.cpp

opengl/CMakeFiles/quaternion_demo.dir/camera.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/quaternion_demo.dir/camera.i"
	cd /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/opengl && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/opengl/camera.cpp > CMakeFiles/quaternion_demo.dir/camera.i

opengl/CMakeFiles/quaternion_demo.dir/camera.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/quaternion_demo.dir/camera.s"
	cd /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/opengl && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/opengl/camera.cpp -o CMakeFiles/quaternion_demo.dir/camera.s

opengl/CMakeFiles/quaternion_demo.dir/trackball.o: opengl/CMakeFiles/quaternion_demo.dir/flags.make
opengl/CMakeFiles/quaternion_demo.dir/trackball.o: ../opengl/trackball.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object opengl/CMakeFiles/quaternion_demo.dir/trackball.o"
	cd /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/opengl && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/quaternion_demo.dir/trackball.o -c /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/opengl/trackball.cpp

opengl/CMakeFiles/quaternion_demo.dir/trackball.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/quaternion_demo.dir/trackball.i"
	cd /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/opengl && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/opengl/trackball.cpp > CMakeFiles/quaternion_demo.dir/trackball.i

opengl/CMakeFiles/quaternion_demo.dir/trackball.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/quaternion_demo.dir/trackball.s"
	cd /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/opengl && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/opengl/trackball.cpp -o CMakeFiles/quaternion_demo.dir/trackball.s

opengl/CMakeFiles/quaternion_demo.dir/quaternion_demo.o: opengl/CMakeFiles/quaternion_demo.dir/flags.make
opengl/CMakeFiles/quaternion_demo.dir/quaternion_demo.o: ../opengl/quaternion_demo.cpp
opengl/CMakeFiles/quaternion_demo.dir/quaternion_demo.o: opengl/quaternion_demo.moc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building CXX object opengl/CMakeFiles/quaternion_demo.dir/quaternion_demo.o"
	cd /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/opengl && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/quaternion_demo.dir/quaternion_demo.o -c /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/opengl/quaternion_demo.cpp

opengl/CMakeFiles/quaternion_demo.dir/quaternion_demo.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/quaternion_demo.dir/quaternion_demo.i"
	cd /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/opengl && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/opengl/quaternion_demo.cpp > CMakeFiles/quaternion_demo.dir/quaternion_demo.i

opengl/CMakeFiles/quaternion_demo.dir/quaternion_demo.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/quaternion_demo.dir/quaternion_demo.s"
	cd /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/opengl && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/opengl/quaternion_demo.cpp -o CMakeFiles/quaternion_demo.dir/quaternion_demo.s

# Object files for target quaternion_demo
quaternion_demo_OBJECTS = \
"CMakeFiles/quaternion_demo.dir/gpuhelper.o" \
"CMakeFiles/quaternion_demo.dir/icosphere.o" \
"CMakeFiles/quaternion_demo.dir/camera.o" \
"CMakeFiles/quaternion_demo.dir/trackball.o" \
"CMakeFiles/quaternion_demo.dir/quaternion_demo.o"

# External object files for target quaternion_demo
quaternion_demo_EXTERNAL_OBJECTS =

opengl/quaternion_demo: opengl/CMakeFiles/quaternion_demo.dir/gpuhelper.o
opengl/quaternion_demo: opengl/CMakeFiles/quaternion_demo.dir/icosphere.o
opengl/quaternion_demo: opengl/CMakeFiles/quaternion_demo.dir/camera.o
opengl/quaternion_demo: opengl/CMakeFiles/quaternion_demo.dir/trackball.o
opengl/quaternion_demo: opengl/CMakeFiles/quaternion_demo.dir/quaternion_demo.o
opengl/quaternion_demo: opengl/CMakeFiles/quaternion_demo.dir/build.make
opengl/quaternion_demo: /usr/lib/x86_64-linux-gnu/libQtCore.so
opengl/quaternion_demo: /usr/lib/x86_64-linux-gnu/libQtGui.so
opengl/quaternion_demo: /usr/lib/x86_64-linux-gnu/libQtOpenGL.so
opengl/quaternion_demo: /usr/lib/x86_64-linux-gnu/libGL.so
opengl/quaternion_demo: /usr/lib/x86_64-linux-gnu/libGLU.so
opengl/quaternion_demo: opengl/CMakeFiles/quaternion_demo.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Linking CXX executable quaternion_demo"
	cd /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/opengl && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/quaternion_demo.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
opengl/CMakeFiles/quaternion_demo.dir/build: opengl/quaternion_demo

.PHONY : opengl/CMakeFiles/quaternion_demo.dir/build

opengl/CMakeFiles/quaternion_demo.dir/clean:
	cd /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/opengl && $(CMAKE_COMMAND) -P CMakeFiles/quaternion_demo.dir/cmake_clean.cmake
.PHONY : opengl/CMakeFiles/quaternion_demo.dir/clean

opengl/CMakeFiles/quaternion_demo.dir/depend: opengl/quaternion_demo.moc
	cd /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/opengl /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/opengl /home/s/Sa.Aguirre/Downloads/Thesis/mfrg/eigen/demos/cmake-build-debug/opengl/CMakeFiles/quaternion_demo.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : opengl/CMakeFiles/quaternion_demo.dir/depend

