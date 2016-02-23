/* gawk_cmd.h
 *
 */


#ifndef GCMD_GCMD_H
#define GCMD_GCMD_H

/* -- defines -- */
#define GCMD_DEFAULT_GCMD ""
#define GCMD_DEFAULT_GCRS "{}"

#define GCMD_VERSION "GCMD extension: version 1.0"

// shortcut for just __unused
#ifndef __GNUC__
#define __attribute__(A)
#endif
#define __UNUSED __attribute__ (( __unused__ ))


/* -- gawk api boilerplate -- */

// these imports are required for gawkapi.h to work correctly
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

// last but not least, gawkapi.h
#include <gawkapi.h>

/* now that we have imports out of the way, we do still need some other boilerplate */
int plugin_is_GPL_compatible;

static const gawk_api_t *api;
static awk_ext_id_t ext_id;
static const char ext_version[] = GCMD_VERSION;

/* table of functions exported to gawk */
static awk_ext_func_t func_table[] = {0};

/* init function */
static awk_bool_t gcmd_init(void);


/* functions for the gcmd input parser */
static awk_bool_t gcmd_can_take_file(const awk_input_buf_t *iobuf);
static awk_bool_t gcmd_take_control_of(awk_input_buf_t *iobuf);


/* tell gawk that we have an input parser */
static awk_input_parser_t gcmd_ip = {
        "gawk_cmd",
        gcmd_can_take_file,
        gcmd_take_control_of,
        NULL
};

/* -- other functions -- */
void gcmd_close(struct awk_input *iobuf);

#endif //GCMD_GCMD_H