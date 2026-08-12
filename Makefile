BUILD_DIR := build/Debug
PRESET := conan-debug
COVERAGE_DIR := build/coverage_html
COVERAGE_INFO := build/coverage.info
SANITIZER ?= address,undefined

.PHONY: bootstrap install configure build test integration run clean rebuild coverage coverage-clean vuln-scan san san-address san-thread san-memory hugepages

bootstrap: install configure build

install:
	conan install . --build=missing -s build_type=Debug

configure:
	cmake --preset $(PRESET)

build:
	cmake --build --preset $(PRESET)

test:
	ctest --test-dir $(BUILD_DIR) --output-on-failure

integration:
	sudo ./$(BUILD_DIR)/tests/surma_tests "[integration]"

run:
	./$(BUILD_DIR)/surma

clean:
	ninja -C $(BUILD_DIR) clean

rebuild: clean build

coverage: SURMA_COVERAGE=ON
coverage: coverage-clean
	cmake --preset $(PRESET) -DSURMA_COVERAGE=ON
	cmake --build --preset $(PRESET)
	ctest --test-dir $(BUILD_DIR) --output-on-failure
	lcov --capture \
		--directory $(BUILD_DIR) \
		--output-file $(COVERAGE_INFO) \
		--exclude '/usr/*' \
		--exclude '/*/catch2/*' \
		--exclude '*/spdlog/*' \
		--ignore-errors unused
	lcov --summary $(COVERAGE_INFO) --fail-under-lines 70
	genhtml $(COVERAGE_INFO) \
		--output-directory $(COVERAGE_DIR)
	@echo "Report at $(COVERAGE_DIR)/index.html"

coverage-clean:
	find $(BUILD_DIR) -name '*.gcda' -delete

vuln-scan:
	conan lock create .
	osv-scanner scan source .
	trivy fs --scanners vuln --exit-code 1 .

san:
	cmake --preset $(PRESET) -DSURMA_SANITIZER=$(SANITIZER)
	cmake --build --preset $(PRESET)
	ctest --test-dir $(BUILD_DIR) --output-on-failure

san-address:
	$(MAKE) san SANITIZER=address,undefined

san-thread:
	$(MAKE) san SANITIZER=thread

san-memory:
	$(MAKE) san SANITIZER=memory

# register hugetables so surma can utilize them
hugepages:
	sudo sysctl -w vm/nr_hugepages=4
