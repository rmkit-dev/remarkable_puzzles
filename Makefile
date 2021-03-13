include defs.mk

#### Standard builds
.PHONY: default all
default: docker
all: docker_build release

.PHONY: release
release: BUILD=release
release: default

.PHONY: debug
debug: BUILD=debug
debug: default

.PHONY: prof
prof: BUILD=prof
prof: default

.PHONY: puzzle_icons
puzzle_icons: BUILD=icons
puzzle_icons: default

.PHONY: resim
resim: BUILD=resim
resim: ARCH=dev
resim: default



#### rmkit
RMKIT_FLAGS = -D"REMARKABLE=1" \
	-pthread -lpthread

$(BUILD_DIR)/rmkit.h:
	cd vendor/rmkit && $(MAKE) rmkit.h
	mkdir -p $(dir $@)
	mv vendor/rmkit/src/build/rmkit.h $(BUILD_DIR)
	cp vendor/rmkit/src/build/$(STB) $(BUILD_DIR)



#### Game files -- defines GAME_OBJS
include Makefile.games



#### Remarkable app
TARGET = puzzles
SRCS   = $(shell find src/ -type f -name '*.cpp')
OBJS   = $(addprefix $(BUILD_DIR)/, $(SRCS:.cpp=.o))

INCLUDES  = -I./ -Isrc/ -Ivendor/inih -Ivendor/puzzles
# rmkit produces a lot of warnings; making it a "system" header disables those
INCLUDES += -isystem $(BUILD_DIR)/

CXXFLAGS  = -Wall $(INCLUDES) $(RMKIT_FLAGS) $(BUILD_FLAGS)
CXXFLAGS += -fdata-sections -ffunction-sections -Wl,--gc-sections
CXXFLAGS += -D'RMP_COMPILE_DATE="$(RMP_COMPILE_DATE)"'
CXXFLAGS += -D'RMP_VERSION="$(RMP_VERSION)"'

# rmkit has to be built as a monolith, so combine all the sources into one
MAIN_OBJ = $(BUILD_DIR)/main.o
MAIN_SRC = $(MAIN_OBJ:.o=.cpp)
OBJS = $(MAIN_OBJ)

$(MAIN_SRC): $(SRCS)
	echo -n "" > $(MAIN_SRC)
	for f in $(SRCS); do echo "#include \"$$f\"" >> $(MAIN_SRC); done

$(MAIN_OBJ): $(BUILD_DIR)/rmkit.h $(MAIN_SRC)
	$(CXX) $(CXXFLAGS) -c -o $@ $(MAIN_SRC)

$(BUILD_DIR)/$(TARGET): $(GAME_OBJS) $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(GAME_OBJS) $(OBJS) $(BUILD_DIR)/$(STB)

.PHONY: $(TARGET)
$(TARGET): $(BUILD_DIR)/$(TARGET)


#### auto-generate deps
CXXFLAGS += -MMD -MP
DEPS=$(OBJS:.o=.d)
-include $(DEPS)


#### Docker version of the toolchain
DOCKER_TAG = remarkable_puzzles_toolchain:latest

DOCKER_MAKEFLAGS=$(if $(shell echo $(firstword $(MAKEFLAGS)) | grep -v '='), -)$(MAKEFLAGS)

.PHONY: docker
docker:
	docker run \
		--user $(shell id -u):$(shell id -g) \
		-v "$(CURDIR):$(CURDIR)" -w "$(CURDIR)" \
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
