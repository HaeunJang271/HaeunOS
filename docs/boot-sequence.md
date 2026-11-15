# Boot sequence (Release 1.0)

## Direct HXF path

1. Host loads HXF into guest RAM (`haeun_hxf_load_ex`).
2. Encodes HBI at `0x0FEFF000`.
3. `haeun_vm_init_version` → optional UART/block/GUI/net/input/viewer.
4. Run until HALT or budget.

## Firmware disk path

1. Host loads HFW; optional `--root-hash` verifies disk HXF.
2. Protects Boot ROM; attaches `--disk`.
3. Guest `firmware/boot.s` reads block 0, validates constrained HXF, copies, writes HBI, jumps to entry.
4. Kernel foundation or process kernel continues.

## Apps / GUI / Net host order (`src/main.c`)

1. VM init  
2. `--apps` / `--gui` attach (ECALL mux + optional HFS/APP_ENV)  
3. `--net` attach  
4. If both GUI and net: **merge `sock_ctx` into GUI mux** (M16) and reinstall `fault_io` (M17)  
5. input / viewer  
6. run / shutdown  

## Process boot (process-v1 images)

`proc_boot.s` builds AS slots + PCB0, enables timer, enters `user_init`.  
`APP_SPAWN` loads registered HXF apps into free slots 1–2.

See also [firmware-boot.md](firmware-boot.md), [boot-disk-v1.md](boot-disk-v1.md), [process-v1.md](process-v1.md).
