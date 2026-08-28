CC          := clang
LDLIBS      := m 
SRC_DIR     := src
TARGET      ?= runtime
MODE        ?= debug
BUILD_DIR_ROOT ?= build
BUILD_DIR   := $(BUILD_DIR_ROOT)/$(TARGET)/$(MODE)
OBJ_DIR     := $(BUILD_DIR)/obj
BIN_DIR     := $(BUILD_DIR)/bin
JOBS        ?= 16
MAKEFLAGS   += --no-print-directory -j$(JOBS) --output-sync=target

ifeq ($(TARGET),runtime)
	TARGET_NAME ?= cx
else ifeq ($(TARGET),editor)
	TARGET_NAME ?= cx-ed
endif

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

TARGET_OUTPUT := $(BIN_DIR)/$(TARGET_NAME)

CFLAGS += \
	-fdiagnostics-color=always \
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
	-O2

CFLAGS_debug += \
	-fsanitize=address,undefined,alignment,leak,nonnull-attribute,pointer-overflow,return \
	-fno-omit-frame-pointer \
	-g3 \
	-ggdb \
	-O0

LDFLAGS_release += \
	-flto \
	-s \
	-Wl,--gc-sections \
	-Wl,-Map=$(TARGET_OUTPUT).map

LDFLAGS_debug += \
	-fsanitize=address,undefined,alignment,leak,nonnull-attribute,pointer-overflow,return \
	-rdynamic

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

ifeq ($(TARGET),runtime)
	SRC        := $(filter-out $(SRC_DIR)/cx_ed_app.c, $(SRC))
else ifeq ($(TARGET),editor)
	SRC        := $(filter-out $(SRC_DIR)/cx_rt_app.c, $(SRC))
endif

OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

LDLIBS := $(addprefix -l,$(LDLIBS))

default: all

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@$(CC) $(CFLAGS) $(CFLAGS_$(MODE)) -c $< -o $@

$(TARGET_OUTPUT): $(OBJ) | $(BIN_DIR)
	@$(CC) $^ $(LDFLAGS) $(LDFLAGS_$(MODE)) $(LDLIBS) -o $@

$(OBJ_DIR):
	@$(CMD_MKDIR) $(OBJ_DIR)

$(BIN_DIR):
	@$(CMD_MKDIR) $(BIN_DIR)

.PHONY: \
	all build run gdb clean \
	release debug release-run debug-run debug-gdb \
	ed-release ed-debug ed-release-run ed-debug-run ed-debug-gdb \
	compile_commands

all: release ed-release

build: $(TARGET_OUTPUT)

run: build
	env ASAN_OPTIONS="color=always" ./$(TARGET_OUTPUT)

# build debug target, run through gdb, and show backtrace when gdb breaks
# (`set confirm off` disabled confirmation checks inside gdb)

gdb: build
	gdb -iex "set color on" -ex "set confirm off" -ex run -ex bt --args ./$(TARGET_OUTPUT)

clean:
	@$(CMD_RMDIR) $(BUILD_DIR_ROOT)

release:
	@$(MAKE) build TARGET=runtime MODE=release

debug:
	@$(MAKE) build TARGET=runtime MODE=debug

release-run:
	@$(MAKE) run TARGET=runtime MODE=release

debug-run:
	@$(MAKE) run TARGET=runtime MODE=debug

debug-gdb:
	@$(MAKE) gdb TARGET=runtime MODE=debug

ed-release:
	@$(MAKE) build TARGET=editor MODE=release

ed-debug:
	@$(MAKE) build TARGET=editor MODE=debug

ed-release-run:
	@$(MAKE) run TARGET=editor MODE=release

ed-debug-run:
	@$(MAKE) run TARGET=editor MODE=debug

ed-debug-gdb:
	@$(MAKE) gdb TARGET=editor MODE=debug

# generate compile_commands.json compilation database for clangd lsp tooling

compile_commands: clean
	@bear -- $(MAKE) debug ed-debug

# dependency includes

-include $(OBJ:.o=.d)
