# MIT License
# Copyright (c) 2026 KAVA Team
# 
# KAVA 2.5 - Makefile

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -I. -I./vm -I./compiler -I./gc -I./collections -I./threads -I./benchmark
LDFLAGS = -lpthread

# SDL2 flags (optional, for graphics)
SDL_CFLAGS = $(shell pkg-config --cflags sdl2 2>/dev/null || echo "")
SDL_LDFLAGS = $(shell pkg-config --libs sdl2 2>/dev/null || echo "")

ifneq ($(SDL_LDFLAGS),)
    CXXFLAGS += -DUSE_SDL $(SDL_CFLAGS)
    LDFLAGS += $(SDL_LDFLAGS) -lSDL2_ttf -lSDL2_image
endif

# Targets
all: kavac kavavm kavabench kpm

# Compilador
kavac: compiler/lexer.cpp compiler/parser.cpp compiler/codegen.cpp compiler/main.cpp
	$(CXX) $(CXXFLAGS) $^ -o kavac

# VM
kavavm: vm/vm.cpp
	$(CXX) $(CXXFLAGS) vm/vm.cpp -o kavavm $(LDFLAGS)

# Benchmark
kavabench: benchmark/main.cpp
	$(CXX) $(CXXFLAGS) benchmark/main.cpp -o kavabench $(LDFLAGS)

# Package Manager
kpm: kpm/main.cpp
	$(CXX) $(CXXFLAGS) kpm/main.cpp -o kpm_bin

# VM standalone (apenas header)
vm/vm.cpp:
	@echo '#include "vm.h"' > vm/vm.cpp
	@echo '' >> vm/vm.cpp
	@echo 'int main(int argc, char** argv) {' >> vm/vm.cpp
	@echo '    if (argc < 2) {' >> vm/vm.cpp
	@echo '        std::cerr << "Uso: kavavm <arquivo.kvb>" << std::endl;' >> vm/vm.cpp
	@echo '        return 1;' >> vm/vm.cpp
	@echo '    }' >> vm/vm.cpp
	@echo '    Kava::VM vm;' >> vm/vm.cpp
	@echo '    if (!vm.loadBytecodeFile(argv[1])) {' >> vm/vm.cpp
	@echo '        std::cerr << "Erro ao carregar: " << argv[1] << std::endl;' >> vm/vm.cpp
	@echo '        return 1;' >> vm/vm.cpp
	@echo '    }' >> vm/vm.cpp
	@echo '    vm.run();' >> vm/vm.cpp
	@echo '    return 0;' >> vm/vm.cpp
	@echo '}' >> vm/vm.cpp

# Testes
test: kavac kavavm
	@bash tests/run_tests.sh

# Benchmark
bench: kavabench
	./kavabench

# Limpeza
clean:
	rm -f kavac kavavm kavabench kpm_bin
	rm -f examples/*.kvb
	rm -f /tmp/kava_test_*

# Instalação
install: all
	mkdir -p /usr/local/bin
	cp kavac /usr/local/bin/
	cp kavavm /usr/local/bin/
	cp kpm_bin /usr/local/bin/kpm

# Documentação
.PHONY: all clean test bench install kpm
