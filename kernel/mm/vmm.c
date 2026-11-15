/* Identity VMM: accepts va==pa (flat map until paging is enabled). */

int vmm_init(void) {
    return 0;
}

int vmm_map(int va, int pa) {
    int result = 0;
    int failed = 0;
    if (va != pa) {
        failed = 1;
        result = 1;
    }
    if (failed == 0) {
        if (va < 0) {
            failed = 1;
            result = 1;
        }
    }
    if (failed == 0) {
        result = va;
    }
    return result;
}

int vmm_unmap(int va) {
    int st = 0;
    if (va == 0) {
        st = 1;
    }
    return st;
}
