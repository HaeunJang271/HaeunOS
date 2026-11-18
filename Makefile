CC ?= cc
CFLAGS ?= -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror
CPPFLAGS ?= -Iinclude
BUILD := build
ifeq ($(OS),Windows_NT)
VM_BIN := $(BUILD)/haeun-vm.exe
VM_TEST_BIN := $(BUILD)/vm-tests.exe
HXF_TEST_BIN := $(BUILD)/hxf-tests.exe
PYTHON ?= python
else
VM_BIN := $(BUILD)/haeun-vm
VM_TEST_BIN := $(BUILD)/vm-tests
HXF_TEST_BIN := $(BUILD)/hxf-tests
PYTHON ?= python3
endif

.PHONY: all test toolchain-test clean release images sdk
.DEFAULT_GOAL := all

ifeq ($(OS),Windows_NT)
$(BUILD):
	pwsh -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(BUILD)' | Out-Null"
clean:
	pwsh -NoProfile -Command "if (Test-Path '$(BUILD)') { Remove-Item -LiteralPath '$(BUILD)' -Recurse -Force }"
else
$(BUILD):
	mkdir -p $(BUILD)
clean:
	rm -rf $(BUILD)
endif

VM_CORE := src/vm.c src/board_mmio.c src/board_extra.c src/board_net.c src/trap.c src/mmu.c src/tlb.c src/pt.c \
	src/register_file.c src/cpu_state.c src/memory_interface.c \
	src/instruction_decoder.c src/instruction_executor.c src/instruction_trace.c \
	src/sec_core.c src/sec_dispatch.c src/sec_ecall.c
HFS_SRCS := src/hfs_io.c src/hfs_mkfs.c src/hfs_ops.c src/hfs_fsck.c src/proc_hfs.c src/proc_hfs_fs.c
GFX_SRCS := src/gfx_fb.c src/gfx_pixel.c src/gfx_font.c src/gfx_wm.c \
	src/gfx_wm_fill.c src/gfx_wm_state.c src/gfx_comp.c src/gfx_ui.c \
	src/gfx_desk.c src/gfx_ecall.c src/gfx_chrome.c src/gfx_chrome_ptr.c \
	src/guest_pcb.c src/telem_inspect.c src/evt.c \
	src/pval_snapshot.c src/pval_invariants.c src/pval_timestamp.c \
	src/pval_compat.c src/pval_report.c src/pval_validate.c \
	src/reconfig_fi.c src/reconfig_txn.c src/reconfig_report.c \
	src/reconfig_eval.c src/reconfig_cli.c src/eval_metrics.c \
	src/planner_types.c src/planner_monitor.c src/planner_analyze.c \
	src/planner_decide.c src/planner_safety.c src/planner_run.c \
	src/planner_report.c src/planner_cli.c \
	src/feedback_types.c src/feedback_metrics.c src/feedback_compare.c \
	src/feedback_controller.c src/feedback_state.c src/feedback_report.c \
	src/feedback_cli.c src/eval_fairness.c
NET_SRCS := src/net_eth.c src/net_arp.c src/net_ipv4.c src/net_udp.c src/net_tcp.c src/net_dns.c src/net_sock.c src/net_drv.c
NET_SOCK_ECALL_SRC := src/net_sock_ecall.c
ifeq ($(OS),Windows_NT)
NET_BACKEND_SRC := src/net_backend_win32.c
NET_BACKEND_LIBS := -lws2_32
else
NET_BACKEND_SRC := src/net_backend_posix.c
NET_BACKEND_LIBS :=
endif

APP_SRCS := src/app_env.c src/app_env_query.c src/app_ecall.c
SM_SRCS := src/svc_mgr.c src/svc_mgr_life.c src/svc_mgr_ecall.c
M20_SRCS := src/ipc.c src/svc.c src/m20_ecall.c $(SM_SRCS)
M7_SRCS := src/fb_dump.c src/ecall_mux.c src/vm_gui_host.c \
	src/vm_net_host.c src/vm_input_host.c src/vm_input_m22_scripts.c \
	src/fb_view.c $(GFX_SRCS) \
	src/proc_ecall_io.c $(HFS_SRCS) $(NET_SOCK_ECALL_SRC) $(NET_SRCS) \
	$(NET_BACKEND_SRC) $(APP_SRCS) $(M20_SRCS)
ifeq ($(OS),Windows_NT)
VIEWER_SRC := src/vm_viewer_win32.c src/vm_viewer_blit.c
VIEWER_LIBS := -luser32 -lgdi32
else
VIEWER_SRC := src/vm_viewer_stub.c
VIEWER_LIBS :=
endif
VM_SRCS := src/main.c $(VM_CORE) src/hxf.c src/hfw.c src/bootinfo.c src/disk_persist.c \
	src/sha256.c src/sec_boot.c $(M7_SRCS) $(VIEWER_SRC) \
	src/opt_types.c src/opt_stats.c src/opt_objectives.c src/opt_pareto.c \
	src/opt_select.c src/opt_state.c src/opt_report.c src/opt_apply.c \
	src/opt_cli.c src/opt_experiment.c src/eval_stats.c \
	src/eval_workload.c src/eval_runner.c src/eval_cost.c \
	src/eval_report.c src/eval_compare.c \
	src/robust_types.c src/robust_replay.c src/robust_runner.c \
	src/robust_sensitivity.c src/robust_reconfig.c src/robust_verify.c \
	src/robust_report.c src/robust_cli.c
VM_HDRS := include/haeun/vm.h include/haeun/board_mmio.h include/haeun/board_extra.h include/haeun/trap.h \
	include/haeun/exception_handler.h include/haeun/mmu.h include/haeun/tlb.h \
	include/haeun/pt.h include/haeun/register_file.h include/haeun/cpu_state.h \
	include/haeun/memory_interface.h include/haeun/instruction_decoder.h \
	include/haeun/instruction_executor.h include/haeun/instruction_trace.h \
	include/haeun/hxf.h include/haeun/hfw.h include/haeun/bootinfo.h \
	include/haeun/disk_persist.h include/haeun/fb_dump.h include/haeun/ecall_mux.h \
	include/haeun/vm_gui_host.h include/haeun/vm_net_host.h include/haeun/net_backend.h \
	include/haeun/vm_input_host.h include/haeun/vm_viewer.h include/haeun/fb_view.h \
	include/haeun/guest_pcb.h include/haeun/telem_inspect.h include/haeun/app_env.h \
	include/haeun/pval_types.h include/haeun/pval_snapshot.h include/haeun/pval_invariants.h \
	include/haeun/pval_timestamp.h include/haeun/pval_compat.h include/haeun/pval_report.h \
	include/haeun/pval_validate.h include/haeun/reconfig.h include/haeun/reconfig_cli.h \
	include/haeun/reconfig_eval.h include/haeun/planner.h include/haeun/planner_types.h \
	include/haeun/eval_metrics.h include/haeun/eval_types.h \
	include/haeun/feedback.h include/haeun/feedback_types.h \
	include/haeun/feedback_experiment.h \
	include/haeun/optimize.h include/haeun/opt_types.h \
	include/haeun/robust.h include/haeun/robust_types.h
$(VM_BIN): $(VM_SRCS) $(VM_HDRS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(VM_SRCS) -o $@ $(VIEWER_LIBS) $(NET_BACKEND_LIBS)

M7_TEST_BIN := $(BUILD)/m7-integration-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(M7_TEST_BIN): tests/m7_integration_tests.c $(M7_SRCS) $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/m7_integration_tests.c $(M7_SRCS) $(VM_CORE) -o $@ $(NET_BACKEND_LIBS)

M8_PCB_PROBE_BIN := $(BUILD)/m8-pcb-probe$(if $(filter Windows_NT,$(OS)),.exe,)
$(M8_PCB_PROBE_BIN): tests/m8_pcb_probe.c $(M7_SRCS) $(VM_CORE) src/hxf.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/m8_pcb_probe.c $(M7_SRCS) $(VM_CORE) src/hxf.c -o $@ $(NET_BACKEND_LIBS)

M9_NET_PROBE_BIN := $(BUILD)/m9-net-probe$(if $(filter Windows_NT,$(OS)),.exe,)
$(M9_NET_PROBE_BIN): tests/m9_net_probe.c $(M7_SRCS) $(VM_CORE) src/hxf.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/m9_net_probe.c $(M7_SRCS) $(VM_CORE) src/hxf.c -o $@ $(NET_BACKEND_LIBS)

M13_NET_BE_TEST_BIN := $(BUILD)/m13-net-backend-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(M13_NET_BE_TEST_BIN): tests/m13_net_backend_tests.c $(NET_BACKEND_SRC) src/net_eth.c src/net_ipv4.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/m13_net_backend_tests.c $(NET_BACKEND_SRC) src/net_eth.c src/net_ipv4.c -o $@ $(NET_BACKEND_LIBS)

M14_APP_TEST_BIN := $(BUILD)/m14-app-env-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(M14_APP_TEST_BIN): tests/m14_app_env_tests.c $(APP_SRCS) $(GFX_SRCS) $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/m14_app_env_tests.c $(APP_SRCS) $(GFX_SRCS) $(VM_CORE) -o $@

M15_ISO_TEST_BIN := $(BUILD)/m15-isolation-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(M15_ISO_TEST_BIN): tests/m15_isolation_tests.c $(APP_SRCS) $(GFX_SRCS) $(NET_SRCS) $(HFS_SRCS) $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/m15_isolation_tests.c $(APP_SRCS) $(GFX_SRCS) $(NET_SRCS) $(HFS_SRCS) $(VM_CORE) -o $@

M16_REL_TEST_BIN := $(BUILD)/m16-reliability-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(M16_REL_TEST_BIN): tests/m16_reliability_tests.c $(APP_SRCS) $(GFX_SRCS) $(NET_SRCS) $(HFS_SRCS) $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/m16_reliability_tests.c $(APP_SRCS) $(GFX_SRCS) $(NET_SRCS) $(HFS_SRCS) $(VM_CORE) -o $@

M17_FAULT_TEST_BIN := $(BUILD)/m17-fault-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(M17_FAULT_TEST_BIN): tests/m17_fault_tests.c src/ecall_mux.c $(APP_SRCS) $(GFX_SRCS) $(NET_SRCS) $(HFS_SRCS) $(NET_SOCK_ECALL_SRC) src/proc_ecall_io.c $(M20_SRCS) $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/m17_fault_tests.c src/ecall_mux.c $(APP_SRCS) $(GFX_SRCS) $(NET_SRCS) $(HFS_SRCS) $(NET_SOCK_ECALL_SRC) src/proc_ecall_io.c $(M20_SRCS) $(VM_CORE) -o $@

M20_UNIT_TEST_BIN := $(BUILD)/m20-unit-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(M20_UNIT_TEST_BIN): tests/m20_unit_tests.c src/evt.c src/ipc.c src/svc.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/m20_unit_tests.c src/evt.c src/ipc.c src/svc.c -o $@

M20_RES_TEST_BIN := $(BUILD)/m20-resource-integrity-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(M20_RES_TEST_BIN): tests/m20_resource_integrity_tests.c $(GFX_SRCS) $(NET_SRCS) $(NET_SOCK_ECALL_SRC) $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/m20_resource_integrity_tests.c $(GFX_SRCS) $(NET_SRCS) $(NET_SOCK_ECALL_SRC) $(VM_CORE) -o $@

M20_INPUT_TEST_BIN := $(BUILD)/m20-input-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(M20_INPUT_TEST_BIN): tests/m20_input_tests.c src/m20_ecall.c src/ipc.c src/svc.c $(GFX_SRCS) $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/m20_input_tests.c src/m20_ecall.c src/ipc.c src/svc.c $(GFX_SRCS) $(VM_CORE) -o $@

M20_IPC_TEST_BIN := $(BUILD)/m20-ipc-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(M20_IPC_TEST_BIN): tests/m20_ipc_tests.c src/m20_ecall.c src/ipc.c src/svc.c $(GFX_SRCS) $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/m20_ipc_tests.c src/m20_ecall.c src/ipc.c src/svc.c $(GFX_SRCS) $(VM_CORE) -o $@

M20_TEXT_TEST_BIN := $(BUILD)/m20-text-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(M20_TEXT_TEST_BIN): tests/m20_text_tests.c src/m20_ecall.c src/ipc.c src/svc.c $(GFX_SRCS) $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/m20_text_tests.c src/m20_ecall.c src/ipc.c src/svc.c $(GFX_SRCS) $(VM_CORE) -o $@

M20_WIN_TEST_BIN := $(BUILD)/m20-window-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(M20_WIN_TEST_BIN): tests/m20_window_tests.c src/m20_ecall.c src/ipc.c src/svc.c $(GFX_SRCS) $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/m20_window_tests.c src/m20_ecall.c src/ipc.c src/svc.c $(GFX_SRCS) $(VM_CORE) -o $@

M20_LAUNCH_TEST_BIN := $(BUILD)/m20-launcher-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(M20_LAUNCH_TEST_BIN): tests/m20_launcher_tests.c $(APP_SRCS) $(GFX_SRCS) $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/m20_launcher_tests.c $(APP_SRCS) $(GFX_SRCS) $(VM_CORE) -o $@

M20_SM_TEST_BIN := $(BUILD)/m20-service-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(M20_SM_TEST_BIN): tests/m20_service_tests.c $(APP_SRCS) $(SM_SRCS) $(GFX_SRCS) $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/m20_service_tests.c $(APP_SRCS) $(SM_SRCS) $(GFX_SRCS) $(VM_CORE) -o $@

M20_FILE_TEST_BIN := $(BUILD)/m20-file-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(M20_FILE_TEST_BIN): tests/m20_file_tests.c $(HFS_SRCS) $(VM_CORE) src/guest_pcb.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/m20_file_tests.c $(HFS_SRCS) $(VM_CORE) src/guest_pcb.c src/proc_ecall_io.c -o $@

M20_INTEG_TEST_BIN := $(BUILD)/m20-integration-tests$(if $(filter Windows_NT,$(OS)),.exe,)
M20_INTEG_SRCS := tests/m20_integration_tests.c tests/m20_integration_harness.c \
	tests/m20_integration_core.c tests/m20_integration_sec.c
$(M20_INTEG_TEST_BIN): $(M20_INTEG_SRCS) $(APP_SRCS) $(SM_SRCS) $(GFX_SRCS) $(HFS_SRCS) $(NET_SRCS) src/ipc.c $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Itests $(M20_INTEG_SRCS) $(APP_SRCS) $(SM_SRCS) $(GFX_SRCS) $(HFS_SRCS) $(NET_SRCS) src/ipc.c $(VM_CORE) -o $@

HFS_TEST_BIN := $(BUILD)/hfs-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(HFS_TEST_BIN): tests/hfs_tests.c src/hfs_io.c src/hfs_mkfs.c src/hfs_ops.c src/hfs_fsck.c include/haeun/hfs.h include/haeun/hfs_internal.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/hfs_tests.c src/hfs_io.c src/hfs_mkfs.c src/hfs_ops.c src/hfs_fsck.c -o $@

PROC_HFS_TEST_BIN := $(BUILD)/proc-hfs-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(PROC_HFS_TEST_BIN): tests/proc_hfs_tests.c $(HFS_SRCS) include/haeun/proc_hfs.h include/haeun/hfs.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/proc_hfs_tests.c $(HFS_SRCS) -o $@

PROC_ECALL_IO_TEST_BIN := $(BUILD)/proc-ecall-io-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(PROC_ECALL_IO_TEST_BIN): tests/proc_ecall_io_tests.c src/proc_ecall_io.c src/guest_pcb.c $(HFS_SRCS) $(VM_CORE) include/haeun/proc_hfs.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/proc_ecall_io_tests.c src/proc_ecall_io.c src/guest_pcb.c $(HFS_SRCS) $(VM_CORE) -o $@
# guest_pcb kept explicit here (not via GFX_SRCS) for a minimal HFS ECALL link set.

GFX_TEST_BIN := $(BUILD)/gfx-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(GFX_TEST_BIN): tests/gfx_tests.c $(GFX_SRCS) $(VM_CORE) include/haeun/gfx.h include/haeun/gfx_wm.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/gfx_tests.c $(GFX_SRCS) $(VM_CORE) -o $@

GFX_WM_TEST_BIN := $(BUILD)/gfx-wm-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(GFX_WM_TEST_BIN): tests/gfx_wm_tests.c $(GFX_SRCS) $(VM_CORE) include/haeun/gfx.h include/haeun/gfx_wm.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/gfx_wm_tests.c $(GFX_SRCS) $(VM_CORE) -o $@

M23_3_STATE_TEST_BIN := $(BUILD)/m23-3-window-state-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(M23_3_STATE_TEST_BIN): tests/m23_3_window_state_tests.c $(GFX_SRCS) $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/m23_3_window_state_tests.c $(GFX_SRCS) $(VM_CORE) -o $@

M23_4_RES_TEST_BIN := $(BUILD)/m23-4-resource-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(M23_4_RES_TEST_BIN): tests/m23_4_resource_tests.c $(GFX_SRCS) $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/m23_4_resource_tests.c $(GFX_SRCS) $(VM_CORE) -o $@

M24_2_EXIT_TEST_BIN := $(BUILD)/m24-2-exit-idempotency-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(M24_2_EXIT_TEST_BIN): tests/m24_2_exit_idempotency_tests.c src/ecall_mux.c $(APP_SRCS) $(GFX_SRCS) $(NET_SRCS) $(HFS_SRCS) $(NET_SOCK_ECALL_SRC) src/proc_ecall_io.c $(M20_SRCS) $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/m24_2_exit_idempotency_tests.c src/ecall_mux.c $(APP_SRCS) $(GFX_SRCS) $(NET_SRCS) $(HFS_SRCS) $(NET_SOCK_ECALL_SRC) src/proc_ecall_io.c $(M20_SRCS) $(VM_CORE) -o $@

M24_2_CAP_TEST_BIN := $(BUILD)/m24-2-process-capacity-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(M24_2_CAP_TEST_BIN): tests/m24_2_process_capacity_tests.c $(APP_SRCS) $(GFX_SRCS) $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/m24_2_process_capacity_tests.c $(APP_SRCS) $(GFX_SRCS) $(VM_CORE) -o $@

M24_2_FAULT_RES_TEST_BIN := $(BUILD)/m24-2-fault-resource-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(M24_2_FAULT_RES_TEST_BIN): tests/m24_2_fault_resource_tests.c src/ecall_mux.c $(APP_SRCS) $(GFX_SRCS) $(NET_SRCS) $(HFS_SRCS) $(NET_SOCK_ECALL_SRC) src/proc_ecall_io.c $(M20_SRCS) $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/m24_2_fault_resource_tests.c src/ecall_mux.c $(APP_SRCS) $(GFX_SRCS) $(NET_SRCS) $(HFS_SRCS) $(NET_SOCK_ECALL_SRC) src/proc_ecall_io.c $(M20_SRCS) $(VM_CORE) -o $@

M24_4_RES_TEST_BIN := $(BUILD)/m24-4-resource-reclamation-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(M24_4_RES_TEST_BIN): tests/m24_4_resource_reclamation_tests.c src/ecall_mux.c $(APP_SRCS) $(GFX_SRCS) $(NET_SRCS) $(HFS_SRCS) $(NET_SOCK_ECALL_SRC) src/proc_ecall_io.c $(M20_SRCS) $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/m24_4_resource_reclamation_tests.c src/ecall_mux.c $(APP_SRCS) $(GFX_SRCS) $(NET_SRCS) $(HFS_SRCS) $(NET_SOCK_ECALL_SRC) src/proc_ecall_io.c $(M20_SRCS) $(VM_CORE) -o $@

M25_2_OWN_TEST_BIN := $(BUILD)/m25-2-as-ownership-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(M25_2_OWN_TEST_BIN): tests/m25_2_as_ownership_tests.c $(APP_SRCS) $(GFX_SRCS) $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/m25_2_as_ownership_tests.c $(APP_SRCS) $(GFX_SRCS) $(VM_CORE) -o $@

M25_2_SCRUB_TEST_BIN := $(BUILD)/m25-2-memory-scrub-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(M25_2_SCRUB_TEST_BIN): tests/m25_2_memory_scrub_tests.c $(APP_SRCS) $(GFX_SRCS) $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/m25_2_memory_scrub_tests.c $(APP_SRCS) $(GFX_SRCS) $(VM_CORE) -o $@

M25_6_HEAP_HOST_BIN := $(BUILD)/m25-6-heap-host-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(M25_6_HEAP_HOST_BIN): tests/m25_6_heap_host_tests.c $(APP_SRCS) $(GFX_SRCS) $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/m25_6_heap_host_tests.c $(APP_SRCS) $(GFX_SRCS) $(VM_CORE) -o $@

GFX_UI_TEST_BIN := $(BUILD)/gfx-ui-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(GFX_UI_TEST_BIN): tests/gfx_ui_tests.c $(GFX_SRCS) $(VM_CORE) include/haeun/gfx.h include/haeun/gfx_wm.h include/haeun/gfx_ui.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/gfx_ui_tests.c $(GFX_SRCS) $(VM_CORE) -o $@

GFX_DESK_TEST_BIN := $(BUILD)/gfx-desk-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(GFX_DESK_TEST_BIN): tests/gfx_desk_tests.c $(GFX_SRCS) $(VM_CORE) include/haeun/gfx_desk.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/gfx_desk_tests.c $(GFX_SRCS) $(VM_CORE) -o $@

GFX_CHROME_TEST_BIN := $(BUILD)/gfx-chrome-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(GFX_CHROME_TEST_BIN): tests/gfx_chrome_tests.c $(GFX_SRCS) $(VM_CORE) include/haeun/gfx_chrome.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/gfx_chrome_tests.c $(GFX_SRCS) $(VM_CORE) -o $@

GFX_ECALL_TEST_BIN := $(BUILD)/gfx-ecall-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(GFX_ECALL_TEST_BIN): tests/gfx_ecall_tests.c $(GFX_SRCS) $(VM_CORE) include/haeun/gfx_ecall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/gfx_ecall_tests.c $(GFX_SRCS) $(VM_CORE) -o $@

NET_TEST_BIN := $(BUILD)/net-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(NET_TEST_BIN): tests/net_tests.c $(NET_SRCS) $(VM_CORE) include/haeun/net.h include/haeun/board_net.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/net_tests.c $(NET_SRCS) $(VM_CORE) -o $@

NET_ICMP_TEST_BIN := $(BUILD)/net-icmp-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(NET_ICMP_TEST_BIN): tests/net_icmp_tests.c $(NET_SRCS) $(VM_CORE) include/haeun/net.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/net_icmp_tests.c $(NET_SRCS) $(VM_CORE) -o $@

NET_UDP_TEST_BIN := $(BUILD)/net-udp-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(NET_UDP_TEST_BIN): tests/net_udp_tests.c $(NET_SRCS) $(VM_CORE) include/haeun/net.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/net_udp_tests.c $(NET_SRCS) $(VM_CORE) -o $@

NET_TCP_TEST_BIN := $(BUILD)/net-tcp-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(NET_TCP_TEST_BIN): tests/net_tcp_tests.c $(NET_SRCS) $(VM_CORE) include/haeun/net.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/net_tcp_tests.c $(NET_SRCS) $(VM_CORE) -o $@

NET_DNS_TEST_BIN := $(BUILD)/net-dns-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(NET_DNS_TEST_BIN): tests/net_dns_tests.c $(NET_SRCS) $(VM_CORE) include/haeun/net.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/net_dns_tests.c $(NET_SRCS) $(VM_CORE) -o $@

NET_SOCK_TEST_BIN := $(BUILD)/net-sock-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(NET_SOCK_TEST_BIN): tests/net_sock_tests.c $(NET_SRCS) $(VM_CORE) include/haeun/net_sock.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/net_sock_tests.c $(NET_SRCS) $(VM_CORE) -o $@

NET_SOCK_ECALL_TEST_BIN := $(BUILD)/net-sock-ecall-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(NET_SOCK_ECALL_TEST_BIN): tests/net_sock_ecall_tests.c $(NET_SOCK_ECALL_SRC) src/guest_pcb.c $(NET_SRCS) $(VM_CORE) include/haeun/net_sock_ecall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/net_sock_ecall_tests.c $(NET_SOCK_ECALL_SRC) src/guest_pcb.c $(NET_SRCS) $(VM_CORE) -o $@

SEC_TEST_BIN := $(BUILD)/sec-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(SEC_TEST_BIN): tests/sec_tests.c src/hxf.c $(VM_CORE) include/haeun/sec.h include/haeun/hxf.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/sec_tests.c src/hxf.c $(VM_CORE) -o $@

SEC_DISPATCH_TEST_BIN := $(BUILD)/sec-dispatch-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(SEC_DISPATCH_TEST_BIN): tests/sec_dispatch_tests.c src/hxf.c $(VM_CORE) include/haeun/sec.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/sec_dispatch_tests.c src/hxf.c $(VM_CORE) -o $@

SEC_ECALL_TEST_BIN := $(BUILD)/sec-ecall-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(SEC_ECALL_TEST_BIN): tests/sec_ecall_tests.c $(VM_CORE) include/haeun/sec.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/sec_ecall_tests.c $(VM_CORE) -o $@

SEC_BOOT_SRCS := src/sha256.c src/sec_boot.c src/sec_core.c
SEC_BOOT_TEST_BIN := $(BUILD)/sec-boot-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(SEC_BOOT_TEST_BIN): tests/sec_boot_tests.c $(SEC_BOOT_SRCS) src/hxf.c include/haeun/sec_boot.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/sec_boot_tests.c $(SEC_BOOT_SRCS) src/hxf.c -o $@












MKFS_BIN := $(BUILD)/haeun-mkfs$(if $(filter Windows_NT,$(OS)),.exe,)
$(MKFS_BIN): tools/haeun_mkfs.c $(HFS_SRCS) include/haeun/hfs.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tools/haeun_mkfs.c $(HFS_SRCS) -o $@

FSCK_BIN := $(BUILD)/haeun-fsck$(if $(filter Windows_NT,$(OS)),.exe,)
$(FSCK_BIN): tools/haeun_fsck.c $(HFS_SRCS) include/haeun/hfs.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tools/haeun_fsck.c $(HFS_SRCS) -o $@

all: $(VM_BIN) $(MKFS_BIN) $(FSCK_BIN)

images: all
	$(PYTHON) tools/package_release.py --images-only

release: all
	$(PYTHON) tools/package_release.py

sdk: all
	$(PYTHON) tools/package_sdk.py

$(VM_TEST_BIN): tests/vm_tests.c $(VM_CORE) include/haeun/vm.h include/haeun/board_mmio.h include/haeun/trap.h include/haeun/mmu.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/vm_tests.c $(VM_CORE) -o $@

$(HXF_TEST_BIN): tests/hxf_tests.c src/hxf.c $(VM_CORE) include/haeun/hxf.h include/haeun/vm.h include/haeun/board_mmio.h include/haeun/trap.h include/haeun/mmu.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/hxf_tests.c src/hxf.c $(VM_CORE) -o $@

BOOTINFO_TEST_BIN := $(BUILD)/bootinfo-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(BOOTINFO_TEST_BIN): tests/bootinfo_tests.c src/bootinfo.c include/haeun/bootinfo.h include/haeun/vm.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/bootinfo_tests.c src/bootinfo.c -o $@

HFW_TEST_BIN := $(BUILD)/hfw-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(HFW_TEST_BIN): tests/hfw_tests.c src/hfw.c include/haeun/hfw.h include/haeun/vm.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/hfw_tests.c src/hfw.c -o $@

DEVICE_TEST_BIN := $(BUILD)/device-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(DEVICE_TEST_BIN): tests/device_tests.c $(VM_CORE) include/haeun/vm.h include/haeun/board_mmio.h include/haeun/trap.h include/haeun/mmu.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/device_tests.c $(VM_CORE) -o $@

ISA4_TEST_BIN := $(BUILD)/isa4-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(ISA4_TEST_BIN): tests/isa4_tests.c $(VM_CORE) include/haeun/vm.h include/haeun/board_mmio.h include/haeun/trap.h include/haeun/mmu.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/isa4_tests.c $(VM_CORE) -o $@

MMU_TEST_BIN := $(BUILD)/mmu-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(MMU_TEST_BIN): tests/mmu_tests.c $(VM_CORE) include/haeun/vm.h include/haeun/mmu.h include/haeun/board_mmio.h include/haeun/trap.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/mmu_tests.c $(VM_CORE) -o $@

PT_TEST_BIN := $(BUILD)/pt-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(PT_TEST_BIN): tests/pt_tests.c $(VM_CORE) include/haeun/pt.h include/haeun/mmu.h include/haeun/vm.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/pt_tests.c $(VM_CORE) -o $@

KERNEL_PAGE_TEST_BIN := $(BUILD)/kernel-page-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(KERNEL_PAGE_TEST_BIN): tests/kernel_page_tests.c $(VM_CORE) include/haeun/pt.h include/haeun/mmu.h include/haeun/vm.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/kernel_page_tests.c $(VM_CORE) -o $@

TLB_TEST_BIN := $(BUILD)/tlb-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(TLB_TEST_BIN): tests/tlb_tests.c $(VM_CORE) include/haeun/tlb.h include/haeun/mmu.h include/haeun/vm.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/tlb_tests.c $(VM_CORE) -o $@

CPU_MODULE_TEST_BIN := $(BUILD)/cpu-module-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(CPU_MODULE_TEST_BIN): tests/cpu_module_tests.c $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/cpu_module_tests.c $(VM_CORE) -o $@

OPCODE_MATRIX_TEST_BIN := $(BUILD)/opcode-matrix-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(OPCODE_MATRIX_TEST_BIN): tests/opcode_matrix_tests.c $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/opcode_matrix_tests.c $(VM_CORE) -o $@

BOARD_EXTRA_TEST_BIN := $(BUILD)/board-extra-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(BOARD_EXTRA_TEST_BIN): tests/board_extra_tests.c $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/board_extra_tests.c $(VM_CORE) -o $@

FB_VIEW_TEST_BIN := $(BUILD)/fb-view-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(FB_VIEW_TEST_BIN): tests/fb_view_tests.c src/fb_view.c $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/fb_view_tests.c src/fb_view.c $(VM_CORE) -o $@

isa-opcodes:
	$(PYTHON) tools/gen_isa_opcodes.py

isa-opcodes-check:
	$(PYTHON) tools/gen_isa_opcodes.py --check

toolchain-test: $(VM_BIN) isa-opcodes-check
	$(PYTHON) tests/toolchain_tests.py $(VM_BIN)

R11_SCHED_BIN := $(BUILD)/r11-sched-tests$(if $(filter Windows_NT,$(OS)),.exe,)
R11_TIMER_BIN := $(BUILD)/r11-timer-tests$(if $(filter Windows_NT,$(OS)),.exe,)
R12_TELEM_BIN := $(BUILD)/r12-telem-tests$(if $(filter Windows_NT,$(OS)),.exe,)
R13_TELEM_BIN := $(BUILD)/r13-telem-inspect-tests$(if $(filter Windows_NT,$(OS)),.exe,)
R20_POLICY_BIN := $(BUILD)/r20-policy-tests$(if $(filter Windows_NT,$(OS)),.exe,)
R21_DET_BIN := $(BUILD)/r21-policy-determinism-tests$(if $(filter Windows_NT,$(OS)),.exe,)
R21_GUEST_BIN := $(BUILD)/r21-policy-guest-tests$(if $(filter Windows_NT,$(OS)),.exe,)
EVAL_SRCS := src/eval_stats.c src/eval_metrics.c src/eval_fairness.c src/eval_cost.c \
	src/eval_workload.c src/eval_runner.c src/eval_report.c src/eval_compare.c
R22_UNIT_BIN := $(BUILD)/r22-eval-unit-tests$(if $(filter Windows_NT,$(OS)),.exe,)
R22_RUN_BIN := $(BUILD)/r22-eval-runner-tests$(if $(filter Windows_NT,$(OS)),.exe,)
HAEUN_EVAL_BIN := $(BUILD)/haeun-eval$(if $(filter Windows_NT,$(OS)),.exe,)
R30_UNIT_BIN := $(BUILD)/r30-fcfs-unit-tests$(if $(filter Windows_NT,$(OS)),.exe,)
R30_GUEST_BIN := $(BUILD)/r30-fcfs-guest-tests$(if $(filter Windows_NT,$(OS)),.exe,)
PVAL_SRCS := src/pval_snapshot.c src/pval_invariants.c src/pval_timestamp.c \
	src/pval_compat.c src/pval_report.c src/pval_validate.c
RECFG_SRCS := src/reconfig_fi.c src/reconfig_txn.c src/reconfig_report.c \
	src/reconfig_eval.c src/reconfig_cli.c
R31_UNIT_BIN := $(BUILD)/r31-pval-unit-tests$(if $(filter Windows_NT,$(OS)),.exe,)
R31_GUEST_BIN := $(BUILD)/r31-pval-guest-tests$(if $(filter Windows_NT,$(OS)),.exe,)
R32_UNIT_BIN := $(BUILD)/r32-reconfig-unit-tests$(if $(filter Windows_NT,$(OS)),.exe,)
R32_GUEST_BIN := $(BUILD)/r32-reconfig-guest-tests$(if $(filter Windows_NT,$(OS)),.exe,)
PLANNER_SRCS := src/planner_types.c src/planner_monitor.c src/planner_analyze.c \
	src/planner_decide.c src/planner_safety.c src/planner_run.c \
	src/planner_report.c src/planner_cli.c src/eval_metrics.c src/telem_inspect.c
R33_UNIT_BIN := $(BUILD)/r33-planner-unit-tests$(if $(filter Windows_NT,$(OS)),.exe,)
R33_GUEST_BIN := $(BUILD)/r33-planner-guest-tests$(if $(filter Windows_NT,$(OS)),.exe,)
R34_UNIT_BIN := $(BUILD)/r34-active-unit-tests$(if $(filter Windows_NT,$(OS)),.exe,)
R34_GUEST_BIN := $(BUILD)/r34-active-guest-tests$(if $(filter Windows_NT,$(OS)),.exe,)
FEEDBACK_CORE := src/feedback_types.c src/feedback_metrics.c src/feedback_compare.c \
	src/feedback_controller.c src/feedback_state.c src/feedback_report.c \
	src/feedback_cli.c src/eval_fairness.c
FEEDBACK_SRCS := $(FEEDBACK_CORE) src/feedback_experiment.c src/eval_stats.c \
	src/eval_workload.c src/eval_runner.c src/eval_cost.c src/eval_report.c \
	src/eval_compare.c
R35_UNIT_BIN := $(BUILD)/r35-feedback-unit-tests$(if $(filter Windows_NT,$(OS)),.exe,)
R35_GUEST_BIN := $(BUILD)/r35-feedback-guest-tests$(if $(filter Windows_NT,$(OS)),.exe,)
OPT_CORE := src/opt_types.c src/opt_stats.c src/opt_objectives.c src/opt_pareto.c \
	src/opt_select.c src/opt_state.c src/opt_report.c src/eval_stats.c
OPT_SRCS := $(OPT_CORE) src/opt_experiment.c src/opt_apply.c src/opt_cli.c \
	src/eval_fairness.c src/eval_workload.c src/eval_runner.c src/eval_cost.c \
	src/eval_report.c src/eval_compare.c
R36_UNIT_BIN := $(BUILD)/r36-opt-unit-tests$(if $(filter Windows_NT,$(OS)),.exe,)
R36_GUEST_BIN := $(BUILD)/r36-opt-guest-tests$(if $(filter Windows_NT,$(OS)),.exe,)
RB_CORE := src/robust_types.c src/robust_replay.c src/robust_report.c
RB_SRCS := $(RB_CORE) src/robust_runner.c src/robust_sensitivity.c \
	src/robust_reconfig.c src/robust_verify.c src/robust_cli.c
R37_UNIT_BIN := $(BUILD)/r37-robust-unit-tests$(if $(filter Windows_NT,$(OS)),.exe,)
R37_GUEST_BIN := $(BUILD)/r37-robust-guest-tests$(if $(filter Windows_NT,$(OS)),.exe,)
R40_INTEG_BIN := $(BUILD)/r40-integration-tests$(if $(filter Windows_NT,$(OS)),.exe,)
R41_VAL_BIN := $(BUILD)/r41-validation-tests$(if $(filter Windows_NT,$(OS)),.exe,)
R42_VAL_BIN := $(BUILD)/r42-validation-tests$(if $(filter Windows_NT,$(OS)),.exe,)
R43_VAL_BIN := $(BUILD)/r43-active-verification-tests$(if $(filter Windows_NT,$(OS)),.exe,)
$(R11_SCHED_BIN): tests/r11_sched_tests.c src/hxf.c $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r11_sched_tests.c src/hxf.c $(VM_CORE) -o $@ $(NET_BACKEND_LIBS)
$(R11_TIMER_BIN): tests/r11_timer_tests.c src/hxf.c $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r11_timer_tests.c src/hxf.c $(VM_CORE) -o $@ $(NET_BACKEND_LIBS)
$(R12_TELEM_BIN): tests/r12_telem_tests.c src/hxf.c $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r12_telem_tests.c src/hxf.c $(VM_CORE) -o $@ $(NET_BACKEND_LIBS)
$(R13_TELEM_BIN): tests/r13_telem_inspect_tests.c src/telem_inspect.c src/hxf.c $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r13_telem_inspect_tests.c src/telem_inspect.c src/hxf.c $(VM_CORE) -o $@ $(NET_BACKEND_LIBS)
$(R20_POLICY_BIN): tests/r20_policy_tests.c src/hxf.c $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r20_policy_tests.c src/hxf.c $(VM_CORE) -o $@ $(NET_BACKEND_LIBS)
$(R21_DET_BIN): tests/r21_policy_determinism_tests.c src/policy_verify.c src/hxf.c $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r21_policy_determinism_tests.c src/policy_verify.c src/hxf.c $(VM_CORE) -o $@ $(NET_BACKEND_LIBS)
$(R21_GUEST_BIN): tests/r21_policy_guest_tests.c src/policy_verify.c src/telem_inspect.c src/hxf.c $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r21_policy_guest_tests.c src/policy_verify.c src/telem_inspect.c src/hxf.c $(VM_CORE) -o $@ $(NET_BACKEND_LIBS)
EVAL_HOST_SRCS := src/eval_stats.c src/eval_metrics.c src/eval_fairness.c src/eval_cost.c \
	src/eval_workload.c src/eval_report.c src/eval_compare.c
$(R22_UNIT_BIN): tests/r22_eval_unit_tests.c $(EVAL_HOST_SRCS) src/telem_inspect.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r22_eval_unit_tests.c $(EVAL_HOST_SRCS) src/telem_inspect.c -o $@
$(R22_RUN_BIN): tests/r22_eval_runner_tests.c $(EVAL_SRCS) src/telem_inspect.c src/hxf.c $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r22_eval_runner_tests.c $(EVAL_SRCS) src/telem_inspect.c src/hxf.c $(VM_CORE) -o $@ $(NET_BACKEND_LIBS)
$(HAEUN_EVAL_BIN): tools/haeun_eval.c $(EVAL_SRCS) src/telem_inspect.c src/hxf.c $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tools/haeun_eval.c $(EVAL_SRCS) src/telem_inspect.c src/hxf.c $(VM_CORE) -o $@ $(NET_BACKEND_LIBS)
$(BUILD)/r11-images.stamp: tools/build_r11.py tools/build_process_image.py $(wildcard userland/r11_*.s) $(wildcard kernel/process/*.s) | $(BUILD)
	$(PYTHON) tools/build_r11.py $(BUILD)
$(BUILD)/r12-images.stamp: tools/build_r12.py tools/build_process_image.py $(wildcard userland/r12_*.s) $(wildcard kernel/process/*.s) | $(BUILD)
	$(PYTHON) tools/build_r12.py $(BUILD)
$(BUILD)/r21-images.stamp: tools/build_r21.py tools/build_process_image.py $(wildcard kernel/process/*.s) | $(BUILD)
	$(PYTHON) tools/build_r21.py $(BUILD)
$(BUILD)/r22-images.stamp: tools/build_r22.py tools/build_r12.py tools/build_process_image.py $(wildcard userland/r12_*.s) userland/r22_burst.s $(wildcard kernel/process/*.s) | $(BUILD)
	$(PYTHON) tools/build_r22.py $(BUILD)
$(R30_UNIT_BIN): tests/r30_fcfs_unit_tests.c src/policy_verify.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r30_fcfs_unit_tests.c src/policy_verify.c -o $@
$(R30_GUEST_BIN): tests/r30_fcfs_guest_tests.c $(EVAL_SRCS) src/policy_verify.c src/telem_inspect.c src/hxf.c $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r30_fcfs_guest_tests.c $(EVAL_SRCS) src/policy_verify.c src/telem_inspect.c src/hxf.c $(VM_CORE) -o $@ $(NET_BACKEND_LIBS)
$(BUILD)/r30-images.stamp: tools/build_r30.py tools/build_r12.py tools/build_process_image.py $(wildcard userland/r12_*.s) userland/r22_burst.s $(wildcard kernel/process/*.s) | $(BUILD)
	$(PYTHON) tools/build_r30.py $(BUILD)
$(R31_UNIT_BIN): tests/r31_pval_unit_tests.c $(PVAL_SRCS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r31_pval_unit_tests.c $(PVAL_SRCS) -o $@
$(R31_GUEST_BIN): tests/r31_pval_guest_tests.c $(PVAL_SRCS) src/hxf.c $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r31_pval_guest_tests.c $(PVAL_SRCS) src/hxf.c $(VM_CORE) -o $@ $(NET_BACKEND_LIBS)
$(R32_UNIT_BIN): tests/r32_reconfig_unit_tests.c $(RECFG_SRCS) $(PVAL_SRCS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r32_reconfig_unit_tests.c $(RECFG_SRCS) $(PVAL_SRCS) -o $@
$(R32_GUEST_BIN): tests/r32_reconfig_guest_tests.c $(RECFG_SRCS) $(PVAL_SRCS) src/hxf.c $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r32_reconfig_guest_tests.c $(RECFG_SRCS) $(PVAL_SRCS) src/hxf.c $(VM_CORE) -o $@ $(NET_BACKEND_LIBS)
$(R33_UNIT_BIN): tests/r33_planner_unit_tests.c $(PLANNER_SRCS) $(RECFG_SRCS) $(PVAL_SRCS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r33_planner_unit_tests.c $(PLANNER_SRCS) $(RECFG_SRCS) $(PVAL_SRCS) -o $@
$(R33_GUEST_BIN): tests/r33_planner_guest_tests.c $(PLANNER_SRCS) $(RECFG_SRCS) $(PVAL_SRCS) src/hxf.c $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r33_planner_guest_tests.c $(PLANNER_SRCS) $(RECFG_SRCS) $(PVAL_SRCS) src/hxf.c $(VM_CORE) -o $@ $(NET_BACKEND_LIBS)
$(R34_UNIT_BIN): tests/r34_active_unit_tests.c src/policy_verify.c $(RECFG_SRCS) $(PVAL_SRCS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r34_active_unit_tests.c src/policy_verify.c $(RECFG_SRCS) $(PVAL_SRCS) -o $@
$(R34_GUEST_BIN): tests/r34_active_guest_tests.c src/policy_verify.c $(RECFG_SRCS) $(PVAL_SRCS) src/hxf.c $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r34_active_guest_tests.c src/policy_verify.c $(RECFG_SRCS) $(PVAL_SRCS) src/hxf.c $(VM_CORE) -o $@ $(NET_BACKEND_LIBS)
$(R35_UNIT_BIN): tests/r35_feedback_unit_tests.c $(FEEDBACK_CORE) $(PLANNER_SRCS) $(RECFG_SRCS) $(PVAL_SRCS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r35_feedback_unit_tests.c $(FEEDBACK_CORE) $(PLANNER_SRCS) $(RECFG_SRCS) $(PVAL_SRCS) -o $@
$(R35_GUEST_BIN): tests/r35_feedback_guest_tests.c $(FEEDBACK_SRCS) $(PLANNER_SRCS) $(RECFG_SRCS) $(PVAL_SRCS) src/hxf.c $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r35_feedback_guest_tests.c $(FEEDBACK_SRCS) $(PLANNER_SRCS) $(RECFG_SRCS) $(PVAL_SRCS) src/hxf.c $(VM_CORE) -o $@ $(NET_BACKEND_LIBS)
$(R36_UNIT_BIN): tests/r36_opt_unit_tests.c $(OPT_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r36_opt_unit_tests.c $(OPT_CORE) -o $@
$(R36_GUEST_BIN): tests/r36_opt_guest_tests.c $(OPT_SRCS) $(PLANNER_SRCS) $(RECFG_SRCS) $(PVAL_SRCS) src/hxf.c $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r36_opt_guest_tests.c $(OPT_SRCS) $(PLANNER_SRCS) $(RECFG_SRCS) $(PVAL_SRCS) src/hxf.c $(VM_CORE) -o $@ $(NET_BACKEND_LIBS)
$(R37_UNIT_BIN): tests/r37_robust_unit_tests.c $(RB_CORE) $(OPT_CORE) src/eval_workload.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r37_robust_unit_tests.c $(RB_CORE) $(OPT_CORE) src/eval_workload.c -o $@
$(R37_GUEST_BIN): tests/r37_robust_guest_tests.c $(RB_SRCS) $(OPT_SRCS) $(PLANNER_SRCS) $(RECFG_SRCS) $(PVAL_SRCS) src/hxf.c $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r37_robust_guest_tests.c $(RB_SRCS) $(OPT_SRCS) $(PLANNER_SRCS) $(RECFG_SRCS) $(PVAL_SRCS) src/hxf.c $(VM_CORE) -o $@ $(NET_BACKEND_LIBS)
$(R40_INTEG_BIN): tests/r40_integration_tests.c $(RB_SRCS) $(OPT_SRCS) $(PLANNER_SRCS) $(RECFG_SRCS) $(PVAL_SRCS) src/hxf.c $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r40_integration_tests.c $(RB_SRCS) $(OPT_SRCS) $(PLANNER_SRCS) $(RECFG_SRCS) $(PVAL_SRCS) src/hxf.c $(VM_CORE) -o $@ $(NET_BACKEND_LIBS)
$(R41_VAL_BIN): tests/r41_validation_tests.c $(RB_SRCS) $(OPT_SRCS) $(PLANNER_SRCS) $(RECFG_SRCS) $(PVAL_SRCS) src/hxf.c $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r41_validation_tests.c $(RB_SRCS) $(OPT_SRCS) $(PLANNER_SRCS) $(RECFG_SRCS) $(PVAL_SRCS) src/hxf.c $(VM_CORE) -o $@ $(NET_BACKEND_LIBS)
$(R42_VAL_BIN): tests/r42_validation_tests.c $(OPT_SRCS) $(PLANNER_SRCS) $(RECFG_SRCS) $(PVAL_SRCS) src/hxf.c $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r42_validation_tests.c $(OPT_SRCS) $(PLANNER_SRCS) $(RECFG_SRCS) $(PVAL_SRCS) src/hxf.c $(VM_CORE) -o $@ $(NET_BACKEND_LIBS)
$(R43_VAL_BIN): tests/r43_active_verification_tests.c src/policy_verify.c $(RECFG_SRCS) $(PVAL_SRCS) src/hxf.c $(VM_CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/r43_active_verification_tests.c src/policy_verify.c $(RECFG_SRCS) $(PVAL_SRCS) src/hxf.c $(VM_CORE) -o $@ $(NET_BACKEND_LIBS)

test: $(VM_TEST_BIN) $(HXF_TEST_BIN) $(BOOTINFO_TEST_BIN) $(HFW_TEST_BIN) $(DEVICE_TEST_BIN) $(ISA4_TEST_BIN) $(MMU_TEST_BIN) $(PT_TEST_BIN) $(KERNEL_PAGE_TEST_BIN) $(TLB_TEST_BIN) $(CPU_MODULE_TEST_BIN) $(OPCODE_MATRIX_TEST_BIN) $(BOARD_EXTRA_TEST_BIN) $(FB_VIEW_TEST_BIN) $(HFS_TEST_BIN) $(PROC_HFS_TEST_BIN) $(PROC_ECALL_IO_TEST_BIN) $(GFX_TEST_BIN) $(GFX_WM_TEST_BIN) $(M23_3_STATE_TEST_BIN) $(M23_4_RES_TEST_BIN) $(M24_2_EXIT_TEST_BIN) $(M24_2_CAP_TEST_BIN) $(M24_2_FAULT_RES_TEST_BIN) $(M24_4_RES_TEST_BIN) $(M25_2_OWN_TEST_BIN) $(M25_2_SCRUB_TEST_BIN) $(M25_6_HEAP_HOST_BIN) $(GFX_UI_TEST_BIN) $(GFX_DESK_TEST_BIN) $(GFX_CHROME_TEST_BIN) $(GFX_ECALL_TEST_BIN) $(NET_TEST_BIN) $(NET_ICMP_TEST_BIN) $(NET_UDP_TEST_BIN) $(NET_TCP_TEST_BIN) $(NET_DNS_TEST_BIN) $(NET_SOCK_TEST_BIN) $(NET_SOCK_ECALL_TEST_BIN) $(SEC_TEST_BIN) $(SEC_DISPATCH_TEST_BIN) $(SEC_ECALL_TEST_BIN) $(SEC_BOOT_TEST_BIN) $(M7_TEST_BIN) $(M8_PCB_PROBE_BIN) $(M9_NET_PROBE_BIN) $(M13_NET_BE_TEST_BIN) $(M14_APP_TEST_BIN) $(M15_ISO_TEST_BIN) $(M16_REL_TEST_BIN) $(M17_FAULT_TEST_BIN) $(M20_UNIT_TEST_BIN) $(M20_RES_TEST_BIN) $(M20_INPUT_TEST_BIN) $(M20_IPC_TEST_BIN) $(M20_TEXT_TEST_BIN) $(M20_WIN_TEST_BIN) $(M20_LAUNCH_TEST_BIN) $(M20_SM_TEST_BIN) $(M20_FILE_TEST_BIN) $(M20_INTEG_TEST_BIN) $(MKFS_BIN) $(FSCK_BIN) $(VM_BIN) $(R11_SCHED_BIN) $(R11_TIMER_BIN) $(R12_TELEM_BIN) $(R13_TELEM_BIN) $(R20_POLICY_BIN) $(R21_DET_BIN) $(R21_GUEST_BIN) $(R22_UNIT_BIN) $(R22_RUN_BIN) $(HAEUN_EVAL_BIN) $(R30_UNIT_BIN) $(R30_GUEST_BIN) $(R31_UNIT_BIN) $(R31_GUEST_BIN) $(R32_UNIT_BIN) $(R32_GUEST_BIN) $(R33_UNIT_BIN) $(R33_GUEST_BIN) $(R34_UNIT_BIN) $(R34_GUEST_BIN) $(R35_UNIT_BIN) $(R35_GUEST_BIN) $(R36_UNIT_BIN) $(R36_GUEST_BIN) $(R37_UNIT_BIN) $(R37_GUEST_BIN) $(R40_INTEG_BIN) $(R41_VAL_BIN) $(R42_VAL_BIN) $(R43_VAL_BIN) $(BUILD)/r11-images.stamp $(BUILD)/r12-images.stamp $(BUILD)/r21-images.stamp $(BUILD)/r22-images.stamp $(BUILD)/r30-images.stamp toolchain-test
	./$(VM_TEST_BIN)
	./$(HXF_TEST_BIN)
	./$(BOOTINFO_TEST_BIN)
	./$(HFW_TEST_BIN)
	./$(DEVICE_TEST_BIN)
	./$(ISA4_TEST_BIN)
	./$(MMU_TEST_BIN)
	./$(PT_TEST_BIN)
	./$(KERNEL_PAGE_TEST_BIN)
	./$(TLB_TEST_BIN)
	./$(CPU_MODULE_TEST_BIN)
	./$(OPCODE_MATRIX_TEST_BIN)
	./$(BOARD_EXTRA_TEST_BIN)
	./$(FB_VIEW_TEST_BIN)
	./$(HFS_TEST_BIN)
	./$(PROC_HFS_TEST_BIN)
	./$(PROC_ECALL_IO_TEST_BIN)
	./$(GFX_TEST_BIN)
	./$(GFX_WM_TEST_BIN)
	./$(M23_3_STATE_TEST_BIN)
	./$(M23_4_RES_TEST_BIN)
	./$(M24_2_EXIT_TEST_BIN)
	./$(M24_2_CAP_TEST_BIN)
	./$(M24_2_FAULT_RES_TEST_BIN)
	./$(M24_4_RES_TEST_BIN)
	./$(M25_2_OWN_TEST_BIN)
	./$(M25_2_SCRUB_TEST_BIN)
	./$(M25_6_HEAP_HOST_BIN)
	./$(GFX_UI_TEST_BIN)
	./$(GFX_DESK_TEST_BIN)
	./$(GFX_CHROME_TEST_BIN)
	./$(GFX_ECALL_TEST_BIN)
	./$(NET_TEST_BIN)
	./$(NET_ICMP_TEST_BIN)
	./$(NET_UDP_TEST_BIN)
	./$(NET_TCP_TEST_BIN)
	./$(NET_DNS_TEST_BIN)
	./$(NET_SOCK_TEST_BIN)
	./$(NET_SOCK_ECALL_TEST_BIN)
	./$(SEC_TEST_BIN)
	./$(SEC_DISPATCH_TEST_BIN)
	./$(SEC_ECALL_TEST_BIN)
	./$(SEC_BOOT_TEST_BIN)
	./$(M7_TEST_BIN)
	$(PYTHON) tests/m7_guest_gui_tests.py $(VM_BIN)
	$(PYTHON) tests/m8_process_gui_tests.py $(VM_BIN)
	$(PYTHON) tests/m9_process_net_tests.py $(VM_BIN)
	./$(M13_NET_BE_TEST_BIN)
	$(PYTHON) tests/m13_process_net_tests.py $(VM_BIN)
	./$(M14_APP_TEST_BIN)
	$(PYTHON) tests/m14_process_apps_tests.py $(VM_BIN)
	./$(M15_ISO_TEST_BIN)
	./$(M16_REL_TEST_BIN)
	./$(M17_FAULT_TEST_BIN)
	$(PYTHON) tests/m17_process_fault_tests.py $(VM_BIN)
	$(PYTHON) tests/m16_repeat_tests.py $(VM_BIN)
	$(PYTHON) tests/m10_input_tests.py $(VM_BIN)
	$(PYTHON) tests/m11_viewer_tests.py $(VM_BIN)
	$(PYTHON) tests/m12_desktop_tests.py $(VM_BIN)
	$(PYTHON) tests/m20_build_integrity_tests.py $(VM_BIN)
	$(PYTHON) tests/m19_sdk_tests.py $(VM_BIN)
	./$(M20_UNIT_TEST_BIN)
	./$(M20_RES_TEST_BIN)
	./$(M20_INPUT_TEST_BIN)
	./$(M20_IPC_TEST_BIN)
	./$(M20_TEXT_TEST_BIN)
	./$(M20_WIN_TEST_BIN)
	./$(M20_LAUNCH_TEST_BIN)
	./$(M20_SM_TEST_BIN)
	./$(M20_FILE_TEST_BIN)
	./$(M20_INTEG_TEST_BIN)
	$(PYTHON) tests/m20_process_tests.py $(VM_BIN)
	$(PYTHON) tests/m20_input_process_tests.py $(VM_BIN)
	$(PYTHON) tests/m20_ipc_process_tests.py $(VM_BIN)
	$(PYTHON) tests/m20_text_process_tests.py $(VM_BIN)
	$(PYTHON) tests/m20_window_process_tests.py $(VM_BIN)
	$(PYTHON) tests/m20_launcher_process_tests.py $(VM_BIN)
	$(PYTHON) tests/m20_file_process_tests.py $(VM_BIN)
	$(PYTHON) tests/m20_integration_process_tests.py $(VM_BIN)
	$(PYTHON) tests/m20_9_gui_e2e_tests.py $(VM_BIN)
	$(PYTHON) tests/m21_persistent_desktop_tests.py $(VM_BIN)
	$(PYTHON) tests/m21_3_persistent_runtime_tests.py $(VM_BIN)
	$(PYTHON) tests/m22_1_desktop_shell_tests.py $(VM_BIN)
	$(PYTHON) tests/m22_1_1_rendering_tests.py $(VM_BIN)
	$(PYTHON) tests/m22_1_2_composition_tests.py $(VM_BIN)
	$(PYTHON) tests/m22_3_interactive_app_tests.py $(VM_BIN)
	$(PYTHON) tests/m22_4_rendering_tests.py $(VM_BIN)
	$(PYTHON) tests/m23_2_focus_recovery_tests.py $(VM_BIN)
	$(PYTHON) tests/m23_2_multi_window_tests.py $(VM_BIN)
	$(PYTHON) tests/m23_2_close_lifecycle_tests.py $(VM_BIN)
	$(PYTHON) tests/m23_3_window_state_tests.py $(VM_BIN)
	$(PYTHON) tests/m23_3_input_routing_tests.py $(VM_BIN)
	$(PYTHON) tests/m23_3_redraw_tests.py $(VM_BIN)
	$(PYTHON) tests/m23_4_integration_tests.py $(VM_BIN)
	$(PYTHON) tests/m23_4_stress_tests.py $(VM_BIN)
	$(PYTHON) tests/m23_4_composition_tests.py $(VM_BIN)
	$(PYTHON) tests/m24_2_lifecycle_contract_tests.py $(VM_BIN)
	$(PYTHON) tests/m24_2_orphan_wait_tests.py $(VM_BIN)
	$(PYTHON) tests/m24_2_parent_death_tests.py $(VM_BIN)
	$(PYTHON) tests/m24_3_init_reaper_tests.py $(VM_BIN)
	$(PYTHON) tests/m24_3_orphan_stress_tests.py $(VM_BIN)
	$(PYTHON) tests/m24_3_reclamation_tests.py $(VM_BIN)
	$(PYTHON) tests/m24_4_lifecycle_integration_tests.py $(VM_BIN)
	$(PYTHON) tests/m24_4_pid_reuse_tests.py $(VM_BIN)
	$(PYTHON) tests/m24_4_resource_reclamation_tests.py $(VM_BIN) $(M24_4_RES_TEST_BIN)
	$(PYTHON) tests/m25_2_as_ownership_tests.py $(VM_BIN) $(M25_2_OWN_TEST_BIN)
	$(PYTHON) tests/m25_2_memory_scrub_tests.py $(VM_BIN) $(M25_2_SCRUB_TEST_BIN)
	$(PYTHON) tests/m25_2_fork_policy_tests.py $(VM_BIN)
	$(PYTHON) tests/m25_3_fork_memory_snapshot_tests.py $(VM_BIN)
	$(PYTHON) tests/m25_3_fork_isolation_tests.py $(VM_BIN)
	$(PYTHON) tests/m25_3_fork_failure_tests.py $(VM_BIN)
	$(PYTHON) tests/m25_4_stack_boundary_tests.py $(VM_BIN)
	$(PYTHON) tests/m25_4_stack_overflow_tests.py $(VM_BIN)
	$(PYTHON) tests/m25_4_fork_stack_guard_tests.py $(VM_BIN)
	$(PYTHON) tests/m25_6_heap_mapping_tests.py $(VM_BIN) $(M25_6_HEAP_HOST_BIN)
	$(PYTHON) tests/m25_6_allocator_tests.py $(VM_BIN)
	$(PYTHON) tests/m25_6_heap_isolation_tests.py $(VM_BIN)
	$(PYTHON) tests/m25_6_heap_lifecycle_tests.py $(VM_BIN)
	./$(R11_SCHED_BIN) $(BUILD)
	./$(R11_TIMER_BIN) $(BUILD)
	./$(R12_TELEM_BIN) $(BUILD)
	./$(R13_TELEM_BIN) $(BUILD)
	./$(R20_POLICY_BIN) $(BUILD)
	./$(R21_DET_BIN) $(BUILD)
	./$(R21_GUEST_BIN) $(BUILD)
	./$(R22_UNIT_BIN)
	./$(R22_RUN_BIN) $(BUILD)
	./$(R30_UNIT_BIN)
	./$(R30_GUEST_BIN) $(BUILD)
	./$(R31_UNIT_BIN)
	./$(R31_GUEST_BIN) $(BUILD)
	./$(R32_UNIT_BIN)
	./$(R32_GUEST_BIN) $(BUILD)
	./$(R33_UNIT_BIN)
	./$(R33_GUEST_BIN) $(BUILD)
	./$(R34_UNIT_BIN)
	./$(R34_GUEST_BIN) $(BUILD)
	./$(R35_UNIT_BIN)
	./$(R35_GUEST_BIN) $(BUILD)
	./$(R36_UNIT_BIN)
	./$(R36_GUEST_BIN) $(BUILD)
	./$(R37_UNIT_BIN)
	./$(R37_GUEST_BIN) $(BUILD)
	./$(R40_INTEG_BIN) $(BUILD)
	./$(R41_VAL_BIN) $(BUILD) both
	./$(R42_VAL_BIN) $(BUILD)
	./$(R43_VAL_BIN) $(BUILD)












