#ifndef HAEUN_EVAL_WORKLOAD_H
#define HAEUN_EVAL_WORKLOAD_H

#include "haeun/eval_types.h"

/**
 * Fills the static catalog entry for kind. Returns HAEUN_EVAL_NO_WORKLOAD
 * for unknown kinds.
 */
haeun_eval_result haeun_eval_workload_get(haeun_eval_wl_kind kind,
                                          haeun_eval_workload *out);

/**
 * Looks up a workload by name (e.g. "cpu_bound").
 */
haeun_eval_result haeun_eval_workload_by_name(const char *name,
                                              haeun_eval_workload *out);

/**
 * Builds absolute HXF path: "<dir>/<image_name>.hxf".
 */
haeun_eval_result haeun_eval_workload_image_path(const haeun_eval_workload *wl,
                                                 const char *build_dir,
                                                 char *path, size_t path_cap);

/**
 * Returns 1 when kind is in the supported catalog.
 */
int haeun_eval_workload_supported(haeun_eval_wl_kind kind);

#endif
