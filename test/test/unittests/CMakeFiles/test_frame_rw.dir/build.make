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
CMAKE_SOURCE_DIR = /home/minh/HTTP3_QUIC/LSQUIC/lsquic

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/minh/HTTP3_QUIC/LSQUIC/lsquic/test

# Include any dependencies generated for this target.
include test/unittests/CMakeFiles/test_frame_rw.dir/depend.make

# Include the progress variables for this target.
include test/unittests/CMakeFiles/test_frame_rw.dir/progress.make

# Include the compile flags for this target's objects.
include test/unittests/CMakeFiles/test_frame_rw.dir/flags.make

test/unittests/CMakeFiles/test_frame_rw.dir/test_frame_rw.c.o: test/unittests/CMakeFiles/test_frame_rw.dir/flags.make
test/unittests/CMakeFiles/test_frame_rw.dir/test_frame_rw.c.o: unittests/test_frame_rw.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/minh/HTTP3_QUIC/LSQUIC/lsquic/test/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object test/unittests/CMakeFiles/test_frame_rw.dir/test_frame_rw.c.o"
	cd /home/minh/HTTP3_QUIC/LSQUIC/lsquic/test/test/unittests && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/test_frame_rw.dir/test_frame_rw.c.o   -c /home/minh/HTTP3_QUIC/LSQUIC/lsquic/test/unittests/test_frame_rw.c

test/unittests/CMakeFiles/test_frame_rw.dir/test_frame_rw.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/test_frame_rw.dir/test_frame_rw.c.i"
	cd /home/minh/HTTP3_QUIC/LSQUIC/lsquic/test/test/unittests && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/minh/HTTP3_QUIC/LSQUIC/lsquic/test/unittests/test_frame_rw.c > CMakeFiles/test_frame_rw.dir/test_frame_rw.c.i

test/unittests/CMakeFiles/test_frame_rw.dir/test_frame_rw.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/test_frame_rw.dir/test_frame_rw.c.s"
	cd /home/minh/HTTP3_QUIC/LSQUIC/lsquic/test/test/unittests && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/minh/HTTP3_QUIC/LSQUIC/lsquic/test/unittests/test_frame_rw.c -o CMakeFiles/test_frame_rw.dir/test_frame_rw.c.s

test/unittests/CMakeFiles/test_frame_rw.dir/test_frame_rw.c.o.requires:

.PHONY : test/unittests/CMakeFiles/test_frame_rw.dir/test_frame_rw.c.o.requires

test/unittests/CMakeFiles/test_frame_rw.dir/test_frame_rw.c.o.provides: test/unittests/CMakeFiles/test_frame_rw.dir/test_frame_rw.c.o.requires
	$(MAKE) -f test/unittests/CMakeFiles/test_frame_rw.dir/build.make test/unittests/CMakeFiles/test_frame_rw.dir/test_frame_rw.c.o.provides.build
.PHONY : test/unittests/CMakeFiles/test_frame_rw.dir/test_frame_rw.c.o.provides

test/unittests/CMakeFiles/test_frame_rw.dir/test_frame_rw.c.o.provides.build: test/unittests/CMakeFiles/test_frame_rw.dir/test_frame_rw.c.o


# Object files for target test_frame_rw
test_frame_rw_OBJECTS = \
"CMakeFiles/test_frame_rw.dir/test_frame_rw.c.o"

# External object files for target test_frame_rw
test_frame_rw_EXTERNAL_OBJECTS =

test/unittests/test_frame_rw: test/unittests/CMakeFiles/test_frame_rw.dir/test_frame_rw.c.o
test/unittests/test_frame_rw: test/unittests/CMakeFiles/test_frame_rw.dir/build.make
test/unittests/test_frame_rw: src/liblsquic/liblsquic.a
test/unittests/test_frame_rw: /usr/lib/x86_64-linux-gnu/libevent.a
test/unittests/test_frame_rw: /home/minh/HTTP3_QUIC/LSQUIC/boringssl/ssl/libssl.a
test/unittests/test_frame_rw: /home/minh/HTTP3_QUIC/LSQUIC/boringssl/crypto/libcrypto.a
test/unittests/test_frame_rw: /usr/lib/x86_64-linux-gnu/libz.a
test/unittests/test_frame_rw: test/unittests/CMakeFiles/test_frame_rw.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/minh/HTTP3_QUIC/LSQUIC/lsquic/test/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable test_frame_rw"
	cd /home/minh/HTTP3_QUIC/LSQUIC/lsquic/test/test/unittests && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test_frame_rw.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
test/unittests/CMakeFiles/test_frame_rw.dir/build: test/unittests/test_frame_rw

.PHONY : test/unittests/CMakeFiles/test_frame_rw.dir/build

test/unittests/CMakeFiles/test_frame_rw.dir/requires: test/unittests/CMakeFiles/test_frame_rw.dir/test_frame_rw.c.o.requires

.PHONY : test/unittests/CMakeFiles/test_frame_rw.dir/requires

test/unittests/CMakeFiles/test_frame_rw.dir/clean:
	cd /home/minh/HTTP3_QUIC/LSQUIC/lsquic/test/test/unittests && $(CMAKE_COMMAND) -P CMakeFiles/test_frame_rw.dir/cmake_clean.cmake
.PHONY : test/unittests/CMakeFiles/test_frame_rw.dir/clean

test/unittests/CMakeFiles/test_frame_rw.dir/depend:
	cd /home/minh/HTTP3_QUIC/LSQUIC/lsquic/test && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/minh/HTTP3_QUIC/LSQUIC/lsquic /home/minh/HTTP3_QUIC/LSQUIC/lsquic/test/unittests /home/minh/HTTP3_QUIC/LSQUIC/lsquic/test /home/minh/HTTP3_QUIC/LSQUIC/lsquic/test/test/unittests /home/minh/HTTP3_QUIC/LSQUIC/lsquic/test/test/unittests/CMakeFiles/test_frame_rw.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : test/unittests/CMakeFiles/test_frame_rw.dir/depend

