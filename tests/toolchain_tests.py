import hashlib
import os
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import haeun_toolchain as tc


PROGRAM = """\
.align 4
_start:
    addi r1, r0, 0
    beq r1, r0, done
    halt
done:
    addi r2, r0, 7
    halt
"""


class ToolchainTests(unittest.TestCase):
    def test_vm_cli_bootinfo_and_stack_register_handoff(self):
        source = """\
_start:
    lw r4, 16(r2)
    bne r4, r14, fail
    lw r4, 20(r2)
    bne r4, r13, fail
    addi r5, r0, 32
    bne r3, r5, fail
    addi r1, r0, 1
    halt
fail:
    addi r1, r0, 0
    halt
"""
        image_bytes=tc.link_hobj(tc.assemble(source))
        with tempfile.TemporaryDirectory() as tmp:
            image=Path(tmp)/"bootinfo-check.hxf"
            image.write_bytes(image_bytes)
            vm=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/"build"/"haeun-vm.exe"
            result=subprocess.run([str(vm),str(image)],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertIn("r1=0x00000001",result.stdout)

    def test_hfw_cli_stages_and_executes_haeun_firmware(self):
        source="""\
.isa 3
_start:
    cmpeq r1, r2, r3
    cmpne r4, r14, r0
    and r1, r1, r4
    cmpne r4, r13, r0
    and r1, r1, r4
    halt
"""
        obj=tc.parse_hobj(tc.assemble(source))
        payload=next(section["data"] for section in obj["sections"] if section["name"]==".text")
        memory_size=(len(payload)+7)&~3
        hfw=struct.pack("<4sHHIIIIII",b"HFW1",1,32,3,0,0,len(payload),memory_size,0)+payload
        with tempfile.TemporaryDirectory() as tmp:
            image=Path(tmp)/"firmware.hfw"
            image.write_bytes(hfw)
            vm=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/"build"/"haeun-vm.exe"
            result=subprocess.run([str(vm),"--firmware",str(image)],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertIn("guest halted after 6 instructions",result.stdout)
        self.assertIn("r1=0x00000001",result.stdout)

    def test_assemble_link_and_run_guest(self):
        obj = tc.assemble(PROGRAM)
        parsed = tc.parse_hobj(obj)
        symbols, relocs = parsed["symbols"], parsed["relocs"]
        self.assertEqual(len(relocs), 1)
        self.assertIn("_start", [symbol["name"] for symbol in symbols])
        hxf = tc.link_hobj(obj)
        self.assertEqual(hxf[:4], b"HXF1")
        self.assertEqual(struct.unpack_from("<I", hxf, 96 + 4)[0] & 0x3FFFF, 1)
        with tempfile.TemporaryDirectory() as tmp:
            image = Path(tmp) / "program.hxf"
            image.write_bytes(hxf)
            vm = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build" / "haeun-vm.exe"
            result = subprocess.run([str(vm), str(image)], check=True,
                                    capture_output=True, text=True)
            self.assertIn("guest halted after 4 instructions", result.stdout)

    def test_all_isa_mnemonics_assemble(self):
        source = """\
_start:
nop
halt
add r1,r2,r3
sub r1,r2,r3
and r1,r2,r3
or r1,r2,r3
xor r1,r2,r3
addi r1,r2,-1
lw r1,4(r2)
sw r1,4(r2)
beq r1,r2,_start
bne r1,r2,_start
jal r15,_start
ecall
"""
        obj = tc.assemble(source)
        text = next(sec["data"] for sec in tc.parse_hobj(obj)["sections"] if sec["name"] == ".text")
        self.assertEqual(len(text), 14 * 4)

    def test_bad_source_rejected(self):
        for source in ("_start: addi r1,r0,131072", "_start: .align 3",
                       "_start: bogus r1", "_start: halt\n_start: halt"):
            with self.subTest(source=source), self.assertRaises(tc.ToolchainError):
                tc.assemble(source)

    def test_link_rejects_undefined_symbol_and_bad_hobj(self):
        obj = tc.assemble(".global missing\n_start: beq r0,r0,missing\nhalt")
        with self.assertRaisesRegex(tc.ToolchainError, "unresolved"):
            tc.link_hobj(obj)
        damaged = bytearray(tc.assemble("_start: halt"))
        damaged[14] = 1
        with self.assertRaises(tc.ToolchainError):
            tc.parse_hobj(bytes(damaged))
        end_target = tc.assemble("_start: beq r0,r0,end\nend:")
        with self.assertRaisesRegex(tc.ToolchainError, "within its section"):
            tc.link_hobj(end_target)

    def test_multiple_objects_and_data_section(self):
        source_a = """\
.global finish
_start: beq r0,r0,finish
local_label: nop
halt
.data
value: .byte 7
"""
        source_b = """\
.global finish
local_label: nop
finish: halt
.data
.align 16
other: .word 0x1234
"""
        first = tc.assemble(source_a)
        second = tc.assemble(source_b)
        hxf = tc.link_hobjs([first, second])
        self.assertEqual(struct.unpack_from("<H", hxf, 24)[0], 2)
        text_segment = struct.unpack_from("<IIIIIIII", hxf, 64)
        data_segment = struct.unpack_from("<IIIIIIII", hxf, 96)
        self.assertEqual(text_segment[1], 5)
        self.assertEqual(data_segment[1], 3)
        self.assertGreaterEqual(data_segment[3], text_segment[3]+text_segment[5])
        with tempfile.TemporaryDirectory() as tmp:
            image = Path(tmp) / "multi.hxf"
            image.write_bytes(hxf)
            vm = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build" / "haeun-vm.exe"
            result = subprocess.run([str(vm), str(image)], check=True,
                                    capture_output=True, text=True)
            self.assertIn("guest halted after 2 instructions", result.stdout)
            asm_tool = ROOT / "tools" / "haeun_toolchain.py"
            src_a, src_b = Path(tmp)/"a.s", Path(tmp)/"b.s"
            obj_a, obj_b, cli_image = Path(tmp)/"a.hobj", Path(tmp)/"b.hobj", Path(tmp)/"cli.hxf"
            src_a.write_text(source_a, encoding="utf-8")
            src_b.write_text(source_b, encoding="utf-8")
            subprocess.run([sys.executable, str(asm_tool), "as", str(src_a), str(obj_a)], check=True)
            subprocess.run([sys.executable, str(asm_tool), "as", str(src_b), str(obj_b)], check=True)
            subprocess.run([sys.executable, str(asm_tool), "ld", "-o", str(cli_image),
                            str(obj_a), str(obj_b)], check=True)
            result = subprocess.run([str(vm), str(cli_image)], check=True,
                                    capture_output=True, text=True)
            self.assertIn("guest halted after 2 instructions", result.stdout)

    def test_hobj_v1_remains_linkable(self):
        section_offset, symbol_offset, data_offset = 40, 68, 92
        blob = bytearray(96)
        struct.pack_into("<4sHHHHHHIIIIII", blob, 0, b"HOBJ", 1, 40, 1, 1, 0, 0,
                         section_offset, symbol_offset, data_offset, data_offset, 4, len(blob))
        struct.pack_into("<8sIIIII", blob, section_offset, b".text\0\0\0", 1, 5, 4, data_offset, 4)
        struct.pack_into("<16sII", blob, symbol_offset, b"_start\0"+bytes(9), 0, 1)
        struct.pack_into("<I", blob, data_offset, 0x04000000)
        self.assertEqual(tc.link_hobj(bytes(blob))[:4], b"HXF1")

    def test_hobj_v1_branch_relocation_remains_linkable(self):
        section_offset, symbols_offset, reloc_offset, data_offset = 40, 68, 116, 132
        blob = bytearray(140)
        struct.pack_into("<4sHHHHHHIIIIII", blob, 0, b"HOBJ", 1, 40, 1, 2, 1, 0,
                         section_offset, symbols_offset, reloc_offset, data_offset, 8, len(blob))
        struct.pack_into("<8sIIIII", blob, section_offset, b".text\0\0\0", 1, 5, 4, data_offset, 8)
        struct.pack_into("<16sII", blob, symbols_offset, b"_start\0"+bytes(9), 0, 1)
        struct.pack_into("<16sII", blob, symbols_offset+24, b"done\0"+bytes(11), 4, 1)
        struct.pack_into("<IHH i I", blob, reloc_offset, 0, 1, 1, 0, 0)
        struct.pack_into("<II", blob, data_offset, 10 << 26, 1 << 26)
        linked = tc.link_hobj(bytes(blob))
        self.assertEqual(struct.unpack_from("<I", linked, 96)[0] & 0x3FFFF, 0)

    def test_isa_v2_jalr_ret_and_versioned_link(self):
        source = """\
.isa 2
_start: jal r15, function
halt
function: ret
"""
        obj = tc.assemble(source)
        self.assertEqual(tc.parse_hobj(obj)["isa"], 2)
        image_bytes = tc.link_hobj(obj)
        self.assertEqual(struct.unpack_from("<I", image_bytes, 8)[0], 2)
        with tempfile.TemporaryDirectory() as tmp:
            image = Path(tmp)/"call-return.hxf"
            image.write_bytes(image_bytes)
            vm = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build" / "haeun-vm.exe"
            result = subprocess.run([str(vm), str(image)], check=True,
                                    capture_output=True, text=True)
            self.assertIn("guest halted after 3 instructions", result.stdout)
        with self.assertRaisesRegex(tc.ToolchainError, "requires HAEUN ISA v2"):
            tc.assemble("_start: ret")
        with self.assertRaisesRegex(tc.ToolchainError, "requires HAEUN ISA v3"):
            tc.assemble("_start: cmplt r1,r2,r3")

    def test_isa_v3_comparison_assembly_and_link(self):
        source = ".isa 3\n_start:\n" + "\n".join(
            f"{name} r1,r2,r3" for name in tc.CMP_OPS
        ) + "\nhalt\n"
        obj = tc.assemble(source)
        parsed = tc.parse_hobj(obj)
        self.assertEqual(parsed["isa"], 3)
        self.assertEqual(struct.unpack_from("<H", obj, 4)[0], 4)
        image = tc.link_hobj(obj)
        self.assertEqual(struct.unpack_from("<I", image, 8)[0], 3)

    def test_link_rejects_mixed_isa_object_versions(self):
        v1 = tc.assemble("_start: halt")
        v2 = tc.assemble(".isa 2\n_start: ret")
        with self.assertRaisesRegex(tc.ToolchainError, "different HAEUN ISA versions"):
            tc.link_hobjs([v1, v2])

    def test_tiny_c_subset_compiles_and_returns_value(self):
        source = "int main(void) { int base = 10; int increment = 3; return (base + increment) ^ 2; }"
        obj = tc.compile_c(source)
        image_bytes = tc.link_hobj(obj)
        with tempfile.TemporaryDirectory() as tmp:
            image = Path(tmp)/"return.hxf"
            image.write_bytes(image_bytes)
            vm = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build" / "haeun-vm.exe"
            result = subprocess.run([str(vm), str(image)], check=True,
                                    capture_output=True, text=True)
            self.assertIn("r1=0x0000000f", result.stdout)
            c_path = Path(tmp)/"return.c"
            c_obj = Path(tmp)/"return.hobj"
            c_image = Path(tmp)/"return-cli.hxf"
            c_path.write_text(source, encoding="utf-8")
            tool = ROOT/"tools"/"haeun_toolchain.py"
            subprocess.run([sys.executable, str(tool), "cc", str(c_path), str(c_obj)], check=True)
            subprocess.run([sys.executable, str(tool), "ld", "-o", str(c_image), str(c_obj)], check=True)
            result = subprocess.run([str(vm), str(c_image)], check=True,
                                    capture_output=True, text=True)
            self.assertIn("r1=0x0000000f", result.stdout)

    def test_tiny_c_subset_rejects_unsupported_or_unrepresentable_code(self):
        for source in ("int main(void) { return 131072; }",
                       "int main(void) { if (1 == 1) return 1; return 0; }",
                       "int main(void) { int x; return x; }",
                       "int main(void) { int x = y; int y = 1; return x; }"):
            with self.subTest(source=source), self.assertRaises(tc.ToolchainError):
                tc.compile_c(source)

    def test_tiny_c_comparisons_if_else_and_while(self):
        source = """\
int main(void) {
    int n = 0;
    int sum = 0;
    while (n != 5) {
        sum = sum + n;
        n = n + 1;
    }
    if (sum == 10) { sum = sum + 5; } else { sum = 0; }
    if (sum != 15) { sum = 0; } else { sum = sum + 1; }
    return sum;
}
"""
        image_bytes = tc.link_hobj(tc.compile_c(source))
        with tempfile.TemporaryDirectory() as tmp:
            image = Path(tmp)/"control.hxf"
            image.write_bytes(image_bytes)
            vm = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build" / "haeun-vm.exe"
            result = subprocess.run([str(vm), str(image)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn("r1=0x00000010", result.stdout)

    def run_c_guest(self, source):
        image_bytes = tc.link_hobj(tc.compile_c(source))
        with tempfile.TemporaryDirectory() as tmp:
            image = Path(tmp)/"functions.hxf"
            image.write_bytes(image_bytes)
            vm = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build" / "haeun-vm.exe"
            result = subprocess.run([str(vm), str(image)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            return result.stdout

    def test_c_function_calls_arguments_and_caller_register_restore(self):
        source = """\
int combine(int a, int b) {
    int local = a + b;
    return local;
}
int main(void) {
    int x = 20;
    int y = 22;
    int z = combine(x, y);
    return x + y + z;
}
"""
        self.assertIn("r1=0x00000054", self.run_c_guest(source))

    def test_c_six_argument_call(self):
        source = """\
int sum6(int a, int b, int c, int d, int e, int f) {
    return a + b + c + d + e + f;
}
int main(void) { return sum6(1, 2, 3, 4, 5, 6); }
"""
        self.assertIn("r1=0x00000015", self.run_c_guest(source))

    def test_c_stack_arguments_and_spilled_local_homes(self):
        source = """\
int combine8(int a, int b, int c, int d, int e, int f, int g, int h) {
    int x = a + h;
    int y = b + g;
    int z = c + f;
    int w = d + e;
    return x + y + z + w;
}
int main(void) {
    int a = 1; int b = 2; int c = 3; int d = 4;
    int e = 5; int f = 6; int g = 7; int h = 8;
    return (a + b) + combine8(a, b, c, d, e, f, g, h);
}
"""
        self.assertIn("r1=0x00000027", self.run_c_guest(source))

    def test_c_deep_expression_spills_and_reloads_temporaries(self):
        source = """\
int main(void) {
    int a=1; int b=2; int c=3; int d=4; int e=5; int f=6;
    int g=7; int h=8; int i=9; int j=10; int k=11; int l=12;
    return (a+b)+((c+d)+((e+f)+((g+h)+((i+j)+(k+l)))));
}
"""
        parsed=tc.TinyCParser(source).parse()
        assembly=tc.TinyCCodegen(parsed["functions"], parsed["externs"]).generate_program()
        self.assertRegex(assembly,r"addi r1, r13, -(\d+)\n    cmplt r1, r1, r14\n    bne r1, r0, \.Lstkfail\n    addi r13, r13, -\1")
        self.assertRegex(assembly,r"sw r\d+, 104\(r13\)")
        self.assertRegex(assembly,r"lw r\d+, 104\(r13\)")
        self.assertIn("r1=0x0000004e", self.run_c_guest(source))

    def test_c_recursive_call_uses_nested_stack_frames(self):
        source = """\
int count_up(int n) {
    int result = 0;
    if (n != 0) { result = count_up(n - 1); }
    return result + 1;
}
int main(void) { return count_up(12); }
"""
        self.assertIn("r1=0x0000000d", self.run_c_guest(source))

    def test_c_stack_overflow_stops_before_crossing_stack_limit(self):
        source = """\
int descend(int a, int b, int c, int d, int e, int f, int n) {
    if (n != 0) { n = descend(a, b, c, d, e, f, n - 1); }
    return n;
}
int main(void) {
    int n = 0;
    while (n < 125000) { n = n + 1; }
    n = n + n;
    return descend(1, 2, 3, 4, 5, 6, n);
}
"""
        image=tc.link_hobj(tc.compile_c(source))
        with tempfile.TemporaryDirectory() as tmp:
            image_path=Path(tmp)/"overflow.hxf"
            image_path.write_bytes(image)
            vm=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/"build"/"haeun-vm.exe"
            result=subprocess.run([str(vm),str(image_path)],capture_output=True,text=True)
        self.assertEqual(result.returncode,1,result.stdout)
        self.assertIn("result 2",result.stderr)
        self.assertIn("cause 3",result.stderr)

    def test_signed_relational_comparisons_in_guest_code(self):
        source = """\
int main(void) {
    int negative = -1;
    int zero = 0;
    int count = 0;
    if (negative < zero) count = count + 1;
    if (negative <= zero) count = count + 1;
    if (negative > zero) count = count + 1;
    if (negative >= zero) count = count + 1;
    if (negative == zero) count = count + 1;
    if (negative != zero) count = count + 1;
    return count;
}
"""
        obj=tc.compile_c(source)
        self.assertEqual(tc.parse_hobj(obj)["isa"],3)
        self.assertIn("r1=0x00000003",self.run_c_guest(source))

    def test_link_firmware_builds_hfw_at_address_zero(self):
        hfw = tc.link_firmware(
            [tc.assemble(".isa 3\n.global _start\n_start:\n    addi r1, r0, 9\n    halt\n")]
        )
        self.assertEqual(hfw[:4], b"HFW1")
        self.assertEqual(struct.unpack_from("<I", hfw, 8)[0], 3)
        self.assertEqual(struct.unpack_from("<I", hfw, 12)[0], 0)
        self.assertEqual(struct.unpack_from("<I", hfw, 16)[0], 0)

    def test_firmware_disk_boot_loads_hxf_and_transfers(self):
        hfw = tc.link_firmware(
            [tc.assemble((ROOT / "firmware" / "boot.s").read_text(encoding="utf-8"))]
        )
        # Pad .text so the HXF spans >1 block and exercises multi-block reads.
        guest_source = (
            ".isa 3\n_start:\n"
            "    lw r1, 0(r2)\n"
            "    jal r4, p_hbi\n"
            "    .word 0x31494248\n"
            "p_hbi:\n"
            "    lw r4, 0(r4)\n"
            "    bne r1, r4, fail\n"
            "    addi r1, r0, 32\n"
            "    bne r3, r1, fail\n"
            "    addi r1, r0, 0x2a\n"
            "    halt\n"
            "fail:\n"
            "    addi r1, r0, 0\n"
            "    halt\n"
        )
        guest_source += "".join(f"    .word {i}\n" for i in range(128))
        guest = tc.link_hobj(tc.assemble(guest_source))
        self.assertGreater(len(guest), 512)
        pad = (512 - (len(guest) % 512)) % 512
        disk = guest + bytes(pad)
        with tempfile.TemporaryDirectory() as tmp:
            fw = Path(tmp) / "boot.hfw"
            raw = Path(tmp) / "disk.raw"
            fw.write_bytes(hfw)
            raw.write_bytes(disk)
            vm = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build" / "haeun-vm.exe"
            result = subprocess.run(
                [str(vm), "--firmware", str(fw), "--disk", str(raw)],
                capture_output=True,
                text=True,
            )
            bad_path = Path(tmp) / "bad.raw"
            bad_path.write_bytes(b"BAD!" + bytes(508))
            bad = subprocess.run(
                [str(vm), "--firmware", str(fw), "--disk", str(bad_path)],
                capture_output=True,
                text=True,
            )
        self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        self.assertIn("r1=0x0000002a", result.stdout)
        self.assertEqual(bad.returncode, 0, bad.stderr + bad.stdout)
        self.assertIn("E\n", bad.stdout)
        self.assertIn("r1=0x00000000", bad.stdout)

    def test_firmware_boot_missing_disk_and_bad_entry(self):
        hfw = tc.link_firmware(
            [tc.assemble((ROOT / "firmware" / "boot.s").read_text(encoding="utf-8"))]
        )
        vm = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build" / "haeun-vm.exe"
        with tempfile.TemporaryDirectory() as tmp:
            fw = Path(tmp) / "boot.hfw"
            fw.write_bytes(hfw)
            missing = subprocess.run(
                [str(vm), "--firmware", str(fw), "--disk", str(Path(tmp) / "no.raw")],
                capture_output=True,
                text=True,
            )
            # Valid HXF magic/segment but entry outside the LOAD segment.
            bad_entry = bytearray(tc.link_hobj(tc.assemble(
                ".isa 3\n.global _start\n_start:\n    halt\n"
            )))
            struct.pack_into("<I", bad_entry, 16, 0x0010F000)
            pad = (512 - (len(bad_entry) % 512)) % 512
            raw = Path(tmp) / "badentry.raw"
            raw.write_bytes(bytes(bad_entry) + bytes(pad))
            bad = subprocess.run(
                [str(vm), "--firmware", str(fw), "--disk", str(raw)],
                capture_output=True,
                text=True,
            )
        self.assertNotEqual(missing.returncode, 0)
        self.assertEqual(bad.returncode, 0, bad.stderr + bad.stdout)
        self.assertIn("E\n", bad.stdout)

    def test_assemble_sfence_isa4(self):
        obj = tc.parse_hobj(tc.assemble(".isa 4\n.global _start\n_start:\n    sfence\n    halt\n"))
        self.assertEqual(obj["isa"], 4)
        text = next(s["data"] for s in obj["sections"] if s["name"] == ".text")
        self.assertEqual(struct.unpack_from("<I", text, 0)[0], 0x12 << 26)
        with self.assertRaises(tc.ToolchainError):
            tc.assemble(".isa 3\n.global _start\n_start:\n    sfence\n")

    def test_isa_metadata_matches_generated_header(self):
        gen = ROOT / "tools" / "gen_isa_opcodes.py"
        result = subprocess.run(
            [sys.executable, str(gen), "--check"], capture_output=True, text=True
        )
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_round_trip_assembly_object_executable_disassembly(self):
        source = """\
.isa 3
.global _start
_start:
    addi r1, r0, 1
    addi r2, r0, 2
    add r3, r1, r2
    cmpeq r4, r3, r2
    bne r4, r0, done
    halt
done:
    halt
"""
        obj = tc.assemble(source)
        hxf = tc.link_hobj(obj)
        obj_lines = "\n".join(tc.dump_hobj(obj))
        hxf_lines = "\n".join(tc.dump_hxf(hxf))
        self.assertIn("addi r1, r0, 1", obj_lines)
        self.assertIn("add r3, r1, r2", obj_lines)
        self.assertIn("cmpeq r4, r3, r2", obj_lines)
        self.assertIn("_start:", obj_lines)
        self.assertIn("done:", obj_lines)
        self.assertIn("addi r1, r0, 1", hxf_lines)
        self.assertIn("; HXF entry", hxf_lines)
        # Invalid word reports through dump helper.
        from haeun_isa import disassemble_word

        self.assertTrue(disassemble_word(0xFC000000).startswith("?"))

    def test_kernel_timer_preemptive_scheduler_two_tasks(self):
        sched = tc.assemble(
            (ROOT / "kernel" / "scheduler" / "sched.s").read_text(encoding="utf-8")
        )
        entry = tc.assemble(
            (ROOT / "kernel" / "scheduler" / "entry.s").read_text(encoding="utf-8")
        )
        self.assertEqual(tc.parse_hobj(sched)["isa"], 4)
        image = tc.link_hobjs([sched, entry])
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "kernel.hxf"
            path.write_bytes(image)
            vm = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build" / "haeun-vm.exe"
            result = subprocess.run(
                [str(vm), str(path)], capture_output=True, text=True
            )
        self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        # Quantum fires during each PUTC trap; tasks interleave without YIELD.
        self.assertIn("ABABABABABAB", result.stdout)
        self.assertIn("r1=0x00000032", result.stdout)

    def test_kernel_foundation_boots_via_firmware(self):
        sys.path.insert(0, str(ROOT / "tools"))
        from build_kernel_foundation import build as build_foundation

        hfw = tc.link_firmware(
            [tc.assemble((ROOT / "firmware" / "boot.s").read_text(encoding="utf-8"))]
        )
        guest = build_foundation()
        self.assertEqual(guest[8:12], (4).to_bytes(4, "little"))
        pad = (512 - (len(guest) % 512)) % 512
        disk = guest + bytes(pad)
        with tempfile.TemporaryDirectory() as tmp:
            fw = Path(tmp) / "boot.hfw"
            raw = Path(tmp) / "disk.raw"
            fw.write_bytes(hfw)
            raw.write_bytes(disk)
            vm = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build" / "haeun-vm.exe"
            result = subprocess.run(
                [str(vm), "--firmware", str(fw), "--disk", str(raw)],
                capture_output=True,
                text=True,
            )
        self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        self.assertIn("[HAEUN-OS] kernel init", result.stdout)
        self.assertIn("kernel ready", result.stdout)
        self.assertIn("r1=0x00000000", result.stdout)

    def test_userland_shell_persists_disk_image(self):
        sys.path.insert(0, str(ROOT / "tools"))
        from build_userland import build as build_shell

        mkfs = ROOT / "build" / ("haeun-mkfs.exe" if os.name == "nt" else "haeun-mkfs")
        fsck = ROOT / "build" / ("haeun-fsck.exe" if os.name == "nt" else "haeun-fsck")
        self.assertTrue(mkfs.exists(), "build haeun-mkfs before toolchain-test")
        shell = build_shell()
        with tempfile.TemporaryDirectory() as tmp:
            img = Path(tmp) / "root.img"
            hxf = Path(tmp) / "shell.hxf"
            hxf.write_bytes(shell)
            subprocess.run([str(mkfs), str(img), "256", "64"], check=True)
            before = img.read_bytes()
            vm = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build" / "haeun-vm.exe"
            result = subprocess.run(
                [str(vm), str(hxf), "--disk", str(img)],
                capture_output=True,
                text=True,
            )
            self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
            self.assertIn("init", result.stdout)
            self.assertIn("help", result.stdout)
            self.assertIn("mount ok", result.stdout)
            after = img.read_bytes()
            self.assertNotEqual(before, after)
            self.assertEqual(after[40], ord("G"))
            self.assertEqual(after[36], 1)  # dirty until fsck/unmount
            subprocess.run([str(fsck), str(img)], check=True)
            repaired = img.read_bytes()
            self.assertEqual(repaired[36], 0)

    def test_cli_root_hash_refuses_mismatch(self):
        source = """\
.isa 1
_start:
    halt
"""
        image_bytes = tc.link_hobj(tc.assemble(source))
        good = hashlib.sha256(image_bytes).hexdigest()
        bad = "00" * 32
        with tempfile.TemporaryDirectory() as tmp:
            image = Path(tmp) / "secure.hxf"
            image.write_bytes(image_bytes)
            vm = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build" / "haeun-vm.exe"
            ok = subprocess.run(
                [str(vm), str(image), "--root-hash", good],
                capture_output=True,
                text=True,
            )
            denied = subprocess.run(
                [str(vm), str(image), "--root-hash", bad],
                capture_output=True,
                text=True,
            )
        self.assertEqual(ok.returncode, 0, ok.stderr)
        self.assertNotEqual(denied.returncode, 0)
        self.assertIn("secure boot refused", denied.stderr)

    def test_firmware_disk_root_hash_refuses_mismatch(self):
        hfw = tc.link_firmware(
            [tc.assemble((ROOT / "firmware" / "boot.s").read_text(encoding="utf-8"))]
        )
        guest_source = ".isa 3\n_start:\n    addi r1, r0, 0x2a\n    halt\n"
        guest = tc.link_hobj(tc.assemble(guest_source))
        pad = (512 - (len(guest) % 512)) % 512
        disk = guest + bytes(pad)
        good = hashlib.sha256(guest).hexdigest()
        bad = "11" * 32
        with tempfile.TemporaryDirectory() as tmp:
            fw = Path(tmp) / "boot.hfw"
            raw = Path(tmp) / "disk.raw"
            fw.write_bytes(hfw)
            raw.write_bytes(disk)
            vm = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build" / "haeun-vm.exe"
            ok = subprocess.run(
                [str(vm), "--firmware", str(fw), "--disk", str(raw), "--root-hash", good],
                capture_output=True,
                text=True,
            )
            denied = subprocess.run(
                [str(vm), "--firmware", str(fw), "--disk", str(raw), "--root-hash", bad],
                capture_output=True,
                text=True,
            )
        self.assertEqual(ok.returncode, 0, ok.stderr + ok.stdout)
        self.assertIn("r1=0x0000002a", ok.stdout)
        self.assertNotEqual(denied.returncode, 0)
        self.assertIn("secure boot refused disk HXF", denied.stderr)


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]] + sys.argv[2:])
