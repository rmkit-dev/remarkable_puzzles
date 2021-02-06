ifndef DEFS_INCLUDED
DEFS_INCLUDED=1

# Normalization rules: use bash, remove builtin rules
SHELL := /usr/bin/env bash
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:


export ARCH ?= arm
export BUILD ?= debug
export BUILD_ROOT ?= build
export BUILD_DIR ?= $(BUILD_ROOT)/$(BUILD)

DOCKER_ENV=-e ARCH -e BUILD -e BUILD_ROOT -e BUILD_DIR

ifeq ($(ARCH),arm)
	CXX = arm-linux-gnueabihf-g++
	CC  = arm-linux-gnueabihf-gcc
	STB = stb.arm.o
else
	CXX = g++
	CC  = gcc
	STB = stb.x86.o
endif

ifeq ($(BUILD),debug)
	BUILD_FLAGS = -g
else ifeq ($(BUILD),prof)
	BUILD_FLAGS = -g -pg -O2 -DNDEBUG
else ifeq ($(BUILD),icons)
	BUILD_FLAGS = -DNDEBUG -DRMP_ICON_APP
else
	# release
	BUILD_FLAGS = -O2 -DNDEBUG
endif

endif # include guard
