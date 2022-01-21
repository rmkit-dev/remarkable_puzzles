ifndef DEFS_INCLUDED
DEFS_INCLUDED=1

# Normalization rules: use bash, remove builtin rules
SHELL := /usr/bin/env bash
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:


export ARCH ?= rm
export BUILD ?= debug
export BUILD_ROOT ?= build
export BUILD_DIR ?= $(BUILD_ROOT)/$(BUILD)
export RMP_COMPILE_DATE ?= $(shell date +%Y-%m-%d)
export RMP_VERSION ?= $(shell git tag --sort v:refname \
	| tail -n1 \
	| perl -pe 's/(\d+)$$/($$1 + 1)."-SNAPSHOT"/e')


DOCKER_ENV=-e ARCH -e BUILD -e BUILD_ROOT -e BUILD_DIR -e RMP_COMPILE_DATE -e RMP_VERSION

ifeq ($(ARCH),rm)
	CXX = arm-linux-gnueabihf-g++
	CC  = arm-linux-gnueabihf-gcc
	STB = stb.arm.o
else ifeq ($(ARCH),kobo)
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
else ifeq ($(BUILD),resim)
	BUILD_FLAGS = -g -UREMARKABLE -DDEV -DRESIM
else
	# release
	BUILD_FLAGS = -O2 -DNDEBUG
endif

endif # include guard
