CC          := clang
LDLIBS      := m 
TARGET_NAME := cx
SRC_DIR     := src
MODE        ?= debug
BUILD_DIR   := build/$(MODE)
OBJ_DIR     := $(BUILD_DIR)/obj
BIN_DIR     := $(BUILD_DIR)/bin
MAKEFLAGS   += --no-print-directory

# Platform specific configuration
ifeq ($(OS),Windows_NT)
	CMD_MKDIR    := mkdir
	CMD_RMDIR    := rmdir /q /s
	LDLIBS       += opengl32 gdi32
	TARGET_NAME  := $(addsuffix .exe,$(TARGET_NAME))
	CFLAGS       += -DPLATFORM_WIN32
else
	CMD_MKDIR    := mkdir -p
	CMD_RMDIR    := rm -rf
	LDLIBS       += GL X11
endif

TARGET := $(BIN_DIR)/$(TARGET_NAME)

CFLAGS += \
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

CFLAGS_release += \
	-DNDEBUG \
	-fdata-sections \
	-ffunction-sections \
	-fno-asynchronous-unwind-tables \
	-fno-exceptions \
	-fno-unwind-tables \
	-flto \
	-Os

CFLAGS_debug += \
	-fsanitize=address,undefined,alignment,leak,nonnull-attribute,pointer-overflow,return \
	-fno-omit-frame-pointer \
	-g3 \
	-ggdb \
	-O0

LDFLAGS +=

LDFLAGS_release += \
	-flto \
	-s \
	-Wl,--gc-sections \
	-Wl,-Map=$(TARGET).map

LDFLAGS_debug += \
	-fsanitize=address,undefined,alignment,leak,nonnull-attribute,pointer-overflow,return

# Do not compile platform-specific code individually:
# We include these types of files in platform-agnostic translation units.
# For example for Windows builds, the file 'foo.win32.c' will be included and built
# as part of the 'foo.c' translation unit.
SRC_IGNORE_WILDCARDS += *.posix.*  # POSIX
SRC_IGNORE_WILDCARDS += *.x11.*    # X11 Window System
SRC_IGNORE_WILDCARDS += *.win32.*    # Windows
SRC_IGNORE_WILDCARDS += *.gl.*     # OpenGL

SRC_ALL        := $(wildcard $(SRC_DIR)/*.c)
SRC_FILTER_OUT := $(foreach x, $(SRC_IGNORE_WILDCARDS), $(wildcard $(SRC_DIR)/$(x)))
SRC            := $(filter-out $(SRC_FILTER_OUT), $(SRC_ALL))

OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

LDLIBS := $(addprefix -l,$(LDLIBS))

default: all

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@$(CC) $(CFLAGS) $(CFLAGS_$(MODE)) -c $< -o $@

$(TARGET): $(OBJ) | $(BIN_DIR)
	@$(CC) $^ $(LDFLAGS) $(LDFLAGS_$(MODE)) $(LDLIBS) -o $@

$(OBJ_DIR):
	@$(CMD_MKDIR) $(OBJ_DIR)

$(BIN_DIR):
	@$(CMD_MKDIR) $(BIN_DIR)

.PHONY: all clean run release release-run debug-run compile_commands

all: $(TARGET)

clean:
	@$(CMD_RMDIR) build

run: $(TARGET)
	@./$(TARGET)

release:
	@$(MAKE) MODE=release all

release-run:
	@$(MAKE) run MODE=release

debug:
	@$(MAKE) MODE=debug all

debug-run:
	@$(MAKE) run MODE=debug

compile_commands:
	@bear -- make clean debug

# dependency includes

-include $(OBJ:.o=.d)
