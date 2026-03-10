# lg-npu - Top-level Makefile

REPO_ROOT := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
FILELIST  := tools/lint/rtl.f
TOP       := npu_shell
SIM_BUILD := sim/build
SIM_OUT   := sim/results
VEC_DIR   := tb/vectors

VERILATOR := verilator
PYTHON    := python3

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

.PHONY: compile
compile: $(SIM_BUILD) ## Verilate the design (compile to C++)
	$(VERILATOR) --sv --cc --exe --build \
		--top-module $(TOP) \
		-Wall -Wno-MULTITOP \
		--Mdir $(SIM_BUILD) \
		--trace \
		-CFLAGS "-std=c++17" \
		-f $(FILELIST) \
		$(REPO_ROOT)tb/integration/npu_e2e_harness.cpp

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
sim-smoke: ## Run smoke regression
	bash sim/scripts/run_all.sh sim/regressions/smoke.list

.PHONY: sim-e2e
sim-e2e: compile $(SIM_OUT) ## Run end-to-end convolution test
	mkdir -p sim/waves
	$(SIM_BUILD)/V$(TOP)

.PHONY: sim-full
sim-full: ## Run full regression
	bash sim/scripts/run_all.sh sim/regressions/full.list

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
