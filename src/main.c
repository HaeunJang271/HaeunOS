#include "haeun/bootinfo.h"
#include "haeun/disk_persist.h"
#include "haeun/fb_dump.h"
#include "haeun/hfw.h"
#include "haeun/hxf.h"
#include "haeun/sec_boot.h"
#include "haeun/vm.h"
#include "haeun/vm_gui_host.h"
#include "haeun/vm_input_host.h"
#include "haeun/vm_net_host.h"
#include "haeun/vm_viewer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_RAM_SIZE (256u * 1024u * 1024u)
#define INITIAL_STACK_BASE UINT32_C(0x0ff00000)
#define BOOTINFO_ADDRESS UINT32_C(0x0feff000)
#define INSTRUCTION_BUDGET UINT64_C(10000000)

static const char *usage_text(void)
{
	return "usage: haeun-vm <hxf>|--firmware <hfw> [--disk IMG] "
	       "[--root-hash HEX] [--gui] [--desktop] [--apps] "
	       "[--app HXF]... [--net|--net-real] [--input] [--viewer] "
	       "[--user] [--fb-dump PPM]\n";
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
	bool desktop_mode = false, apps_mode = false;
	bool builtins = false;
	const char *firmware_path = NULL, *hxf_path = NULL, *disk_path = NULL;
	const char *root_hash_hex = NULL, *fb_dump_path = NULL;
	const char *extra_apps[HAEUN_APP_MAX];
	int extra_app_count = 0;
	uint8_t root_hash[HAEUN_SHA256_DIGEST_LEN];
	uint64_t steps;
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
		else if (strcmp(argv[i], "--viewer") == 0)
			viewer_mode = true;
		else if (strcmp(argv[i], "--user") == 0)
			user_mode = true;
		else if (strcmp(argv[i], "--fb-dump") == 0 && i + 1 < argc)
			fb_dump_path = argv[++i];
		else if (argv[i][0] != '-' && hxf_path == NULL &&
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
		if (apps_mode)
			haeun_vm_input_host_load_m20_script(&input_host);
		else
			haeun_vm_input_host_load_demo_script(&input_host);
	}
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
		result = haeun_vm_viewer_run(&viewer, &vm, INSTRUCTION_BUDGET,
					     &steps);
	else if (net_mode)
		result = haeun_vm_net_host_run(&net_host, &vm,
					       INSTRUCTION_BUDGET, &steps);
	else if (input_mode)
		result = haeun_vm_input_host_run(&input_host, &vm,
						 INSTRUCTION_BUDGET, &steps);
	else
		result = haeun_vm_run(&vm, INSTRUCTION_BUDGET, &steps);
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
