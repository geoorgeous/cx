CC := clang
CSTD = -std-c99
CFLAGS_WARN := -Wformat=2 -Wextra -Wall -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wcast-align -Waggregate-return -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -Wold-style-definition
CFLAGS_NOWARN := -Wno-unused-parameter
CFLAGS_DBG := -ggdb -O0

LIBS := m

TARGET_NAME := cx

BIN_PATH := bin
OBJ_PATH := obj
SRC_PATH := src
DBG_PATH := dbg

SRC_IGNORE := src/gl_context_nix_x11.c src/gl_context_win32.c src/platform_window_nix_x11.c src/platform_window_win32.c

CFLAGS := $(CCSTD) $(CFLAGS_WARN) $(CFLAGS_NOWARN)

ifeq ($(OS),Windows_NT)
	MKDIRCMD := mkdir
	RMDIRCMD := rmdir /q /s
	LIBS += opengl32 gdi32
	TARGET_NAME := $(addsuffix .exe,$(TARGET_NAME))
	CFLAGS += -DPLATFORM_WIN32
else
	MKDIRCMD := mkdir -p
	RMDIRCMD := rm -rf
	LIBS += GL X11
endif

COBJFLAGS := $(CFLAGS) -c

TARGET := $(BIN_PATH)/$(TARGET_NAME)
TARGET_DEBUG := $(DBG_PATH)/$(TARGET_NAME)

SRC := $(filter-out $(SRC_IGNORE), $(foreach x, $(SRC_PATH), $(wildcard $(addprefix $(x)/*,.c*))))
OBJ := $(addprefix $(OBJ_PATH)/, $(addsuffix .o, $(notdir $(basename $(SRC)))))
OBJ_DEBUG := $(addprefix $(DBG_PATH)/, $(addsuffix .o, $(notdir $(basename $(SRC)))))

LIBS := $(foreach x,$(LIBS),$(addprefix -l,$(x)))

default: debug

# release

$(OBJ_PATH)/%.o: $(SRC_PATH)/%.c* | $(OBJ_PATH)
	@$(CC) $(COBJFLAGS) -DNDEBUG -o $@ $<

$(OBJ_PATH):
	@$(MKDIRCMD) $(OBJ_PATH)
	
$(TARGET): $(OBJ) | $(BIN_PATH)
	@$(CC) -o $@ $(OBJ) $(LIBS) $(CFLAGS) -DNDEBUG
	
$(BIN_PATH):
	@$(MKDIRCMD) $(BIN_PATH)

#debug

$(DBG_PATH)/%.o: $(SRC_PATH)/%.c* | $(DBG_PATH)
	@$(CC) $(COBJFLAGS) $(CFLAGS_DBG) -o $@ $<

$(DBG_PATH):
	@$(MKDIRCMD) $(DBG_PATH)

$(TARGET_DEBUG): $(OBJ_DEBUG) | $(DBG_PATH)
	@$(CC) $(CFLAGS) $(CFLAGS_DBG) $(OBJ_DEBUG) $(LIBS) -o $@

#phony rules

.PHONY: release
release: $(TARGET)

.PHONY: debug
debug: $(TARGET_DEBUG)

.PHONY: clean
clean:
	@$(RMDIRCMD) $(OBJ_PATH) $(BIN_PATH) $(DBG_PATH)

.PHONY: compile_commands
compile_commands:
	@bear -- make clean debug
