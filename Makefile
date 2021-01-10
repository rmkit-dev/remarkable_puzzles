include defs.mk

.PHONY: default all
default: docker
all: docker_build release

.PHONY: release
release: BUILD=release
release: default

.PHONY: debug
debug: BUILD=debug
debug: default



#### rmkit
RMKIT_FLAGS = -D"REMARKABLE=1" \
	-DSTB_IMAGE_IMPLEMENTATION \
	-DSTB_IMAGE_RESIZE_IMPLEMENTATION \
	-DSTB_IMAGE_WRITE_IMPLEMENTATION \
	-DSTB_TRUETYPE_IMPLEMENTATION \
	-pthread -lpthread

$(BUILD_DIR)/rmkit.h:
	cd vendor/rmkit && $(MAKE) rmkit.h
	mkdir -p $(dir $@)
	mv vendor/rmkit/src/build/rmkit.h $(BUILD_DIR)



#### Game files -- defines GAME_OBJS
include Makefile.games



#### Remarkable app
TARGET = puzzles
SRCS   = $(wildcard src/*.cpp)
OBJS   = $(addprefix $(BUILD_DIR)/, $(SRCS:.cpp=.o))

INCLUDES  = -I./ -Isrc/
INCLUDES += -I$(BUILD_DIR)/ # for rmkit

CXXFLAGS  = -Wall $(INCLUDES) $(RMKIT_FLAGS) $(BUILD_FLAGS)
CXXFLAGS += -fdata-sections -ffunction-sections -Wl,--gc-sections

# auto-generate deps
CXXFLAGS += -MMD -MP
DEPS=$(OBJS:.o=.d)
-include $(DEPS)

$(OBJS): $(BUILD_DIR)/%.o : %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD_DIR)/$(TARGET): $(BUILD_DIR)/rmkit.h $(GAME_OBJS) $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(GAME_OBJS) $(OBJS)

.PHONY: $(TARGET)
$(TARGET): $(BUILD_DIR)/$(TARGET)



#### Docker version of the toolchain
DOCKER_TAG = remarkable_puzzles_toolchain:latest

DOCKER_MAKEFLAGS=$(if $(shell echo $(firstword $(MAKEFLAGS)) | grep -v '='), -)$(MAKEFLAGS)

.PHONY: docker
docker:
	docker run \
		-v "$(PWD):$(PWD)" -w "$(PWD)" \
		$(DOCKER_ENV) \
		--rm $(DOCKER_TAG) \
		make $(TARGET) $(DOCKER_MAKEFLAGS)

.PHONY: docker_build
docker_build:
	docker build -t $(DOCKER_TAG) .



#### Clean
.PHONY: clean
clean:
	$(RM) -rf $(BUILD_ROOT)
	cd vendor/rmkit && make clean
