/* SPDX-License-Identifier: MIT */
/**
 * ucmdline.h - "micro-commandline". Miniature C89 command line parser 
 */

#ifndef UCMDLINE_H
#define UCMDLINE_H

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UCMDLINE_VERSION_MAJOR 0
#define UCMDLINE_VERSION_MINOR 2
#define UCMDLINE_VERSION_PATCH 0

/* Error Type */
typedef int ucmdline_err;

/* Error Code */
enum {
    UC_OK                  = 0,
    UC_ERR_UNKNOWN_OPT     = -1,  /* no matching spec; err_char/err_name set */
    UC_ERR_MISSING_VALUE   = -2,  /* required value absent at end of argv    */
    UC_ERR_OUT_OF_SPACE    = -3,  /* caller result buffers were too small    */
    UC_ERR_INVALID         = -4,  /* NULL result, NULL spec, or NULL argv    */
    UC_ERR_UNEXPECTED_VALUE = -5  /* value given for an option taking none   */
};

/* Argument Kind */
typedef enum {
    UC_ARG_NONE = 0,      /* flag: no value allowed                          */
    UC_ARG_REQUIRED,      /* value mandatory                                 */
    UC_ARG_OPTIONAL       /* value only when attached (--name=val / -oval)   */
} ucmdline_argkind;

typedef struct ucmdline_spec {
    const char *long_name;       /* long name without "--", or NULL          */
    char        short_name;      /* short char, or '\0'                      */
    ucmdline_argkind argkind;
    int         id;              /* caller-defined id reported in results    */
} ucmdline_spec;

/* Convenience constructors for spec table entries.
 * --- (example) ----
 *   static const ucmdline_spec spec[] = {
 *       UC_FLAG("verbose", 'v', OPT_VERBOSE),
 *       UC_REQ("output", 'o', OPT_OUTPUT),
 *       UC_OPT("level", 'l', OPT_LEVEL),
 *   };
 * ------------------
 */
 
#define UC_SPEC(longname, shortchar, kind, id) \
    { (longname), (shortchar), (kind), (id) }

#define UC_FLAG(longname, shortchar, id) \
    UC_SPEC((longname), (shortchar), UC_ARG_NONE, (id))
#define UC_FLAG_LONG(name, id) \
    UC_SPEC((name), '\0', UC_ARG_NONE, (id))
#define UC_FLAG_SHORT(c, id) \
    UC_SPEC(NULL, (c), UC_ARG_NONE, (id))

#define UC_REQ(longname, shortchar, id) \
    UC_SPEC((longname), (shortchar), UC_ARG_REQUIRED, (id))
#define UC_REQ_LONG(name, id) \
    UC_SPEC((name), '\0', UC_ARG_REQUIRED, (id))
#define UC_REQ_SHORT(c, id) \
    UC_SPEC(NULL, (c), UC_ARG_REQUIRED, (id))

#define UC_OPT(longname, shortchar, id) \
    UC_SPEC((longname), (shortchar), UC_ARG_OPTIONAL, (id))
#define UC_OPT_LONG(name, id) \
    UC_SPEC((name), '\0', UC_ARG_OPTIONAL, (id))
#define UC_OPT_SHORT(c, id) \
    UC_SPEC(NULL, (c), UC_ARG_OPTIONAL, (id))

#define UC_SPEC_LEN(a) (sizeof(a) / sizeof((a)[0]))

/* Result */
typedef struct ucmdline_opt {
    int         id;          /* id from the matching spec                    */
    const char *name;        /* long name from the spec, or NULL for short   */
    char        short_name;  /* short option char, or '\0' for long          */
    unsigned char src;       /* how this occurrence was written (UC_SRC_*)   */
    const char *value;       /* value pointer into argv, or NULL if none     */
} ucmdline_opt;

/* Origin of a matched option occurrence. */
typedef enum {
    UC_SRC_SHORT = 0,   /* -x or bundled -xyz                           */
    UC_SRC_LONG,        /* --name or --name=value                       */
    UC_SRC_ASSIGN       /* bare key=value (UC_PARSE_ASSIGN mode)        */
} ucmdline_src;   

typedef struct ucmdline_result {
    unsigned flags;               /* parse-mode flags (UC_PARSE_*)              */

    ucmdline_opt  *opts;          /* receives the first opts_len occurrences    */
    size_t         opts_cap;      /* capacity of opts[]                         */
    size_t         opts_len;      /* number of entries stored in opts[]         */
    size_t         opts_total;    /* total matched occurrences (incl. unstored) */

    const char   **positional;    /* positional arguments, in order             */
    size_t         positional_cap;
    size_t         positional_len;
    size_t         positional_total;

    ucmdline_err   err;
    size_t         err_index;      /* argv index of the offending token       */
    char           err_char;       /* offending short char (unknown/missing)  */
    const char   * err_name;       /* offending long token (unknown/missing)  */
} ucmdline_result;

/* Parse-mode flags for ucmdline_result.flags. */
#define UC_PARSE_ASSIGN 1u  

/* Zeroes every field of res. Call before wiring output buffers. */
void ucmdline_result_init(ucmdline_result *res);

/* Parses argv[1..argc-1] against spec[0..nspec-1] writing into res. */
ucmdline_err ucmdline_parse(const ucmdline_spec *spec, size_t nspec,
                            int argc, char *const argv[],
                            ucmdline_result *res);


/* True if at least one occurrence of id was parsed. */
bool ucmdline_has(const ucmdline_result *res, int id);

/* First occurrence of id, or NULL. */
const ucmdline_opt *ucmdline_find(const ucmdline_result *res, int id);

/* Number of occurrences of id that were parsed and stored. */
size_t ucmdline_count(const ucmdline_result *res, int id);

/* Iterates every stored occurrence in parse order. */
typedef void (*ucmdline_visit_fn)(void *ctx, const ucmdline_opt *opt);
void ucmdline_each(const ucmdline_result *res, ucmdline_visit_fn fn,
                   void *ctx);

#ifdef __cplusplus
}
#endif

#endif /* UCMDLINE_H */
