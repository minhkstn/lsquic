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
CMAKE_SOURCE_DIR = /home/minh/HTTP3_QUIC/LSQUIC/lsquic_minh_kstn_branch

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/minh/HTTP3_QUIC/LSQUIC/lsquic_minh_kstn_branch

# Include any dependencies generated for this target.
include tests/CMakeFiles/test_ack.dir/depend.make

# Include the progress variables for this target.
include tests/CMakeFiles/test_ack.dir/progress.make

# Include the compile flags for this target's objects.
include tests/CMakeFiles/test_ack.dir/flags.make

tests/CMakeFiles/test_ack.dir/test_ack.c.o: tests/CMakeFiles/test_ack.dir/flags.make
tests/CMakeFiles/test_ack.dir/test_ack.c.o: tests/test_ack.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/minh/HTTP3_QUIC/LSQUIC/lsquic_minh_kstn_branch/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object tests/CMakeFiles/test_ack.dir/test_ack.c.o"
	cd /home/minh/HTTP3_QUIC/LSQUIC/lsquic_minh_kstn_branch/tests && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/test_ack.dir/test_ack.c.o   -c /home/minh/HTTP3_QUIC/LSQUIC/lsquic_minh_kstn_branch/tests/test_ack.c

tests/CMakeFiles/test_ack.dir/test_ack.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/test_ack.dir/test_ack.c.i"
	cd /home/minh/HTTP3_QUIC/LSQUIC/lsquic_minh_kstn_branch/tests && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/minh/HTTP3_QUIC/LSQUIC/lsquic_minh_kstn_branch/tests/test_ack.c > CMakeFiles/test_ack.dir/test_ack.c.i

tests/CMakeFiles/test_ack.dir/test_ack.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/test_ack.dir/test_ack.c.s"
	cd /home/minh/HTTP3_QUIC/LSQUIC/lsquic_minh_kstn_branch/tests && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/minh/HTTP3_QUIC/LSQUIC/lsquic_minh_kstn_branch/tests/test_ack.c -o CMakeFiles/test_ack.dir/test_ack.c.s

tests/CMakeFiles/test_ack.dir/test_ack.c.o.requires:

.PHONY : tests/CMakeFiles/test_ack.dir/test_ack.c.o.requires

tests/CMakeFiles/test_ack.dir/test_ack.c.o.provides: tests/CMakeFiles/test_ack.dir/test_ack.c.o.requires
	$(MAKE) -f tests/CMakeFiles/test_ack.dir/build.make tests/CMakeFiles/test_ack.dir/test_ack.c.o.provides.build
.PHONY : tests/CMakeFiles/test_ack.dir/test_ack.c.o.provides

tests/CMakeFiles/test_ack.dir/test_ack.c.o.provides.build: tests/CMakeFiles/test_ack.dir/test_ack.c.o


# Object files for target test_ack
test_ack_OBJECTS = \
"CMakeFiles/test_ack.dir/test_ack.c.o"

# External object files for target test_ack
test_ack_EXTERNAL_OBJECTS =

tests/test_ack: tests/CMakeFiles/test_ack.dir/test_ack.c.o
tests/test_ack: tests/CMakeFiles/test_ack.dir/build.make
tests/test_ack: src/liblsquic/liblsquic.a
tests/test_ack: /usr/lib/x86_64-linux-gnu/libevent.a
tests/test_ack: /home/minh/HTTP3_QUIC/LSQUIC/boringssl/ssl/libssl.a
tests/test_ack: /home/minh/HTTP3_QUIC/LSQUIC/boringssl/crypto/libcrypto.a
tests/test_ack: /usr/lib/x86_64-linux-gnu/libz.a
tests/test_ack: tests/CMakeFiles/test_ack.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/minh/HTTP3_QUIC/LSQUIC/lsquic_minh_kstn_branch/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable test_ack"
	cd /home/minh/HTTP3_QUIC/LSQUIC/lsquic_minh_kstn_branch/tests && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test_ack.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
tests/CMakeFiles/test_ack.dir/build: tests/test_ack

.PHONY : tests/CMakeFiles/test_ack.dir/build

tests/CMakeFiles/test_ack.dir/requires: tests/CMakeFiles/test_ack.dir/test_ack.c.o.requires

.PHONY : tests/CMakeFiles/test_ack.dir/requires

tests/CMakeFiles/test_ack.dir/clean:
	cd /home/minh/HTTP3_QUIC/LSQUIC/lsquic_minh_kstn_branch/tests && $(CMAKE_COMMAND) -P CMakeFiles/test_ack.dir/cmake_clean.cmake
.PHONY : tests/CMakeFiles/test_ack.dir/clean

tests/CMakeFiles/test_ack.dir/depend:
	cd /home/minh/HTTP3_QUIC/LSQUIC/lsquic_minh_kstn_branch && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/minh/HTTP3_QUIC/LSQUIC/lsquic_minh_kstn_branch /home/minh/HTTP3_QUIC/LSQUIC/lsquic_minh_kstn_branch/tests /home/minh/HTTP3_QUIC/LSQUIC/lsquic_minh_kstn_branch /home/minh/HTTP3_QUIC/LSQUIC/lsquic_minh_kstn_branch/tests /home/minh/HTTP3_QUIC/LSQUIC/lsquic_minh_kstn_branch/tests/CMakeFiles/test_ack.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : tests/CMakeFiles/test_ack.dir/depend

