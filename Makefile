BUILD_DIR := build/Debug
PRESET := conan-debug

.PHONY: install configure build test run clean rebuild bootstrap

install:
	conan install . --build=missing -s build_type=Debug

configure:
	cmake --preset $(PRESET)

build:
	cmake --build --preset $(PRESET)

test:
	ctest --test-dir $(BUILD_DIR) --output-on-failure

run:
	./$(BUILD_DIR)/surma

clean:
	ninja -C $(BUILD_DIR) clean

rebuild: clean build

bootstrap: install configure build
