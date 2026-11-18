#include "haeun/bootinfo.h"
#include "haeun/disk_persist.h"
#include "haeun/fb_dump.h"
#include "haeun/hfw.h"
#include "haeun/hxf.h"
#include "haeun/sec_boot.h"
#include "haeun/pval_validate.h"
#include "haeun/feedback.h"
#include "haeun/optimize.h"
#include "haeun/planner.h"
#include "haeun/robust.h"
#include "haeun/reconfig_cli.h"
#include "haeun/telem_inspect.h"
#include "haeun/vm.h"
#include "haeun/vm_gui_host.h"
#include "haeun/vm_input_host.h"
#include "haeun/vm_net_host.h"
#include "haeun/vm_viewer.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_RAM_SIZE (256u * 1024u * 1024u)
#define INITIAL_STACK_BASE UINT32_C(0x0ff00000)
#define BOOTINFO_ADDRESS UINT32_C(0x0feff000)
/* Non-persist smoke/regression budget. Persist defaults to unbounded. */
#define INSTRUCTION_BUDGET UINT64_C(10000000)

static const char *usage_text(void)
{
	return "usage: haeun-vm <hxf>|--firmware <hfw> [--disk IMG] "
	       "[--root-hash HEX] [--gui] [--desktop] [--apps] [--persist] "
	       "[--persist-stress] [--max-steps N] "
	       "[--app HXF]... [--net|--net-real] [--input] "
	       "[--input-script NAME] [--viewer] [--user] [--fb-dump PPM] "
	       "[--telem|--telem-json] "
	       "[--validate-policy|--validate-policy-json] "
	       "[--target rr|fcfs] "
	       "[--reconfigure-policy rr|fcfs --to rr|fcfs] "
	       "[--reconfigure-json] "
	       "[--plan-policy] [--plan-json] [--execute] "
	       "[--feedback-report|--feedback-json] "
	       "[--adaptive|--evaluate-adaptive] [--dry-run] "
	       "[--cycles N] [--state FILE] "
	       "[--optimize|--optimization-report|--optimization-json] "
	       "[--profile NAME] [--repetitions N] "
	       "[--apply-recommendation FILE] [--build-dir DIR] "
	       "[--robustness-report|--robustness-json] "
	       "[--experiment-preset smoke|standard|extended|research] "
	       "[--sensitivity-analysis] [--reconfiguration-stress] "
	       "[--verify-recommendation] [--r37-out DIR]\n";
}

static void uart_stdout(void *context, uint8_t byte)
{
	(void)context;
	(void)fputc((int)byte, stdout);
}

/** Reads an entire file into a newly allocated buffer; caller frees on success. */
static uint8_t *read_file(const char *path, size_t *out_size)
{
	FILE *file;
	long file_length;
	uint8_t *buf;
	size_t bytes_read;

	file = fopen(path, "rb");
	if (file == NULL) {
		(void)fprintf(stderr, "cannot open '%s': ", path);
		perror(NULL);
		return NULL;
	}
	if (fseek(file, 0, SEEK_END) != 0 || (file_length = ftell(file)) < 0 ||
	    (unsigned long)file_length > UINT32_MAX ||
	    fseek(file, 0, SEEK_SET) != 0) {
		(void)fprintf(stderr, "image size or seek is invalid\n");
		(void)fclose(file);
		return NULL;
	}
	buf = (uint8_t *)malloc((size_t)file_length);
	if (buf == NULL) {
		(void)fprintf(stderr, "unable to allocate image buffer\n");
		(void)fclose(file);
		return NULL;
	}
	bytes_read = fread(buf, 1u, (size_t)file_length, file);
	if (bytes_read != (size_t)file_length || fclose(file) != 0) {
		(void)fprintf(stderr, "failed to read complete image\n");
		free(buf);
		return NULL;
	}
	*out_size = (size_t)file_length;
	return buf;
}

static int register_one_app(haeun_vm_gui_host *host, const char *name,
			    const char *path)
{
	size_t sz = 0;
	uint8_t *img;
	haeun_app_result ar;

	img = read_file(path, &sz);
	if (img == NULL) {
		(void)fprintf(stderr, "missing app image %s\n", path);
		return -1;
	}
	ar = haeun_vm_gui_host_register_app(host, name, img, sz);
	/* Keep images alive for the process lifetime (leak by design). */
	if (ar != HAEUN_APP_OK) {
		free(img);
		(void)fprintf(stderr, "register %s failed\n", name);
		return -1;
	}
	(void)printf("apps: registered %s (%u bytes)\n", name, (unsigned)sz);
	return 0;
}

static int register_builtin_apps(haeun_vm_gui_host *host)
{
	static const char *names[] = {"calc", "edit", "files"};
	static const char *descs[] = {"12+34 demo", "text edit", "file list"};
	static const char *cats[] = {"demo", "tools", "tools"};
	static const char *paths[] = {"build/app_calc.hxf", "build/app_edit.hxf",
				      "build/app_files.hxf"};
	size_t i;
	for (i = 0; i < 3u; ++i) {
		if (register_one_app(host, names[i], paths[i]) != 0)
			return -1;
		(void)haeun_app_set_meta(&host->apps, (int)i, descs[i], "1.0",
					 cats[i], 0u);
	}
	/* Optional M20.6 windowless service binary. */
	if (register_one_app(host, "svcidle", "build/app_svc_idle.hxf") == 0) {
		int idx = haeun_app_count(&host->apps) - 1;
		(void)haeun_app_set_meta(&host->apps, idx, "idle svc", "1.0", "svc",
					 HAEUN_APP_FLAG_SINGLE);
		(void)haeun_sm_register(&host->sm, "idle", "event-stop idle", "1.0",
					 idx, HAEUN_SM_START_MANUAL,
					 HAEUN_SM_RESTART_ON_FAIL);
	}
	/* Optional M20.7 GUI file manager. */
	if (register_one_app(host, "filemgr", "build/app_filemgr.hxf") == 0) {
		int idx = haeun_app_count(&host->apps) - 1;
		(void)haeun_app_set_meta(&host->apps, idx, "HFS files", "1.0",
					 "tools", 0u);
	}
	return 0;
}

static const char *app_stem(const char *path)
{
	const char *base = path;
	const char *p;
	for (p = path; *p != '\0'; ++p) {
		if (*p == '/' || *p == '\\')
			base = p + 1;
	}
	return base;
}

int main(int argc, char **argv)
{
	uint8_t *image = NULL, *disk = NULL, *ram = NULL;
	size_t image_size = 0, disk_size = 0;
	uint32_t entry, isa_version;
	haeun_vm vm;
	haeun_bootinfo bootinfo;
	haeun_vm_gui_host gui_host;
	haeun_vm_net_host net_host;
	haeun_vm_input_host input_host;
	haeun_vm_viewer viewer;
	haeun_vm_result result;
	bool firmware_mode = false, gui_mode = false, net_mode = false;
	bool net_real = false;
	bool input_mode = false, viewer_mode = false, user_mode = false;
	bool desktop_mode = false, apps_mode = false, persist_mode = false;
	bool persist_stress = false, builtins = false, has_max_steps = false;
	bool telem_mode = false, telem_json = false;
	bool validate_mode = false, validate_json = false;
	bool reconfig_mode = false, reconfig_json = false;
	bool plan_mode = false, plan_json = false, plan_execute = false;
	bool feedback_mode = false, feedback_json = false;
	bool adaptive_mode = false, adaptive_dry_run = false;
	bool opt_mode = false, opt_json = false, opt_run = false;
	bool opt_apply = false;
	bool rb_mode = false, rb_json = false, rb_run = false;
	bool rb_sens = false, rb_stress = false, rb_verify = false;
	unsigned feedback_cycles = 1u;
	unsigned opt_reps = 0u;
	const char *feedback_state = NULL;
	const char *opt_profile = NULL;
	const char *opt_build_dir = "build";
	const char *opt_apply_path = NULL;
	const char *rb_preset = NULL;
	const char *rb_out = NULL;
	uint32_t validate_target = UINT32_MAX;
	uint32_t reconfig_from = UINT32_MAX, reconfig_to = UINT32_MAX;
	const char *firmware_path = NULL, *hxf_path = NULL, *disk_path = NULL;
	const char *root_hash_hex = NULL, *fb_dump_path = NULL;
	const char *input_script = NULL;
	const char *extra_apps[HAEUN_APP_MAX];
	int extra_app_count = 0;
	uint8_t root_hash[HAEUN_SHA256_DIGEST_LEN];
	uint64_t steps;
	uint64_t run_budget = INSTRUCTION_BUDGET;
	uint64_t max_steps_arg = 0u;
	int i, rc = 2;

	memset(&gui_host, 0, sizeof(gui_host));
	memset(&net_host, 0, sizeof(net_host));
	memset(&input_host, 0, sizeof(input_host));
	memset(&viewer, 0, sizeof(viewer));
	for (i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--firmware") == 0 && i + 1 < argc) {
			firmware_mode = true;
			firmware_path = argv[++i];
		} else if (strcmp(argv[i], "--disk") == 0 && i + 1 < argc)
			disk_path = argv[++i];
		else if (strcmp(argv[i], "--root-hash") == 0 && i + 1 < argc)
			root_hash_hex = argv[++i];
		else if (strcmp(argv[i], "--gui") == 0)
			gui_mode = true;
		else if (strcmp(argv[i], "--desktop") == 0) {
			desktop_mode = true;
			gui_mode = true;
		} else if (strcmp(argv[i], "--apps") == 0) {
			apps_mode = true;
			builtins = true;
			desktop_mode = true;
			gui_mode = true;
		} else if (strcmp(argv[i], "--persist") == 0) {
			persist_mode = true;
			desktop_mode = true;
			gui_mode = true;
		} else if (strcmp(argv[i], "--persist-stress") == 0) {
			persist_stress = true;
			persist_mode = true;
			desktop_mode = true;
			gui_mode = true;
		} else if (strcmp(argv[i], "--max-steps") == 0 && i + 1 < argc) {
			const char *arg = argv[++i];
			char *end = NULL;
			errno = 0;
			max_steps_arg = strtoull(arg, &end, 10);
			if (errno != 0 || end == arg || *end != '\0' ||
			    max_steps_arg == 0u) {
				(void)fprintf(stderr,
					      "invalid --max-steps value\n");
				return 2;
			}
			has_max_steps = true;
		} else if (strcmp(argv[i], "--app") == 0 && i + 1 < argc) {
			if (extra_app_count >= HAEUN_APP_MAX) {
				(void)fprintf(stderr,
					      "too many --app paths (max %d)\n",
					      HAEUN_APP_MAX);
				return 2;
			}
			extra_apps[extra_app_count++] = argv[++i];
			apps_mode = true;
			desktop_mode = true;
			gui_mode = true;
		} else if (strcmp(argv[i], "--net") == 0)
			net_mode = true;
		else if (strcmp(argv[i], "--net-real") == 0) {
			net_mode = true;
			net_real = true;
		} else if (strcmp(argv[i], "--input") == 0)
			input_mode = true;
		else if (strcmp(argv[i], "--input-script") == 0 &&
			 i + 1 < argc) {
			input_script = argv[++i];
			input_mode = true;
		} else if (strcmp(argv[i], "--viewer") == 0)
			viewer_mode = true;
		else if (strcmp(argv[i], "--user") == 0)
			user_mode = true;
		else if (strcmp(argv[i], "--fb-dump") == 0 && i + 1 < argc)
			fb_dump_path = argv[++i];
		else if (strcmp(argv[i], "--telem") == 0)
			telem_mode = true;
		else if (strcmp(argv[i], "--telem-json") == 0) {
			telem_mode = true;
			telem_json = true;
		} else if (strcmp(argv[i], "--validate-policy") == 0)
			validate_mode = true;
		else if (strcmp(argv[i], "--validate-policy-json") == 0) {
			validate_mode = true;
			validate_json = true;
		} else if (strcmp(argv[i], "--target") == 0 && i + 1 < argc) {
			const char *t = argv[++i];
			if (strcmp(t, "rr") == 0)
				validate_target = HAEUN_PVAL_POLICY_RR;
			else if (strcmp(t, "fcfs") == 0)
				validate_target = HAEUN_PVAL_POLICY_FCFS;
			else {
				(void)fprintf(stderr, "unknown --target %s\n", t);
				return 2;
			}
		} else if (strcmp(argv[i], "--reconfigure-policy") == 0 &&
			   i + 1 < argc) {
			reconfig_mode = true;
			if (haeun_recfg_cli_parse_policy(argv[++i],
							&reconfig_from) != 0) {
				(void)fprintf(stderr,
					      "unknown --reconfigure-policy\n");
				return 2;
			}
		} else if (strcmp(argv[i], "--to") == 0 && i + 1 < argc) {
			if (haeun_recfg_cli_parse_policy(argv[++i],
							&reconfig_to) != 0) {
				(void)fprintf(stderr, "unknown --to policy\n");
				return 2;
			}
		} else if (strcmp(argv[i], "--reconfigure-json") == 0) {
			reconfig_json = true;
		} else if (strcmp(argv[i], "--plan-policy") == 0) {
			plan_mode = true;
		} else if (strcmp(argv[i], "--plan-json") == 0) {
			plan_mode = true;
			plan_json = true;
		} else if (strcmp(argv[i], "--execute") == 0) {
			plan_execute = true;
		} else if (strcmp(argv[i], "--feedback-report") == 0) {
			feedback_mode = true;
		} else if (strcmp(argv[i], "--feedback-json") == 0) {
			feedback_mode = true;
			feedback_json = true;
		} else if (strcmp(argv[i], "--adaptive") == 0 ||
			   strcmp(argv[i], "--evaluate-adaptive") == 0) {
			feedback_mode = true;
			adaptive_mode = true;
		} else if (strcmp(argv[i], "--dry-run") == 0) {
			adaptive_dry_run = true;
		} else if (strcmp(argv[i], "--cycles") == 0 && i + 1 < argc) {
			feedback_cycles = (unsigned)strtoul(argv[++i], NULL, 10);
			if (feedback_cycles == 0u)
				feedback_cycles = 1u;
		} else if (strcmp(argv[i], "--state") == 0 && i + 1 < argc) {
			feedback_state = argv[++i];
		} else if (strcmp(argv[i], "--optimize") == 0) {
			opt_mode = true;
			opt_run = true;
		} else if (strcmp(argv[i], "--optimization-report") == 0) {
			opt_mode = true;
		} else if (strcmp(argv[i], "--optimization-json") == 0) {
			opt_mode = true;
			opt_json = true;
		} else if (strcmp(argv[i], "--profile") == 0 && i + 1 < argc) {
			opt_profile = argv[++i];
		} else if (strcmp(argv[i], "--repetitions") == 0 &&
			   i + 1 < argc) {
			opt_reps = (unsigned)strtoul(argv[++i], NULL, 10);
		} else if (strcmp(argv[i], "--build-dir") == 0 &&
			   i + 1 < argc) {
			opt_build_dir = argv[++i];
		} else if (strcmp(argv[i], "--apply-recommendation") == 0 &&
			   i + 1 < argc) {
			opt_mode = true;
			opt_apply = true;
			opt_apply_path = argv[++i];
		} else if (strcmp(argv[i], "--robustness-report") == 0) {
			rb_mode = true;
			rb_run = true;
		} else if (strcmp(argv[i], "--robustness-json") == 0) {
			rb_mode = true;
			rb_run = true;
			rb_json = true;
		} else if (strcmp(argv[i], "--experiment-preset") == 0 &&
			   i + 1 < argc) {
			rb_preset = argv[++i];
			rb_mode = true;
		} else if (strcmp(argv[i], "--sensitivity-analysis") == 0) {
			rb_mode = true;
			rb_run = true;
			rb_sens = true;
		} else if (strcmp(argv[i], "--reconfiguration-stress") == 0) {
			rb_mode = true;
			rb_stress = true;
		} else if (strcmp(argv[i], "--verify-recommendation") == 0) {
			rb_mode = true;
			rb_run = true;
			rb_verify = true;
		} else if (strcmp(argv[i], "--r37-out") == 0 && i + 1 < argc) {
			rb_out = argv[++i];
			rb_mode = true;
		} else if (argv[i][0] != '-' && hxf_path == NULL &&
			 firmware_path == NULL)
			hxf_path = argv[i];
		else {
			(void)fprintf(stderr, "%s", usage_text());
			return 2;
		}
	}
	if (firmware_mode == (hxf_path != NULL) ||
	    (root_hash_hex != NULL && firmware_mode && disk_path == NULL)) {
		(void)fprintf(stderr, "%s", usage_text());
		return 2;
	}
	image = read_file(firmware_mode ? firmware_path : hxf_path, &image_size);
	if (image == NULL)
		return 2;
	if (disk_path != NULL &&
	    (disk = read_file(disk_path, &disk_size)) == NULL)
		goto out;
	ram = (uint8_t *)calloc(INITIAL_RAM_SIZE, 1u);
	if (ram == NULL) {
		(void)fprintf(stderr, "unable to allocate 256 MiB guest RAM\n");
		goto out;
	}
	if (firmware_mode) {
		if (root_hash_hex != NULL &&
		    (haeun_sec_boot_parse_root_hash(root_hash_hex, root_hash) !=
			     HAEUN_SEC_OK ||
		     haeun_sec_boot_verify_disk_hxf(disk, disk_size, root_hash) !=
			     HAEUN_SEC_OK)) {
			(void)fprintf(stderr, "secure boot refused disk HXF\n");
			goto out;
		}
		if (haeun_hfw_load(image, image_size, ram, INITIAL_RAM_SIZE,
				   &entry, &isa_version) != HAEUN_HFW_OK) {
			(void)fprintf(stderr, "HFW load failed\n");
			goto out;
		}
	} else {
		if (root_hash_hex != NULL &&
		    (haeun_sec_boot_parse_root_hash(root_hash_hex, root_hash) !=
			     HAEUN_SEC_OK ||
		     haeun_sec_boot_verify(image, image_size, root_hash) !=
			     HAEUN_SEC_OK)) {
			(void)fprintf(stderr, "secure boot refused image HXF\n");
			goto out;
		}
		if (haeun_hxf_load_ex(image, image_size, ram, INITIAL_RAM_SIZE,
				      &entry, &isa_version) != HAEUN_HXF_OK) {
			(void)fprintf(stderr, "HXF load failed\n");
			goto out;
		}
		bootinfo.flags = 0u;
		bootinfo.ram_size = INITIAL_RAM_SIZE;
		bootinfo.stack_base = INITIAL_STACK_BASE;
		bootinfo.stack_top = INITIAL_RAM_SIZE;
		bootinfo.isa_version = isa_version;
		if (haeun_bootinfo_encode(ram + BOOTINFO_ADDRESS,
					  INITIAL_RAM_SIZE - BOOTINFO_ADDRESS,
					  &bootinfo) != HAEUN_BOOTINFO_OK) {
			(void)fprintf(stderr, "bootinfo encode failed\n");
			goto out;
		}
	}
	free(image);
	image = NULL;
	if (haeun_vm_init_version(&vm, ram, INITIAL_RAM_SIZE, entry,
				  isa_version) != HAEUN_VM_OK) {
		(void)fprintf(stderr, "VM initialization failed\n");
		goto out;
	}
	vm.r[13] = INITIAL_RAM_SIZE;
	vm.r[14] = INITIAL_STACK_BASE;
	haeun_vm_uart_set_tx_sink(&vm, uart_stdout, NULL);
	if (firmware_mode)
		haeun_vm_protect_boot_rom(&vm);
	if (disk != NULL && !haeun_vm_block_attach(&vm, disk, disk_size)) {
		(void)fprintf(stderr, "invalid disk image for block attach\n");
		goto out;
	}
	if (!firmware_mode) {
		vm.r[2] = BOOTINFO_ADDRESS;
		vm.r[3] = HAEUN_BOOTINFO_SIZE;
	}
	if (apps_mode) {
		if (haeun_vm_gui_host_attach_apps(&gui_host, &vm) !=
		    HAEUN_GFX_OK) {
			(void)fprintf(stderr, "APPS host attach failed\n");
			goto out;
		}
		if (builtins && register_builtin_apps(&gui_host) != 0)
			goto out;
		for (i = 0; i < extra_app_count; ++i) {
			char name[32];
			const char *stem = app_stem(extra_apps[i]);
			size_t n = 0;
			while (stem[n] != '\0' && stem[n] != '.' && n + 1u < sizeof(name)) {
				name[n] = stem[n];
				n++;
			}
			name[n] = '\0';
			if (name[0] == '\0') {
				(void)fprintf(stderr, "bad --app name for %s\n",
					      extra_apps[i]);
				goto out;
			}
			if (register_one_app(&gui_host, name, extra_apps[i]) !=
			    0)
				goto out;
		}
		if (!builtins && extra_app_count == 0) {
			(void)fprintf(stderr, "no apps registered\n");
			goto out;
		}
	} else if (gui_mode &&
		   haeun_vm_gui_host_attach_ex(&gui_host, &vm,
					       desktop_mode ? 1 : 0) !=
			   HAEUN_GFX_OK) {
		(void)fprintf(stderr, "GUI host attach failed\n");
		goto out;
	}
	if (net_mode &&
	    haeun_vm_net_host_attach_ex(&net_host, &vm, net_real ? 1 : 0) !=
		    HAEUN_NET_OK) {
		(void)fprintf(stderr, "NET host attach failed\n");
		goto out;
	}
	/*
	 * net attach installs its own mux; restore GUI/apps/FD assists when both
	 * hosts are active so --apps/--gui --net keeps a single combined mux.
	 */
	if (net_mode && (apps_mode || gui_mode) && gui_host.attached) {
		gui_host.mux.sock_ctx = &net_host.sock_ctx;
		haeun_vm_set_ecall_io(&vm, haeun_ecall_mux_io, &gui_host.mux);
		haeun_vm_set_fault_io(&vm, haeun_ecall_mux_on_fault, &gui_host.mux);
	}
	if (input_mode) {
		if (haeun_vm_input_host_attach(&input_host, &vm,
					       gui_mode ? &gui_host.wm
							: NULL) != 0) {
			(void)fprintf(stderr, "INPUT host attach failed\n");
			goto out;
		}
		if (gui_mode) {
			input_host.evt = &gui_host.evt;
			if (desktop_mode)
				input_host.chrome = &gui_host.chrome;
		}
		if (input_script != NULL) {
			if (haeun_vm_input_host_load_m22_3_script(
				    &input_host, input_script) != 0) {
				(void)fprintf(stderr,
					      "unknown --input-script '%s'\n",
					      input_script);
				goto out;
			}
		} else if (persist_stress)
			haeun_vm_input_host_load_m21_stress_script(&input_host);
		else if (persist_mode)
			haeun_vm_input_host_load_m21_script(&input_host);
		else if (apps_mode)
			haeun_vm_input_host_load_m20_script(&input_host);
		else
			haeun_vm_input_host_load_demo_script(&input_host);
	}
	/*
	 * Budget policy (M21.3):
	 * - non-persist: fixed INSTRUCTION_BUDGET (smoke / regression)
	 * - persist: unbounded cumulative steps; viewer still uses VIEW_SLICE
	 *   quanta so the Win32 pump stays responsive
	 * - --max-steps N: explicit ceiling for any mode (tests / safety)
	 * STEP_LIMIT remains a real terminal result when a ceiling is hit;
	 * persist simply does not install the 10M ceiling by default.
	 */
	if (has_max_steps)
		run_budget = max_steps_arg;
	else if (persist_mode)
		run_budget = UINT64_MAX;
	else
		run_budget = INSTRUCTION_BUDGET;
	if (persist_mode)
		(void)printf("persist: run_budget=%llu (viewer quantum=slice)\n",
			     (unsigned long long)run_budget);
	if (viewer_mode) {
		haeun_viewer_result vr = haeun_vm_viewer_attach(
			&viewer, &vm, gui_mode ? &gui_host.wm : NULL, 1);
		if (vr == HAEUN_VIEWER_UNSUPPORTED) {
			(void)fprintf(stderr, "viewer unsupported on this host\n");
			goto out;
		}
		if (vr != HAEUN_VIEWER_OK) {
			(void)fprintf(stderr, "VIEWER attach failed\n");
			goto out;
		}
		if (input_mode)
			viewer.script = &input_host;
		if (desktop_mode)
			viewer.chrome = &gui_host.chrome;
	}
	if (user_mode)
		vm.privilege = HAEUN_PRIV_USER;
	if (viewer_mode)
		result = haeun_vm_viewer_run(&viewer, &vm, run_budget, &steps);
	else if (net_mode)
		result = haeun_vm_net_host_run(&net_host, &vm, run_budget,
					       &steps);
	else if (input_mode)
		result = haeun_vm_input_host_run(&input_host, &vm, run_budget,
						 &steps);
	else
		result = haeun_vm_run(&vm, run_budget, &steps);
	if (net_mode && net_host.tx.count > 0)
		(void)printf("net-host: TX frames=%d last_len=%u real=%d "
			     "be_tx=%u be_rx=%u\n",
			     net_host.tx.count, net_host.tx.len,
			     net_host.real_mode, net_host.be.tx_ok,
			     net_host.be.rx_ok);
	if (input_mode && !viewer_mode)
		(void)printf("input-host: keys=%d mice=%d focus_hits=%d "
			     "focus_id=%d\n",
			     input_host.keys_fed, input_host.mice_posted,
			     input_host.focus_hits,
			     gui_mode ? gui_host.wm.focus_id : -1);
	if (viewer_mode)
		(void)printf("viewer: frames=%d keys=%d mice=%d focus_hits=%d "
			     "focus_id=%d\n",
			     viewer.frames_drawn, viewer.keys_fed,
			     viewer.mice_posted, viewer.focus_hits,
			     gui_mode ? gui_host.wm.focus_id : -1);
	if (fb_dump_path != NULL) {
		if (haeun_fb_dump_ppm_path(&vm, fb_dump_path) !=
		    HAEUN_FB_DUMP_OK) {
			(void)fprintf(stderr, "framebuffer PPM dump failed\n");
			goto out;
		}
		(void)printf("wrote framebuffer PPM %s\n", fb_dump_path);
	}
	if (telem_mode) {
		haeun_telem_report trep;
		if (haeun_telem_read(&vm, &trep) != HAEUN_TELEM_OK)
			(void)fprintf(stderr, "telemetry read failed\n");
		else if (telem_json)
			haeun_telem_fprint_json(stdout, &trep);
		else
			haeun_telem_fprint(stdout, &trep);
	}
	if (validate_mode) {
		haeun_pval_report vrep;
		haeun_pval_result vr =
			haeun_pval_validate(&vm, validate_target, &vrep);
		if (vr != HAEUN_PVAL_OK)
			(void)fprintf(stderr, "policy validation failed (%d)\n",
				      (int)vr);
		else if (validate_json)
			haeun_pval_report_fprint_json(stdout, &vrep);
		else
			haeun_pval_report_fprint(stdout, &vrep);
	}
	if (reconfig_mode) {
		int rrc;
		if (reconfig_from == UINT32_MAX || reconfig_to == UINT32_MAX) {
			(void)fprintf(stderr,
				      "reconfigure requires --reconfigure-policy "
				      "and --to\n");
			rc = 2;
			goto out;
		}
		rrc = haeun_recfg_cli_run(&vm, reconfig_from, reconfig_to,
					  reconfig_json ? 1 : 0);
		if (rrc != 0) {
			rc = rrc;
			goto out;
		}
	}
	if (plan_mode) {
		int prc = haeun_plan_cli_run(&vm, plan_json ? 1 : 0,
					     plan_execute ? 1 : 0);
		if (prc != 0) {
			rc = prc;
			goto out;
		}
	}
	if (feedback_mode) {
		int frc = haeun_fb_cli_run(&vm, feedback_json ? 1 : 0,
					   adaptive_mode ? 1 : 0,
					   adaptive_dry_run ? 1 : 0,
					   feedback_cycles, feedback_state);
		if (frc != 0) {
			rc = frc;
			goto out;
		}
	}
	if (opt_mode) {
		int orc = haeun_opt_cli_run(
			&vm, opt_build_dir, opt_json ? 1 : 0, opt_run ? 1 : 0,
			adaptive_dry_run ? 1 : 0, opt_apply ? 1 : 0, opt_profile,
			opt_reps, feedback_state, opt_apply_path);
		if (orc != 0) {
			rc = orc;
			goto out;
		}
	}
	if (rb_mode) {
		int rbr = haeun_rb_cli_run(
			&vm, opt_build_dir, rb_json ? 1 : 0, rb_run ? 1 : 0,
			adaptive_dry_run ? 1 : 0, rb_sens ? 1 : 0,
			rb_stress ? 1 : 0, rb_verify ? 1 : 0, rb_preset, opt_reps,
			opt_profile, feedback_state,
			rb_out ? rb_out : "build/r37");
		if (rbr != 0) {
			rc = rbr;
			goto out;
		}
	}
	if (disk != NULL && disk_path != NULL &&
	    haeun_disk_write_back(disk_path, disk, disk_size) != 0) {
		(void)fprintf(stderr, "failed to write persistent disk image\n");
		goto out;
	}
	if (result == HAEUN_VM_HALTED) {
		(void)printf("guest halted after %llu instructions; r1=0x%08x\n",
			     (unsigned long long)steps, vm.r[1]);
		rc = 0;
		goto out;
	}
	if (result == HAEUN_VM_STEP_LIMIT)
		(void)fprintf(stderr,
			      "guest stopped: STEP_LIMIT after %llu instructions "
			      "(budget=%llu) at PC 0x%08x (last cause %u)\n",
			      (unsigned long long)steps,
			      (unsigned long long)run_budget, vm.pc, vm.cause);
	else
		(void)fprintf(stderr,
			      "guest stopped with result %u at PC 0x%08x (cause %u)\n",
			      (unsigned)result, vm.pc, vm.cause);
	rc = 1;
out:
	if (net_mode)
		haeun_vm_net_host_shutdown(&net_host);
	free(image);
	free(disk);
	free(ram);
	return rc;
}
