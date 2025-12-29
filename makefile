# tool macros
CC ?= gcc
CSTD = -std-c99
CFLAGS_WARN := -Wformat=2 -Wextra -Wall -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wcast-align -Waggregate-return -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -Wold-style-definition
CFLAGS_NOWARN := -Wno-unused-parameter
DBGFLAGS := -g -O0
COBJFLAGS := $(CFLAGS) -c
LIBS := m GL X11
TARGET_NAME := cx

# path macros
BIN_PATH := bin
OBJ_PATH := obj
SRC_PATH := .
DBG_PATH := dbg

SRC_IGNORE := ./font.c ./gl_context_nix_x11.c ./gl_context_win32.c ./otf.c ./platform_window_nix_x11.c ./platform_window_win32.c

# compile macros
ifeq ($(OS),Windows_NT)
	TARGET_NAME := $(addsuffix .exe,$(TARGET_NAME))
endif
TARGET := $(BIN_PATH)/$(TARGET_NAME)
TARGET_DEBUG := $(DBG_PATH)/$(TARGET_NAME)

# default rule
default: debug

# src files & obj files
SRC := $(filter-out $(SRC_IGNORE), $(foreach x, $(SRC_PATH), $(wildcard $(addprefix $(x)/*,.c*))))
OBJ := $(addprefix $(OBJ_PATH)/, $(addsuffix .o, $(notdir $(basename $(SRC)))))
OBJ_DEBUG := $(addprefix $(DBG_PATH)/, $(addsuffix .o, $(notdir $(basename $(SRC)))))

CFLAGS := $(CCSTD)\
          $(CFLAGS_WARN)\
          $(CFLAGS_NOWARN)

LIBS := $(foreach x, $(LIBS), $(addprefix -l, $(x)))

# non-phony targets
$(TARGET): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LIBS) $(CFLAGS)

$(OBJ_PATH)/%.o: $(SRC_PATH)/%.c*
	$(CC) $(COBJFLAGS) -o $@ $<

$(DBG_PATH)/%.o: $(SRC_PATH)/%.c*
	$(CC) $(COBJFLAGS) $(DBGFLAGS) -o $@ $<

$(TARGET_DEBUG): $(OBJ_DEBUG)
	$(CC) $(LIBS) $(CFLAGS) $(DBGFLAGS) $(OBJ_DEBUG) -o $@

# phony rules
.PHONY: makedir
makedir:
	@mkdir -p $(BIN_PATH) $(OBJ_PATH) $(DBG_PATH)

.PHONY: release
release: makedir $(TARGET)

.PHONY: debug
debug: makedir $(TARGET_DEBUG)

.PHONY: clean
clean:
	@rm -rf $(OBJ_PATH) $(BIN_PATH) $(DBG_PATH)

.PHONY: compile_commands
compilation_commands:
	bear -- make clean makedir debug