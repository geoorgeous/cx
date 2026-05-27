CC          := clang
CSTD         = -std-c99
CFLAGS_WARN := \
			-pedantic-errors \
			-Wformat=2 \
			-Wall \
			-Wextra \
			-Wfloat-equal \
			-Wundef \
			-Wshadow \
			-Wpointer-arith \
			-Wcast-align \
            -Waggregate-return \
			-Wcast-qual \
			-Wstrict-prototypes \
			-Wmissing-prototypes \
			-Wold-style-definition \
			-Wconversion \
			-Wswitch-enum \
			-Wstrict-overflow=5 \
			-Wdouble-promotion \
			-Wmissing-declarations \
			-Wstrict-aliasing \
			-fstrict-aliasing \
			-Wvla \
			-Wpacked \
			-Wpadded \
			-Walloca \
			-Wnull-dereference \
			-Wdisabled-optimization \
			-fno-common

CFLAGS_DBG  := \
			-ggdb \
			-O0 \
			-fsanitize=address,undefined \
			-fno-omit-frame-pointer \
			-g3

LIBS        := m
TARGET_NAME := cx
SRC_DIR     := src
BIN_DIR     := bin
DBG_DIR     := dbg

# Do not compile platform-specific code individually:
# We include these types of files in platform-agnostic translation units.
# For example for Windows builds, the file 'foo.win32.c' will be included and built
# as part of the 'foo.c' translation unit.
SRC_IGNORE_WILDCARDS := *.nix.* *.nix_x11.* *.win32.* *.gl.*

CFLAGS := $(CCSTD) $(CFLAGS_WARN) $(CFLAGS_NOWARN)

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
	LIBS        += GL X11
endif

COBJFLAGS := $(CFLAGS) -c

TARGET     := $(BIN_DIR)/$(TARGET_NAME)
TARGET_DBG := $(DBG_DIR)/$(TARGET_NAME)

SRC_ALL        := $(wildcard $(addprefix $(SRC_DIR)/*, .c*))
SRC_FILTER_OUT := $(foreach x, $(SRC_IGNORE_WILDCARDS), $(wildcard $(SRC_DIR)/$(x)))
SRC            := $(filter-out $(SRC_FILTER_OUT), $(SRC_ALL))

OBJ     := $(addprefix $(BIN_DIR)/, $(addsuffix .o, $(notdir $(basename $(SRC)))))
OBJ_DBG := $(addprefix $(DBG_DIR)/, $(addsuffix .o, $(notdir $(basename $(SRC)))))

LIBS := $(foreach x,$(LIBS),$(addprefix -l,$(x)))

default: debug

# release

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c* | $(BIN_DIR)
	@$(CC) $(COBJFLAGS) -DNDEBUG -o $@ $<

$(BIN_DIR):
	@$(MKDIRCMD) $(BIN_DIR)
	
$(TARGET): $(OBJ) | $(BIN_DIR)
	@$(CC) -o $@ $(OBJ) $(LIBS) $(CFLAGS)

#debug

$(DBG_DIR)/%.o: $(SRC_DIR)/%.c* | $(DBG_DIR)
	@$(CC) $(COBJFLAGS) $(CFLAGS_DBG) -o $@ $<

$(DBG_DIR):
	@$(MKDIRCMD) $(DBG_DIR)

$(TARGET_DBG): $(OBJ_DBG) | $(DBG_DIR)
	@$(CC) $(CFLAGS) $(CFLAGS_DBG) $(OBJ_DBG) $(LIBS) -o $@

#phony rules

.PHONY: release
release: $(TARGET)

.PHONY: debug
debug: $(TARGET_DBG)

.PHONY: clean
clean:
	@$(RMDIRCMD) $(BIN_DIR) $(DBG_DIR)

.PHONY: compile_commands
compile_commands:
	@bear -- make clean debug
