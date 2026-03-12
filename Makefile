# lg-npu - Top-level Makefile

REPO_ROOT := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
FILELIST  := tools/lint/rtl.f
TOP       := npu_shell
SIM_BUILD   := sim/build
SYNTH_BUILD := syn/build
VEC_DIR     := tb/vectors
WAVE_DIR    := sim/waves

VERILATOR := verilator
PYTHON    := python3

CC       := gcc
SW_STD   := -std=c17
SW_WARN  := -Wall -Wextra -Wpedantic -Werror \
            -Wcast-align -Wcast-qual -Wconversion -Wdouble-promotion \
            -Wfloat-equal -Wformat=2 -Wformat-nonliteral -Wformat-security \
            -Wimplicit-fallthrough -Wmissing-declarations \
            -Wmissing-field-initializers -Wnull-dereference -Wpacked \
            -Wpointer-arith -Wredundant-decls -Wshadow -Wsign-conversion \
            -Wstrict-overflow=5 -Wswitch-default -Wswitch-enum -Wundef \
            -Wuninitialized -Wunreachable-code -Wunused -Wvla -Wwrite-strings
SW_OPT   := -Ofast -flto
SW_FLAGS := $(SW_STD) $(SW_WARN) $(SW_OPT)
SW_PIC   := -fPIC -fvisibility=hidden
SW_BUILD := sw/build
SW_TEST_BUILD := sw/build/tests

SW_RUNTIME_SRC := sw/runtime/command_builder.c \
                  sw/runtime/submit.c \
                  sw/runtime/liblgnpu.c \
                  sw/runtime/tensor_desc.c
SW_RUNTIME_OBJ := $(patsubst sw/runtime/%.c,$(SW_BUILD)/%.o,$(SW_RUNTIME_SRC))

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

.PHONY: sim-smoke
sim-smoke: sim-conv-tests sim-control-tests sim-perf-tests ## Run smoke regression (all test suites)
	@echo ""
	@echo "=== sim-smoke: ALL SUITES PASSED ==="

.PHONY: sim-e2e
sim-e2e: compile $(WAVE_DIR) ## Run original end-to-end convolution test
	$(SIM_BUILD)/e2e/V$(TOP)

.PHONY: sim-conv-tests
sim-conv-tests: compile-conv-tests $(WAVE_DIR) ## Run deterministic conv test suite (10 cases)
	$(SIM_BUILD)/conv_tests/V$(TOP)

.PHONY: sim-control-tests
sim-control-tests: compile-control-tests $(WAVE_DIR) ## Run control/sequencing test suite
	$(SIM_BUILD)/control_tests/V$(TOP)

.PHONY: sim-perf-tests
sim-perf-tests: compile-perf-tests $(WAVE_DIR) ## Run performance counter test suite
	$(SIM_BUILD)/perf_tests/V$(TOP)

.PHONY: sim-full-tests
sim-full-tests: compile-full-tests $(WAVE_DIR) ## Run full regression test suite
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

# SW build targets
$(SW_BUILD):
	mkdir -p $(SW_BUILD)

.PHONY: sw-check
sw-check: ## Syntax-check all SW runtime sources (C23)
	$(CC) $(SW_FLAGS) -fsyntax-only $(SW_RUNTIME_SRC)

.PHONY: sw-build
sw-build: $(SW_BUILD) ## Compile SW runtime into .a and .so (C23)
	@for src in $(SW_RUNTIME_SRC); do \
		obj=$(SW_BUILD)/$$(basename $${src} .c).o; \
		$(CC) $(SW_FLAGS) $(SW_PIC) -c $$src -o $$obj; \
	done
	ar rcs $(SW_BUILD)/liblgnpu_rt.a $(SW_RUNTIME_OBJ)
	$(CC) $(SW_FLAGS) -shared -o $(SW_BUILD)/liblgnpu_rt.so $(SW_RUNTIME_OBJ)

.PHONY: synth-yosys
synth-yosys: ## Run Yosys generic synthesis to catch structural issues
	@mkdir -p $(SYNTH_BUILD)
	sv2v -Iinclude -Iinclude/pkg -Iinclude/defines \
		$$(grep '\.sv$$' $(FILELIST)) > $(SYNTH_BUILD)/design.v
	yosys -l $(SYNTH_BUILD)/synth.log -p 'read_verilog $(SYNTH_BUILD)/design.v; hierarchy -top $(TOP) -check; proc; opt; check -assert; synth -top $(TOP); stat'
	@echo ""
	@echo "=== synth-yosys: log at $(SYNTH_BUILD)/synth.log ==="

# SW test targets
SW_TEST_FLAGS := $(SW_STD) $(SW_WARN) -Ofast -Wno-unused-function

$(SW_TEST_BUILD):
	mkdir -p $(SW_TEST_BUILD)

.PHONY: sw-test-tensor
sw-test-tensor: $(SW_TEST_BUILD) ## Run tensor validation tests
	$(CC) $(SW_TEST_FLAGS) -o $(SW_TEST_BUILD)/test_tensor \
		sw/tests/tensor/test_tensor.c \
		sw/runtime/tensor_desc.c
	$(SW_TEST_BUILD)/test_tensor

.PHONY: sw-test-layout
sw-test-layout: $(SW_TEST_BUILD) ## Run layout conversion tests
	$(CC) $(SW_TEST_FLAGS) -o $(SW_TEST_BUILD)/test_layout \
		sw/tests/layout/test_layout.c \
		sw/runtime/tensor_desc.c
	$(SW_TEST_BUILD)/test_layout

.PHONY: sw-test-command
sw-test-command: $(SW_TEST_BUILD) ## Run command construction tests
	$(CC) $(SW_TEST_FLAGS) -o $(SW_TEST_BUILD)/test_command \
		sw/tests/command/test_command.c \
		sw/runtime/command_builder.c
	$(SW_TEST_BUILD)/test_command

.PHONY: sw-test-device
sw-test-device: $(SW_TEST_BUILD) ## Run device control tests (mock backend)
	$(CC) $(SW_TEST_FLAGS) -o $(SW_TEST_BUILD)/test_device \
		sw/tests/device/test_device.c \
		sw/runtime/liblgnpu.c \
		sw/runtime/submit.c
	$(SW_TEST_BUILD)/test_device

.PHONY: sw-test-dma
sw-test-dma: $(SW_TEST_BUILD) ## Run DMA operation tests (mock backend)
	$(CC) $(SW_TEST_FLAGS) -o $(SW_TEST_BUILD)/test_dma \
		sw/tests/runtime/test_dma.c \
		sw/runtime/liblgnpu.c \
		sw/runtime/submit.c
	$(SW_TEST_BUILD)/test_dma

.PHONY: sw-test-integration
sw-test-integration: $(SW_TEST_BUILD) ## Run integration tests (mock backend)
	$(CC) $(SW_TEST_FLAGS) -o $(SW_TEST_BUILD)/test_integration \
		sw/tests/integration/test_integration.c \
		sw/runtime/command_builder.c \
		sw/runtime/liblgnpu.c \
		sw/runtime/submit.c \
		sw/runtime/tensor_desc.c
	$(SW_TEST_BUILD)/test_integration

.PHONY: sw-test
sw-test: sw-test-tensor sw-test-layout sw-test-command sw-test-device sw-test-dma sw-test-integration ## Run all SW runtime tests
	@echo ""
	@echo "=== sw-test: ALL SUITES PASSED ==="

.PHONY: clean
clean: ## Remove all build artifacts
	rm -rf $(SIM_BUILD) $(SYNTH_BUILD) $(SW_BUILD) $(SW_TEST_BUILD) obj_dir
	rm -f sim/waves/*.vcd sim/waves/*.fst

.PHONY: clean-all
clean-all: clean ## Remove build artifacts and generated files
	rm -rf generated/
