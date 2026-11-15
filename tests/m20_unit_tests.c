#include "haeun/evt.h"
#include "haeun/ipc.h"
#include "haeun/svc.h"

#include <stdio.h>
#include <string.h>

static int fails;

static void expect(int cond, const char *msg) {
	if (!cond) {
		(void)fprintf(stderr, "FAIL: %s\n", msg);
		fails++;
	}
}

static void test_evt_isolation(void) {
	haeun_evt_hub hub;
	haeun_evt ev, out;
	haeun_evt_hub_init(&hub, NULL);
	memset(&ev, 0, sizeof(ev));
	ev.type = HAEUN_EVT_KEY_DOWN;
	ev.a = 0x1eu;
	expect(haeun_evt_push(&hub, 2, &ev) == 0, "push a");
	ev.a = 0x2eu;
	expect(haeun_evt_push(&hub, 3, &ev) == 0, "push b");
	expect(haeun_evt_poll(&hub, 3, &out) == 1, "b poll");
	expect(out.a == 0x2eu, "b key");
	expect(haeun_evt_poll(&hub, 2, &out) == 1, "a poll");
	expect(out.a == 0x1eu, "a key");
	expect(haeun_evt_poll(&hub, 2, &out) == 0, "a empty");
	haeun_evt_reap(&hub, 2);
	expect(haeun_evt_poll(&hub, 2, &out) == 0, "reaped");
}

static void test_evt_overflow(void) {
	haeun_evt_hub hub;
	haeun_evt ev, out;
	int i;
	haeun_evt_hub_init(&hub, NULL);
	memset(&ev, 0, sizeof(ev));
	ev.type = HAEUN_EVT_KEY_DOWN;
	for (i = 0; i < HAEUN_EVT_QUEUE + 3; ++i) {
		ev.a = (uint32_t)i;
		expect(haeun_evt_push(&hub, 1, &ev) == 0, "overflow push");
	}
	expect(hub.slots[0].drops >= 3u, "drops counted");
	expect(haeun_evt_poll(&hub, 1, &out) == 1, "poll after overflow");
	expect(out.a == 3u, "oldest dropped");
}

static void test_ipc(void) {
	haeun_ipc_hub hub;
	uint8_t msg[8] = { 'h', 'i', 0 };
	uint8_t out[8];
	int epid, sender = -1, n;
	haeun_ipc_hub_init(&hub);
	epid = haeun_ipc_create(&hub, 2);
	expect(epid >= 0, "create");
	expect(haeun_ipc_send(&hub, epid, 3, msg, 2) == 0, "send");
	expect(haeun_ipc_recv(&hub, epid, 3, out, 8, &sender) < 0, "foreign recv");
	n = haeun_ipc_recv(&hub, epid, 2, out, 8, &sender);
	expect(n == 2 && sender == 3 && out[0] == 'h', "recv ok");
	expect(haeun_ipc_send(&hub, 99, 3, msg, 2) < 0, "bad epid");
	expect(haeun_ipc_send(&hub, epid, 3, msg, 100) == -3, "oversize");
	haeun_ipc_reap(&hub, 2);
	expect(haeun_ipc_recv(&hub, epid, 2, out, 8, &sender) < 0, "reaped");
}

static void test_svc(void) {
	haeun_svc_hub hub;
	haeun_svc_hub_init(&hub);
	expect(haeun_svc_register(&hub, 1, "echo", 0) == 0, "reg");
	expect(haeun_svc_register(&hub, 2, "echo", 1) < 0, "dup");
	expect(haeun_svc_lookup(&hub, "echo") == 0, "lookup");
	haeun_svc_reap(&hub, 1);
	expect(haeun_svc_lookup(&hub, "echo") < 0, "reaped svc");
}

int main(void) {
	test_evt_isolation();
	test_evt_overflow();
	test_ipc();
	test_svc();
	if (fails != 0) {
		(void)fprintf(stderr, "m20_unit_tests: %d failures\n", fails);
		return 1;
	}
	(void)printf("m20_unit_tests ok\n");
	return 0;
}
