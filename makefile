CC          := clang
LDLIBS      := m 
SRC_DIR     := src
TARGET      ?= runtime
MODE        ?= debug
BUILD_DIR_ROOT ?= build
BUILD_DIR   := $(BUILD_DIR_ROOT)/$(TARGET)_$(MODE)
OBJ_DIR     := $(BUILD_DIR)/obj
BIN_DIR     := $(BUILD_DIR)/bin
MAKEFLAGS   += --no-print-directory -j16

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

default: help

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@$(CC) $(CFLAGS) $(CFLAGS_$(MODE)) -c $< -o $@

$(TARGET_OUTPUT): $(OBJ) | $(BIN_DIR)
	@$(CC) $^ $(LDFLAGS) $(LDFLAGS_$(MODE)) $(LDLIBS) -o $@

$(OBJ_DIR):
	@$(CMD_MKDIR) $(OBJ_DIR)

$(BIN_DIR):
	@$(CMD_MKDIR) $(BIN_DIR)

.PHONY: \
	help all build clean run run-gdb \
	release debug release-run debug-run debug-gdb \
	ed-release ed-debug ed-release-run ed-debug-run ed-debug-gdb \
	compile-commands

help: ## Shows all commands
	@grep -h -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-18s\033[0m %s\n", $$1, $$2}'

all: release ed-release ## Build both the Runtime and the Editor in release mode

build: $(TARGET_OUTPUT) # Builds TARGET_OUTPUT

clean: ## Cleans the project, deleting the build directory
	@$(CMD_RMDIR) $(BUILD_DIR_ROOT)

run: # Runs TARGET_OUTPUT
	./$(TARGET_OUTPUT)

run-gdb: # Run TARGET_OUTPUT through GDB, showing the backtrace (bt) when the debugger breaks
	gdb -ex "set confirm off" -ex run -ex bt --args ./$(TARGET_OUTPUT)


# runtime recipes

release: ## Builds the Runtime in release mode
	@$(MAKE) --output-sync=target build TARGET=runtime MODE=release

debug: ## Builds the Runtime in debug mode
	@$(MAKE) --output-sync=target build TARGET=runtime MODE=debug

release-run: release ## Builds the Runtime in release mode and runs it
	@$(MAKE) run TARGET=runtime MODE=release

debug-run: debug ## Builds the Runtime in debug mode and runs it
	@$(MAKE) run TARGET=runtime MODE=debug

debug-gdb: debug ## Builds the Runtime in debug mode and runs it through the debugger
	@$(MAKE) run-gdb TARGET=runtime MODE=debug


# editor recipes

ed-release: ## Builds the Editor in release mode
	@$(MAKE) --output-sync=target build TARGET=editor MODE=release

ed-debug: ## Builds the Editor in debug mode
	@$(MAKE) --output-sync=target build TARGET=editor MODE=debug

ed-release-run: ed-release ## Builds the Editor in release mode and runs it
	@$(MAKE) run TARGET=editor MODE=release

ed-debug-run: ed-debug ## Builds the Editor in debug mode and runs it
	@$(MAKE) run TARGET=editor MODE=debug

ed-debug-gdb: ed-debug ## Builds the Editor in debug mode and runs it through the debugger
	@$(MAKE) run-gdb TARGET=editor MODE=debug


compile-commands: clean ## Uses Bear to generate compile_commands.json compilation database for external tools
	@bear $(BEAR_ARGS) -- $(MAKE) debug ed-debug


# dependency includes

-include $(OBJ:.o=.d)
