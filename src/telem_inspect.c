#include "haeun/telem_inspect.h"

#include <string.h>

enum {
    PCB_BASE = 0x0000A000u,
    PCB_STRIDE = 0x100u,
    PCB_CURR = 0x0000A400u,
    IDLE_FLAG = 0x0000A460u,
    T_IRQ = 0x0000A464u,
    T_CTX = 0x0000A468u,
    AS_SLOTS = 3u,
    OFF_STATE = 0u,
    OFF_PID = 4u,
    OFF_RUN = 128u,
    OFF_READY_WAIT = 132u,
    OFF_SLEEP = 136u,
    OFF_BLOCKED = 140u,
    OFF_DISPATCH = 144u,
    OFF_COMPLETE_LO = 148u,
    OFF_COMPLETE_VALID = 152u,
    OFF_ACCOUNT = 156u,
    OFF_READY_SINCE = 160u,
    OFF_SLEEP_SINCE = 164u,
    OFF_BLOCKED_SINCE = 168u,
    STATE_FREE = 0u,
    STATE_ZOMBIE = 5u
};

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static int ram_ok(const haeun_vm *vm, uint32_t off) {
    return vm != NULL && vm->ram != NULL &&
           (size_t)off + 4u <= vm->ram_size;
}

static uint32_t load(const haeun_vm *vm, uint32_t off) {
    return rd32(vm->ram + off);
}

static void add_note(haeun_telem_report *out, haeun_telem_note_kind kind,
                     const char *text) {
    size_t n;
    if (out->note_count >= HAEUN_TELEM_NOTE_MAX)
        return;
    out->note_kind[out->note_count] = kind;
    n = strlen(text);
    if (n >= HAEUN_TELEM_NOTE_LEN)
        n = HAEUN_TELEM_NOTE_LEN - 1u;
    memcpy(out->notes[out->note_count], text, n);
    out->notes[out->note_count][n] = '\0';
    out->note_count++;
}

/**
 * Returns a static name for PCB state 0..5, or "UNKNOWN".
 */
const char *haeun_telem_state_name(uint32_t state) {
    switch (state) {
    case 0u:
        return "FREE";
    case 1u:
        return "READY";
    case 2u:
        return "RUNNING";
    case 3u:
        return "SLEEP";
    case 4u:
        return "WAIT";
    case 5u:
        return "ZOMBIE";
    default:
        return "UNKNOWN";
    }
}

static void fill_consistency(haeun_telem_report *out) {
    uint32_t disp_sum = 0u;
    unsigned i;
    int saw_stale_free = 0;
    add_note(out, HAEUN_TELEM_NOTE_INFO,
             "slot 3 has no process address space (expected)");
    for (i = 0; i < HAEUN_TELEM_NPROC; ++i) {
        const haeun_telem_slot *s = &out->slots[i];
        disp_sum += s->dispatch_count;
        if (!saw_stale_free && s->state == STATE_FREE &&
            (s->run_ticks != 0u || s->dispatch_count != 0u ||
             s->complete_valid != 0u)) {
            add_note(out, HAEUN_TELEM_NOTE_EXPECTED,
                     "FREE slot may retain stale telemetry (expected)");
            saw_stale_free = 1;
        }
        if (s->complete_valid != 0u && s->state != STATE_ZOMBIE &&
            s->state != STATE_FREE)
            add_note(out, HAEUN_TELEM_NOTE_WARN,
                     "complete_valid set while state is not ZOMBIE/FREE");
    }
    if (out->context_switches > disp_sum)
        add_note(out, HAEUN_TELEM_NOTE_WARN,
                 "context_switches exceeds sum of dispatch_count");
    add_note(out, HAEUN_TELEM_NOTE_INFO,
             "IDLE_FLAG is independent of timer_irqs/context_switches");
}

/**
 * Fills a report by reading guest RAM only. Does not write RAM or CSRs.
 */
haeun_telem_result haeun_telem_read(const haeun_vm *vm, haeun_telem_report *out) {
    unsigned i;
    if (vm == NULL || out == NULL)
        return HAEUN_TELEM_BAD_ARGUMENT;
    if (!ram_ok(vm, PCB_BASE) || !ram_ok(vm, T_CTX) || !ram_ok(vm, IDLE_FLAG))
        return HAEUN_TELEM_NO_PCB_PAGE;
    memset(out, 0, sizeof(*out));
    out->idle_flag = load(vm, IDLE_FLAG);
    out->timer_irqs = load(vm, T_IRQ);
    out->context_switches = load(vm, T_CTX);
    out->curr_index = load(vm, PCB_CURR);
    for (i = 0; i < HAEUN_TELEM_NPROC; ++i) {
        uint32_t base = PCB_BASE + (uint32_t)i * PCB_STRIDE;
        haeun_telem_slot *s = &out->slots[i];
        s->slot = (uint32_t)i;
        s->state = load(vm, base + OFF_STATE);
        s->pid = load(vm, base + OFF_PID);
        s->run_ticks = load(vm, base + OFF_RUN);
        s->ready_wait_ticks = load(vm, base + OFF_READY_WAIT);
        s->sleep_ticks = load(vm, base + OFF_SLEEP);
        s->blocked_ticks = load(vm, base + OFF_BLOCKED);
        s->dispatch_count = load(vm, base + OFF_DISPATCH);
        s->complete_lo = load(vm, base + OFF_COMPLETE_LO);
        s->complete_valid = load(vm, base + OFF_COMPLETE_VALID);
        s->account_lo = load(vm, base + OFF_ACCOUNT);
        s->ready_since = load(vm, base + OFF_READY_SINCE);
        s->sleep_since = load(vm, base + OFF_SLEEP_SINCE);
        s->blocked_since = load(vm, base + OFF_BLOCKED_SINCE);
        s->as_slot_valid = (i < AS_SLOTS) ? 1 : 0;
    }
    fill_consistency(out);
    return HAEUN_TELEM_OK;
}

/**
 * Prints a human-readable telemetry report to fp.
 */
void haeun_telem_fprint(FILE *fp, const haeun_telem_report *rep) {
    unsigned i;
    if (fp == NULL || rep == NULL)
        return;
    (void)fprintf(fp, "HaeunOS Process Telemetry\n");
    (void)fprintf(fp, "=========================\n\n");
    (void)fprintf(fp, "Global\n");
    (void)fprintf(fp, "------\n");
    (void)fprintf(fp, "curr_index       : %u\n", rep->curr_index);
    (void)fprintf(fp, "IDLE_FLAG        : %u\n", rep->idle_flag);
    (void)fprintf(fp, "timer_irqs       : %u\n", rep->timer_irqs);
    (void)fprintf(fp, "context_switches : %u\n\n", rep->context_switches);
    (void)fprintf(fp, "PCB Telemetry\n");
    (void)fprintf(fp, "-------------\n");
    for (i = 0; i < HAEUN_TELEM_NPROC; ++i) {
        const haeun_telem_slot *s = &rep->slots[i];
        (void)fprintf(fp, "\nSlot %u%s\n", s->slot,
                      s->as_slot_valid ? "" : " (no AS)");
        (void)fprintf(fp, "  PID             : %u\n", s->pid);
        (void)fprintf(fp, "  State           : %s (%u)\n",
                      haeun_telem_state_name(s->state), s->state);
        (void)fprintf(fp, "  Run ticks       : %u\n", s->run_ticks);
        (void)fprintf(fp, "  Ready wait      : %u\n", s->ready_wait_ticks);
        (void)fprintf(fp, "  Sleep ticks     : %u\n", s->sleep_ticks);
        (void)fprintf(fp, "  Blocked ticks   : %u\n", s->blocked_ticks);
        (void)fprintf(fp, "  Dispatch count  : %u\n", s->dispatch_count);
        if (s->complete_valid != 0u)
            (void)fprintf(fp, "  Completion      : %u\n", s->complete_lo);
        else
            (void)fprintf(fp, "  Completion      : N/A\n");
    }
    if (rep->note_count != 0u) {
        (void)fprintf(fp, "\nNotes\n-----\n");
        for (i = 0; i < rep->note_count; ++i) {
            const char *tag = "info";
            if (rep->note_kind[i] == HAEUN_TELEM_NOTE_EXPECTED)
                tag = "expected";
            else if (rep->note_kind[i] == HAEUN_TELEM_NOTE_WARN)
                tag = "warn";
            (void)fprintf(fp, "[%s] %s\n", tag, rep->notes[i]);
        }
    }
}

/**
 * Prints a compact JSON object for machine consumption.
 */
void haeun_telem_fprint_json(FILE *fp, const haeun_telem_report *rep) {
    unsigned i;
    if (fp == NULL || rep == NULL)
        return;
    (void)fprintf(fp,
                  "{\"curr_index\":%u,\"idle_flag\":%u,\"timer_irqs\":%u,"
                  "\"context_switches\":%u,\"slots\":[",
                  rep->curr_index, rep->idle_flag, rep->timer_irqs,
                  rep->context_switches);
    for (i = 0; i < HAEUN_TELEM_NPROC; ++i) {
        const haeun_telem_slot *s = &rep->slots[i];
        if (i != 0u)
            (void)fputc(',', fp);
        (void)fprintf(fp,
                      "{\"slot\":%u,\"pid\":%u,\"state\":%u,\"run_ticks\":%u,"
                      "\"ready_wait_ticks\":%u,\"sleep_ticks\":%u,"
                      "\"blocked_ticks\":%u,\"dispatch_count\":%u,"
                      "\"complete_lo\":%u,\"complete_valid\":%u,"
                      "\"as_slot_valid\":%d}",
                      s->slot, s->pid, s->state, s->run_ticks,
                      s->ready_wait_ticks, s->sleep_ticks, s->blocked_ticks,
                      s->dispatch_count, s->complete_lo, s->complete_valid,
                      s->as_slot_valid);
    }
    (void)fprintf(fp, "]}\n");
}
