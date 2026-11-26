#ifndef QSO_STATE_H
#define QSO_STATE_H

#include "config.h"

/*
 * Return values:
 *
 *  1: State loaded or saved successfully.
 *  0: State file absent (load only).
 * -1: Invalid state or I/O failure.
 */

int state_load(globe_config *cfg,
               const char *path);

int state_save(const globe_config *cfg,
               const char *path);

#endif
