# lg-npu - Top-level Makefile

REPO_ROOT := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
FILELIST  := tools/lint/rtl.f
TOP       := npu_shell
SIM_BUILD := sim/build
SIM_OUT   := sim/results
VEC_DIR   := tb/vectors
WAVE_DIR  := sim/waves

VERILATOR := verilator
PYTHON    := python3

# Common verilator flags
VFLAGS := --sv --cc --exe --build \
          --top-module $(TOP) \
          -Wall -Wno-MULTITOP \
          --trace \
          -CFLAGS "-std=c++17 -I$(REPO_ROOT)tb/integration" \
          -f $(FILELIST)

.PHONY: help
help: ## Show available targets
	@grep -E '^[a-zA-Z_-]+:.*##' $(MAKEFILE_LIST) | \
		awk 'BEGIN {FS = ":.*##"}; {printf "  %-20s %s\n", $$1, $$2}'

.PHONY: lint
lint: ## Run Verilator lint on all RTL
	$(VERILATOR) --sv --lint-only \
		--top-module $(TOP) \
		-Wall \
		-Wno-MULTITOP \
		-f $(FILELIST)

$(SIM_BUILD):
	mkdir -p $(SIM_BUILD)

$(SIM_OUT):
	mkdir -p $(SIM_OUT)

$(WAVE_DIR):
	mkdir -p $(WAVE_DIR)

# Build a test binary: $(call build_test,<build-subdir>,<harness-cpp>)
define build_test
	$(VERILATOR) $(VFLAGS) \
		--Mdir $(SIM_BUILD)/$(1) \
		$(REPO_ROOT)$(2)
endef

# Legacy compile target (original E2E harness)
.PHONY: compile
compile: $(SIM_BUILD) ## Verilate the design (compile to C++)
	$(call build_test,e2e,tb/integration/npu_e2e_harness.cpp)

# Compile each test suite into its own build directory
.PHONY: compile-conv-tests
compile-conv-tests: $(SIM_BUILD)
	$(call build_test,conv_tests,tb/integration/npu_e2e_conv_tests.cpp)

.PHONY: compile-control-tests
compile-control-tests: $(SIM_BUILD)
	$(call build_test,control_tests,tb/integration/npu_control_tests.cpp)

.PHONY: compile-perf-tests
compile-perf-tests: $(SIM_BUILD)
	$(call build_test,perf_tests,tb/perf/npu_perf_tests.cpp)

.PHONY: compile-full-tests
compile-full-tests: $(SIM_BUILD)
	$(call build_test,full_tests,tb/integration/npu_full_tests.cpp)

.PHONY: sim-unit
sim-unit: ## Run unit-level tests
	bash sim/scripts/run_unit.sh

.PHONY: sim-block
sim-block: ## Run block-level tests
	bash sim/scripts/run_block.sh

.PHONY: sim-integration
sim-integration: ## Run integration tests
	bash sim/scripts/run_integration.sh

.PHONY: sim-smoke
sim-smoke: sim-conv-tests sim-control-tests sim-perf-tests ## Run smoke regression (all test suites)
	@echo ""
	@echo "=== sim-smoke: ALL SUITES PASSED ==="

.PHONY: sim-e2e
sim-e2e: compile $(SIM_OUT) $(WAVE_DIR) ## Run original end-to-end convolution test
	$(SIM_BUILD)/e2e/V$(TOP)

.PHONY: sim-conv-tests
sim-conv-tests: compile-conv-tests $(SIM_OUT) $(WAVE_DIR) ## Run deterministic conv test suite (10 cases)
	$(SIM_BUILD)/conv_tests/V$(TOP)

.PHONY: sim-control-tests
sim-control-tests: compile-control-tests $(SIM_OUT) $(WAVE_DIR) ## Run control/sequencing test suite
	$(SIM_BUILD)/control_tests/V$(TOP)

.PHONY: sim-perf-tests
sim-perf-tests: compile-perf-tests $(SIM_OUT) $(WAVE_DIR) ## Run performance counter test suite
	$(SIM_BUILD)/perf_tests/V$(TOP)

.PHONY: sim-full-tests
sim-full-tests: compile-full-tests $(SIM_OUT) $(WAVE_DIR) ## Run full regression test suite
	$(SIM_BUILD)/full_tests/V$(TOP)

.PHONY: sim-full
sim-full: sim-smoke sim-full-tests ## Run complete regression (smoke + full)
	@echo ""
	@echo "=== sim-full: ALL SUITES PASSED ==="

.PHONY: vectors
vectors: ## Generate test vectors from Python reference models
	$(PYTHON) model/vectors/gen_conv_vectors.py  --out-dir $(VEC_DIR)
	$(PYTHON) model/vectors/gen_quant_vectors.py --out-dir $(VEC_DIR)

.PHONY: format
format: ## Run code formatter on RTL and scripts
	bash tools/format/run_format.sh

.PHONY: check-tree
check-tree: ## Validate directory structure
	bash tools/util/check_tree.sh

.PHONY: gen-addrmap
gen-addrmap: ## Regenerate SV address-map package from spec
	$(PYTHON) tools/gen/gen_addrmap.py \
		--out include/pkg/npu_addrmap_pkg.sv

.PHONY: gen-headers
gen-headers: ## Regenerate C/UAPI headers from register spec
	$(PYTHON) tools/gen/gen_headers.py \
		--out-dir sw/uapi

.PHONY: gen
gen: gen-addrmap gen-headers ## Run all generators

.PHONY: viz
viz: ## Generate architecture diagrams into docs/diagrams/
	bash tools/visualize/gen_all.sh

.PHONY: waves
waves: ## Open latest waveform in Surfer viewer
	bash tools/visualize/open_surfer.sh

.PHONY: clean
clean: ## Remove all build artifacts
	rm -rf $(SIM_BUILD) $(SIM_OUT) obj_dir
	rm -f sim/waves/*.vcd sim/waves/*.fst

.PHONY: clean-all
clean-all: clean ## Remove build artifacts and generated files
	rm -rf generated/
