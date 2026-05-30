CC          := clang
LDLIBS      := m 
TARGET_NAME := cx
SRC_DIR     := src
MODE        ?= debug
BUILD_DIR   := build/$(MODE)
OBJ_DIR     := $(BUILD_DIR)/obj
BIN_DIR     := $(BUILD_DIR)/bin

CFLAGS := \
	-ffp-contract=off \
	-fno-common \
	-fstrict-flex-arrays=3 \
	-Wall -Wextra \
	-Waggregate-return \
	-Walloca \
	-Wbad-function-cast \
	-Wcast-align \
	-Wcast-qual \
	-Wconversion \
	-Wdate-time \
	-Wdisabled-optimization \
	-Wdouble-promotion \
	-Wenum-conversion \
	-Wfloat-equal \
	-Wformat=2 \
	-Winline \
	-Wmissing-declarations \
	-Wmissing-prototypes \
	-Wnested-externs \
	-Wnull-dereference \
	-Wold-style-definition \
	-Wpacked \
	-Wpointer-arith \
	-Wredundant-decls \
	-Wshadow \
	-Wsign-conversion \
	-Wstrict-aliasing \
	-Wstrict-overflow=2 \
	-Wstrict-prototypes \
	-Wundef \
	-Wunreachable-code \
	-Wunused \
	-Wwrite-strings \
	-Wvla \
	-MMD -MP \
	-std=c99 -pedantic-errors

	#-Wswitch-default \
	#-Wswitch-enum \
	#-Wpadded \

CFLAGS_release := \
	-DNDEBUG \
	-fdata-sections \
	-ffunction-sections \
	-flto \
	-O2

CFLAGS_debug := \
	-fsanitize=address,undefined,alignment,leak,nonnull-attribute,pointer-overflow,return \
	-fno-omit-frame-pointer \
	-g3 \
	-ggdb \
	-O0

LDFLAGS :=

LDFLAGS_release := \
	-flto \
	-Wl,--gc-sections \
	-Wl,-Map=$(TARGET_RELEASE).map

LDFLAGS_debug := \
	-fsanitize=address,undefined,alignment,leak,nonnull-attribute,pointer-overflow,return

# Do not compile platform-specific code individually:
# We include these types of files in platform-agnostic translation units.
# For example for Windows builds, the file 'foo.win32.c' will be included and built
# as part of the 'foo.c' translation unit.
SRC_IGNORE_WILDCARDS += *.posix.*  # POSIX
SRC_IGNORE_WILDCARDS += *.x11.*    # X11 Window System
SRC_IGNORE_WILDCARDS += *.win32.*    # Windows
SRC_IGNORE_WILDCARDS += *.gl.*     # OpenGL

# Platform specific configuration
ifeq ($(OS),Windows_NT)
	MKDIRCMD    := mkdir
	RMDIRCMD    := rmdir /q /s
	LIBS        += opengl32 gdi32
	TARGET_NAME := $(addsuffix .exe,$(TARGET_NAME))
	CFLAGS      += -DPLATFORM_WIN32
else
	MKDIRCMD    := mkdir -p
	RMDIRCMD    := rm -rf
	LDLIBS        += GL X11
endif

TARGET := $(BIN_DIR)/$(TARGET_NAME)

SRC_ALL        := $(wildcard $(SRC_DIR)/*.c)
SRC_FILTER_OUT := $(foreach x, $(SRC_IGNORE_WILDCARDS), $(wildcard $(SRC_DIR)/$(x)))
SRC            := $(filter-out $(SRC_FILTER_OUT), $(SRC_ALL))

OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

LDLIBS := $(foreach x,$(LDLIBS),$(addprefix -l,$(x)))

default: all

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@$(CC) $(CFLAGS) $(CFLAGS_$(MODE)) -c $< -o $@

$(TARGET): $(OBJ) | $(BIN_DIR)
	@$(CC) $^ $(LDFLAGS) $(LDFLAGS_$(MODE)) $(LDLIBS) -o $@

$(OBJ_DIR):
	@$(MKDIRCMD) $(OBJ_DIR)

$(BIN_DIR):
	@$(MKDIRCMD) $(BIN_DIR)

.PHONY: all clean run release debug compile_commands

all: $(TARGET)

clean:
	@$(RMDIRCMD) build

release:
	@$(MAKE) MODE=release all

release-run: release
	@./$(TARGET)

debug:
	@$(MAKE) MODE=debug all

debug-run: debug
	@./$(TARGET)

compile_commands:
	@bear -- make clean debug

# dependency includes

-include $(OBJ:.o=.d)
