ASMJIT_DIR := asmjit
ASMJIT_LIB := $(ASMJIT_DIR)/build/libasmjit.so
ASMJIT_GIT_REPO := https://github.com/asmjit/asmjit.git
LIB_DIR := libs

INCLUDES := -Isrc/includes/ -I$(ASMJIT_DIR)/src
CFLAGS := $(INCLUDES)\
	-Wall \
	-Wextra \
	-g \
	-std=c++17

LDFLAGS := -L$(LIB_DIR) -lasmjit

SOURCES := $(shell find src/ -name "*.cpp")
OBJECTS := $(addprefix build/,$(SOURCES:.cpp=.o))

EXEC_TEST := windte

all: $(EXEC_TEST)

$(EXEC_TEST): $(ASMJIT_LIB) $(OBJECTS)
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(ASMJIT_LIB): $(ASMJIT_DIR)
	cd $(ASMJIT_DIR) && mkdir -p build && cd build && cmake .. && make
	@mkdir -p $(LIB_DIR)
	cp $(ASMJIT_LIB) $(LIB_DIR)/

$(ASMJIT_DIR):
	git clone $(ASMJIT_GIT_REPO) $(ASMJIT_DIR)

build/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) -c $< -o $@

test: $(EXEC_TEST)
	LD_LIBRARY_PATH=$(LIB_DIR) ./$(EXEC_TEST)

clean:
	rm -rf build/ $(EXEC_TEST)

dump_bin:
	objdump -D -b binary -m i386:x86-64 output.bin -M intel
	bat output.asm