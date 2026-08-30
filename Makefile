BUILD_DIR := build/Debug
PRESET := conan-debug
COVERAGE_DIR := build/coverage_html
COVERAGE_INFO := build/coverage.info
SANITIZER ?= address,undefined
NETWORK_FIXTURE := ./tests/capture/network_fixture.sh
TEST_BINARY := ./$(BUILD_DIR)/tests/surma_tests

.PHONY: bootstrap install configure build test integration integration-setup integration-test integration-clean run clean rebuild fresh coverage coverage-clean vuln-scan san san-address san-thread san-memory hugepages

bootstrap:
	$(MAKE) install
	$(MAKE) configure
	$(MAKE) build

install:
	conan install . --build=missing -s build_type=Debug

configure:
	cmake --preset $(PRESET)

build:
	cmake --build --preset $(PRESET)

test:
	ctest --test-dir $(BUILD_DIR) --output-on-failure

integration:
	$(MAKE) integration-setup
	trap '$(MAKE) integration-clean' EXIT; \
	$(MAKE) integration-test

integration-setup:
	sudo bash $(NETWORK_FIXTURE) setup

integration-test:
	sudo ip netns exec surma-test $(TEST_BINARY) "[integration]" &
	TEST_PID=$$!; \
	sleep 1; \
	sudo bash $(NETWORK_FIXTURE) inject; \
	wait $$TEST_PID

integration-clean:
	sudo bash $(NETWORK_FIXTURE) teardown

run:
	sudo ./$(BUILD_DIR)/surma

clean:
	cmake --build --preset $(PRESET) --target clean

rebuild:
	cmake --build --preset $(PRESET) --clean-first

fresh:
	@test "$(BUILD_DIR)" = "build/Debug" || { \
		echo "ERROR: refusing to delete unexpected BUILD_DIR='$(BUILD_DIR)'"; \
		exit 1; \
	}
	rm -rf -- "$(BUILD_DIR)"
	$(MAKE) bootstrap

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
