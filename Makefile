ASMJIT_DIR := asmjit
ASMJIT_LIB_DIR := $(ASMJIT_DIR)/build
ASMJIT_LIB := $(ASMJIT_LIB_DIR)/libasmjit.a
ASMJIT_GIT_REPO := https://github.com/asmjit/asmjit.git

INCLUDES := -Isrc/includes/ -I$(ASMJIT_DIR)/src
CFLAGS := $(INCLUDES)\
	-Wall \
	-Wextra \
	-g \
	-std=c++17

LDFLAGS := -L $(ASMJIT_LIB_DIR) -lasmjit

SOURCES := $(shell find src/ -name "*.cpp")
OBJECTS := $(addprefix build/,$(SOURCES:.cpp=.o))

EXEC_TEST := windte

all: $(ASMJIT_LIB) $(EXEC_TEST)

$(EXEC_TEST): $(OBJECTS)
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(ASMJIT_LIB): $(ASMJIT_DIR)
	cd $(ASMJIT_DIR) && mkdir -p build && cd build && cmake .. -DASMJIT_NO_AARCH64=ON -DASMJIT_NO_JIT=ON -DASMJIT_NO_JIT=ON -DASMJIT_STATIC=ON && make

$(ASMJIT_DIR):
	git clone $(ASMJIT_GIT_REPO) $(ASMJIT_DIR)

build/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) -c $< -o $@

test: $(EXEC_TEST)
	./$(EXEC_TEST) grammar/wind.0.w

clean:
	rm -rf build/ $(EXEC_TEST)

dump_bin:
	objdump -D -b binary -m i386:x86-64 output.bin -M intel
	bat output.asm

.PHONY: test dump_bin