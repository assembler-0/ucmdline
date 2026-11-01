/* SPDX-License-Identifier: MIT */
/**
 * ucmdline.h - "micro-commandline". Miniature C89 command line parser 
 */
 
#include "ucmdline.h"

static const ucmdline_spec *find_long(const ucmdline_spec *spec,
                                      size_t nspec,
                                      const char *name, size_t namelen)
{
    size_t k;

    for (k = 0; k < nspec; ++k) {
        const char *ln = spec[k].long_name;
        if (ln != NULL && strlen(ln) == namelen &&
            strncmp(ln, name, namelen) == 0)
            return &spec[k];
    }
    return NULL;
}

static const ucmdline_spec *find_short(const ucmdline_spec *spec,
                                       size_t nspec, char c)
{
    size_t k;

    for (k = 0; k < nspec; ++k) {
        if (spec[k].short_name != '\0' && spec[k].short_name == c)
            return &spec[k];
    }
    return NULL;
}

static void record_opt(ucmdline_result *res, int id,
                       const char *name, char short_name,
                       unsigned char src, const char *value)
{
    res->opts_total += 1;
    if (res->opts != NULL && res->opts_len < res->opts_cap) {
        res->opts[res->opts_len].id = id;
        res->opts[res->opts_len].name = name;
        res->opts[res->opts_len].short_name = short_name;
        res->opts[res->opts_len].src = src;
        res->opts[res->opts_len].value = value;
        res->opts_len += 1;
    }
}

static void record_positional(ucmdline_result *res, const char *arg)
{
    res->positional_total += 1;
    if (res->positional != NULL &&
        res->positional_len < res->positional_cap) {
        res->positional[res->positional_len] = arg;
        res->positional_len += 1;
    }
}

static ucmdline_err handle_long(const ucmdline_spec *spec, size_t nspec,
                                char *token, int *idx, int argc,
                                char *const argv[], ucmdline_result *res)
{
    const ucmdline_spec *sp;
    const char *name = token + 2;
    const char *value = NULL;
    const char *eq = strchr(token, '=');
    size_t namelen;

    if (eq != NULL) {
        namelen = (size_t)(eq - name);
        value = eq + 1;
    } else {
        namelen = strlen(name);
    }

    sp = find_long(spec, nspec, name, namelen);
    if (sp == NULL) {
        res->err = UC_ERR_UNKNOWN_OPT;
        res->err_index = (size_t)*idx;
        res->err_name = token;
        return res->err;
    }

    if (sp->argkind == UC_ARG_NONE) {
        if (value != NULL) {
            res->err = UC_ERR_UNEXPECTED_VALUE;
            res->err_index = (size_t)*idx;
            res->err_name = token;
            return res->err;
        }
    } else if (sp->argkind == UC_ARG_REQUIRED) {
        if (value == NULL) {
            if (*idx + 1 >= argc) {
                res->err = UC_ERR_MISSING_VALUE;
                res->err_index = (size_t)*idx;
                res->err_name = token;
                return res->err;
            }
            value = argv[*idx + 1];
            *idx += 1;
        }
    }

    record_opt(res, sp->id, sp->long_name, '\0', UC_SRC_LONG, value);
    return UC_OK;
}

static int handle_assign(const ucmdline_spec *spec, size_t nspec,
                         char *token, int idx, ucmdline_result *res)
{
    const ucmdline_spec *sp;
    const char *eq = strchr(token, '=');
    const char *value;
    size_t namelen;

    if (eq == NULL)
        return 1;

    namelen = (size_t)(eq - token);
    value = eq + 1;

    sp = find_long(spec, nspec, token, namelen);
    if (sp == NULL)
        return 1;

    if (sp->argkind == UC_ARG_NONE) {
        res->err = UC_ERR_UNEXPECTED_VALUE;
        res->err_index = (size_t)idx;
        res->err_name = token;
        return res->err;
    }

    record_opt(res, sp->id, sp->long_name, '\0', UC_SRC_ASSIGN, value);
    return 0;
}

static ucmdline_err handle_short(const ucmdline_spec *spec, size_t nspec,
                                 char *token, int *idx, int argc,
                                 char *const argv[], ucmdline_result *res)
{
    size_t j = 1;

    while (token[j] != '\0') {
        const ucmdline_spec *sp = find_short(spec, nspec, token[j]);

        if (sp == NULL) {
            res->err = UC_ERR_UNKNOWN_OPT;
            res->err_index = (size_t)*idx;
            res->err_char = token[j];
            return res->err;
        }

        if (sp->argkind == UC_ARG_NONE) {
            record_opt(res, sp->id, NULL, token[j], UC_SRC_SHORT, NULL);
            j += 1;
        } else {
            const char *value = NULL;

            if (token[j + 1] != '\0') {
                value = &token[j + 1];
            } else if (sp->argkind == UC_ARG_REQUIRED) {
                if (*idx + 1 >= argc) {
                    res->err = UC_ERR_MISSING_VALUE;
                    res->err_index = (size_t)*idx;
                    res->err_char = token[j];
                    return res->err;
                }
                value = argv[*idx + 1];
                *idx += 1;
            }

            record_opt(res, sp->id, NULL, token[j], UC_SRC_SHORT, value);
            break;
        }
    }

    return UC_OK;
}

void ucmdline_result_init(ucmdline_result *res)
{
    if (res != NULL)
        memset(res, 0, sizeof(*res));
}

static void reset_bookkeeping(ucmdline_result *res)
{
    res->opts_len = 0;
    res->opts_total = 0;
    res->positional_len = 0;
    res->positional_total = 0;
    res->err = UC_OK;
    res->err_index = 0;
    res->err_char = '\0';
    res->err_name = NULL;
}

ucmdline_err ucmdline_parse(const ucmdline_spec *spec, size_t nspec,
                            int argc, char *const argv[],
                            ucmdline_result *res)
{
    int i;
    bool stop = false;

    if (res == NULL || (spec == NULL && nspec > 0) ||
        (argv == NULL && argc > 1)) {
        if (res != NULL) {
            reset_bookkeeping(res);
            res->err = UC_ERR_INVALID;
        }
        return UC_ERR_INVALID;
    }

    reset_bookkeeping(res);

    if (argc <= 1)
        return UC_OK;

    for (i = 1; i < argc; ++i) {
        char *arg = argv[i];

        if (stop || arg[0] != '-' || arg[1] == '\0') {
            if (!stop && (res->flags & UC_PARSE_ASSIGN)) {
                int ar = handle_assign(spec, nspec, arg, i, res);
                if (ar == 0)
                    continue;
                if (ar < 0)
                    return res->err;
            }
            record_positional(res, arg);
            continue;
        }

        if (arg[1] == '-') {
            if (arg[2] == '\0') {
                stop = true;
                continue;
            }
            if (handle_long(spec, nspec, arg, &i, argc, argv, res) != UC_OK)
                return res->err;
        } else {
            if (handle_short(spec, nspec, arg, &i, argc, argv, res) != UC_OK)
                return res->err;
        }
    }

    if (res->opts_total > res->opts_len ||
        res->positional_total > res->positional_len) {
        res->err = UC_ERR_OUT_OF_SPACE;
        return res->err;
    }

    return UC_OK;
}

bool ucmdline_has(const ucmdline_result *res, int id)
{
    return ucmdline_find(res, id) != NULL;
}

const ucmdline_opt *ucmdline_find(const ucmdline_result *res, int id)
{
    size_t k;

    if (res == NULL)
        return NULL;

    for (k = 0; k < res->opts_len; ++k) {
        if (res->opts[k].id == id)
            return &res->opts[k];
    }
    return NULL;
}

size_t ucmdline_count(const ucmdline_result *res, int id)
{
    size_t k;
    size_t n = 0;

    if (res == NULL)
        return 0;

    for (k = 0; k < res->opts_len; ++k) {
        if (res->opts[k].id == id)
            n += 1;
    }
    return n;
}

void ucmdline_each(const ucmdline_result *res, ucmdline_visit_fn fn,
                   void *ctx)
{
    size_t k;

    if (res == NULL || fn == NULL)
        return;

    for (k = 0; k < res->opts_len; ++k)
        fn(ctx, &res->opts[k]);
}
