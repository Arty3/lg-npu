// ============================================================================
// rtl.f - Verilator / simulator file list (dependency-ordered)
// ============================================================================

// --- Defines ----------------------------------------------------------------
-Iinclude
-Iinclude/pkg
-Iinclude/defines

// --- Packages (dependency order: types first, then dependents) --------------
include/pkg/npu_types_pkg.sv
include/pkg/npu_perf_pkg.sv
include/pkg/npu_cfg_pkg.sv
include/pkg/npu_addrmap_pkg.sv
include/pkg/npu_cmd_pkg.sv
include/pkg/npu_tensor_pkg.sv

// --- Interfaces -------------------------------------------------------------
rtl/ifaces/mmio_if.sv
rtl/ifaces/cmd_if.sv
rtl/ifaces/mem_req_rsp_if.sv
rtl/ifaces/stream_if.sv
rtl/ifaces/tensor_if.sv

// --- Common building blocks -------------------------------------------------
rtl/common/counter.sv
rtl/common/fifo.sv
rtl/common/pipeline_reg.sv
rtl/common/skid_buffer.sv
rtl/common/arbiter_rr.sv
rtl/common/priority_encoder.sv
rtl/common/onehot_mux.sv

// --- Platform wrappers ------------------------------------------------------
rtl/platform/mem_macro_wrap.sv
rtl/platform/reset_sync.sv
rtl/platform/clock_gate_wrap.sv

// --- Control ----------------------------------------------------------------
rtl/control/npu_reset_ctrl.sv
rtl/control/npu_irq_ctrl.sv
rtl/control/npu_feature_id.sv
rtl/control/npu_perf_counters.sv
rtl/control/npu_reg_block.sv
rtl/control/npu_cmd_fetch.sv
rtl/control/npu_cmd_decode.sv
rtl/control/npu_queue.sv

// --- Memory -----------------------------------------------------------------
rtl/memory/npu_local_mem_wrap.sv
rtl/memory/npu_weight_buffer.sv
rtl/memory/npu_act_buffer.sv
rtl/memory/npu_psum_buffer.sv
rtl/memory/npu_buffer_router.sv
rtl/memory/npu_mem_top.sv
rtl/memory/npu_dma_reader.sv
rtl/memory/npu_dma_writer.sv
rtl/memory/npu_dma_frontend.sv
rtl/memory/npu_scratchpad.sv

// --- Ops: reusable primitives -----------------------------------------------
rtl/ops/postproc/bias_add.sv
rtl/ops/activation/activation.sv
rtl/ops/postproc/quantize.sv
rtl/ops/postproc/postproc.sv

// --- Vec backend ------------------------------------------------------------
rtl/backends/vec/vec_ctrl.sv
rtl/backends/vec/vec_backend.sv

// --- Composites: pooling ----------------------------------------------------
rtl/composites/pooling/pool_ctrl.sv
rtl/composites/pooling/pool_composite.sv

// --- Composites: lnorm ------------------------------------------------------
rtl/composites/lnorm/lnorm_ctrl.sv
rtl/composites/lnorm/lnorm_composite.sv

// --- Composites: softmax ----------------------------------------------------
rtl/composites/softmax/softmax_exp_lut.sv
rtl/composites/softmax/softmax_ctrl.sv
rtl/composites/softmax/softmax_composite.sv

// --- GEMM backend -----------------------------------------------------------
rtl/backends/gemm/gemm_addr_gen.sv
rtl/backends/gemm/gemm_ctrl.sv
rtl/backends/gemm/gemm_backend.sv

// --- Convolution backend ----------------------------------------------------
rtl/backends/conv/conv_pe.sv
rtl/backends/conv/conv_array.sv
rtl/backends/conv/conv_accum.sv
rtl/backends/conv/conv_addr_gen.sv
rtl/backends/conv/conv_loader.sv
rtl/backends/conv/conv_line_buffer.sv
rtl/backends/conv/conv_window_gen.sv
rtl/backends/conv/conv_writer.sv
rtl/backends/conv/conv_ctrl.sv
rtl/backends/conv/conv_backend.sv

// --- Core -------------------------------------------------------------------
rtl/core/npu_status.sv
rtl/core/npu_completion.sv
rtl/core/npu_scheduler.sv
rtl/core/npu_dispatch.sv
rtl/core/npu_core.sv
rtl/core/npu_shell.sv
