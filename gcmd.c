/* gawk_cmd.c
 *
 * gawk extension that provides an input parser which runs a command against FILENAME
 * instead of just reading FILENAME directly
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "gcmd.h"

/* internal struct for keeping track of what we're doing */
typedef struct {
    char * cmdline;  /* what command(s) are being run? */
    FILE * cmdout;   /* file descriptor that points to stdin/stdout of cmdline */
    int oldfd;       /* hold onto the old file descriptor so awk has something to close */
} gcmd_state;


/* init_gcmd_state
 *   initialize and return a new gcmd_state object
 */
static inline gcmd_state * init_gcmd_state(void) {
    gcmd_state *state = NULL;

    state = malloc(sizeof *state);
    state->cmdline = NULL;
    state->cmdout = NULL;

    return state;
}

static inline void free_cmd_state(gcmd_state **state) {
    if (*state != NULL) {
        if ((*state)->cmdline != NULL) {
            free((*state)->cmdline);
            (*state)->cmdline = NULL;
        }

        if ((*state)->cmdout != NULL) {
            pclose((*state)->cmdout);
            (*state)->cmdout = NULL;
        }

        (*state)->oldfd = 0;
        free((*state));
        *state = NULL;
    }
}


/*
 * gcmd_init
 *  initialize our gawk extension and some global awk variables that we care about:
 *    - GCMD  the command to execute
 *    - GCRS  the replacement string
 */
static awk_bool_t gcmd_init(void) {
    awk_value_t default_gcmd;
    awk_value_t default_gcrs;

    /* register the input parser with gawk */
    register_input_parser(&gcmd_ip);

    /* set default values for our variables */
    make_const_string(GCMD_DEFAULT_GCMD, strlen(GCMD_DEFAULT_GCMD), &default_gcmd);
    sym_update("GCMD", &default_gcmd);

    make_const_string(GCMD_DEFAULT_GCRS, strlen(GCMD_DEFAULT_GCRS), &default_gcrs);
    sym_update("GCRS", &default_gcrs);

    /* everything went well */
    return awk_true;
}

static awk_bool_t (*init_func)(void) = gcmd_init;
dl_load_func(func_table, gcmd, "")


/*
 * gcmd_can_take_file
 *  if gcmd is set, and FILENAME is not "-", we can do this
 */
static awk_bool_t gcmd_can_take_file(const awk_input_buf_t *iobuf) {
    awk_value_t _gcmd;
    awk_bool_t rval = awk_false;

    // get the current value for gcmd
    sym_lookup("GCMD", AWK_STRING, &_gcmd);

    // TODO: support piping into gcmd
    if (_gcmd.str_value.len > 0 && (strcmp(iobuf->name, "-") != 0)) {
        rval = awk_true;
    }
    return rval;
}


/*
 * gcmd_take_control_of
 *  this is where we get ready to run a command against the file
 */
static awk_bool_t gcmd_take_control_of(awk_input_buf_t *iobuf) {
    gcmd_state *state = init_gcmd_state();
    awk_value_t _gcmd;
    awk_value_t _gcrs;
    char * pos = NULL;
    char * _cmdline = NULL;
    char * _cmdpos = NULL;
    char * _tmp = NULL;
    // memory allocation and copy progress stuff
    bool _gcrs_copied = false;
    size_t fnlen = strlen(iobuf->name);
    size_t cllen = 0;
    size_t clsize = 0;
    size_t sz = (size_t)sysconf(_SC_PAGESIZE);

    // get the value of GCMD and GCRS
    sym_lookup("GCMD", AWK_STRING, &_gcmd);
    sym_lookup("GCRS", AWK_STRING, &_gcrs);

    /* we could use str(n)cat here, and this is often going to be a trivial operation
     * but the possibility exists that it might not be
     * this implementation should be faster when cmdline is long or complex
     *
     * essentially what's going to happen here is:
     *   - iterate over GCMD, and copy characters one at a time into _cmdline
     *   - if _cmdline isn't big enough, realloc it with an additional page of memory
     *     I'm using pages instead of powers of 2 because it's unlikely (but not impossible) that we'll be
     *     doing this morethan once or twice anyway
     *   - if GCRS is a non-empty string and we come across a substring that exactly matches GCRS, copy in the contents of FILENAME instead
     *   - if we get to the end of GCMD but never did a GCRS replacement, add a space and FILENAME there
     *   - realloc the string again to fit the result
     */
    for (pos = (_gcmd.str_value.str); *pos != '\0'; pos++) {
        // allocate more memory for _cmdline if we need to
        if (cllen + fnlen + 1 >= clsize) {
            if ((_tmp = realloc(_cmdline, clsize + sz)) == NULL) {
                free(_cmdline);
                return awk_false;
            }

            clsize += sz;
            _cmdline = _tmp;
            _cmdpos = (_cmdline + cllen);
        }

        // check to see if we've hit a GCRS
        if (strncmp(pos, _gcrs.str_value.str, _gcrs.str_value.len) == 0) {
            strncpy(_cmdpos, iobuf->name, fnlen);
            cllen += fnlen;
            _cmdpos += fnlen;
            pos += _gcrs.str_value.len - 1;
            _gcrs_copied = true;
            continue;
        }


        // copy the next character from GCMD to _cmdline
        *_cmdpos++ = *pos;
        cllen++;

        // if we hit the end of the string, but never hit a GCRS, append a space followed by FILENAME
        // otherwise, append a NULL byte just in case
        if (_gcrs_copied == false && *(pos + 1) == '\0') {
            strncpy(_cmdpos++, " ", 1);
            strncpy(_cmdpos, iobuf->name, fnlen);
            _cmdpos += fnlen;
            cllen += fnlen;
        }
    }
    strncpy(_cmdpos, "\0", 1);
    cllen++;

    // try to realloc _cmdline down to fit our data
    if ((_tmp = realloc(_cmdline, cllen)) != NULL) {
        _cmdline = _tmp;
    }
    warning(ext_id, "_cmdline: '%s'\n", _cmdline);

    state->cmdline = _cmdline;


    // copy the existing fd to state
    /* TODO: if popen succeeds but sh exits immediately due to some error (e.g. command not found)
     *       we should detect that and return awk_false
     */
    state->oldfd = iobuf->fd;
    state->cmdout = popen(_cmdline, "r");
    if (state->cmdout == NULL) {
        free_cmd_state(&state);
        return awk_false;
    }

    iobuf->fd = fileno(state->cmdout);
    if (iobuf->fd == -1) {
        free_cmd_state(&state);
        return awk_false;
    }


    // now plug our stuff into iobuf then return true
    // because we replaced iobuf->fd with the fd we got from popen, we can just plug the system read function in here
    iobuf->read_func = read;
    iobuf->close_func = gcmd_close;
    iobuf->opaque = state;

    return awk_true;
}

/*
 * gcmd_close
 *   clean up after ourselves
 */
void gcmd_close(struct awk_input *iobuf) {
    gcmd_state *state = (gcmd_state *)iobuf->opaque;

    iobuf->fd = state->oldfd;
    free_cmd_state(&state);
}
