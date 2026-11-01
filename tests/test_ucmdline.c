/*
 * test_ucmdline.c - test for ucmdline
 */

#include <stdio.h>
#include "ucmdline.h"

static int g_pass = 0;
static int g_fail = 0;

static void check_(int cond, const char *tag, const char *file, int line)
{
    if (cond) {
        g_pass += 1;
    } else {
        g_fail += 1;
        printf("FAIL [%s] %s:%d\n", tag, file, line);
    }
}

#define CHECK(tag, cond) check_((cond), (tag), __FILE__, __LINE__)

enum {
    OPT_VERBOSE = 1,
    OPT_QUIET,
    OPT_OUTPUT,
    OPT_LEVEL,
    OPT_ALPHA,
    OPT_X,
    OPT_DEBUG
};

static const ucmdline_spec spec_table[] = {
    UC_FLAG("verbose", 'v', OPT_VERBOSE),
    UC_FLAG("quiet", 'q', OPT_QUIET),
    UC_REQ("output", 'o', OPT_OUTPUT),
    UC_OPT("level", 'l', OPT_LEVEL),
    UC_REQ_LONG("alpha", OPT_ALPHA),
    UC_REQ_SHORT('x', OPT_X),
    UC_FLAG("debug", 'd', OPT_DEBUG)
};

#define NSPEC UC_SPEC_LEN(spec_table)

typedef struct setup {
    ucmdline_opt opts[64];
    const char *pos[64];
    ucmdline_result res;
} setup;

static void run_parse_f(setup *s, char *const argv[], int argc, unsigned flags)
{
    ucmdline_result_init(&s->res);
    s->res.flags = flags;
    s->res.opts = s->opts;
    s->res.opts_cap = 64;
    s->res.positional = s->pos;
    s->res.positional_cap = 64;
    (void)ucmdline_parse(spec_table, NSPEC, argc, argv, &s->res);
}

static void run_parse(setup *s, char *const argv[], int argc)
{
    run_parse_f(s, argv, argc, 0u);
}

static void test_init_zeroes(void)
{
    ucmdline_result r;

    memset(&r, 0xAA, sizeof(r));
    ucmdline_result_init(&r);
    CHECK("init", r.err == UC_OK);
    CHECK("init", r.flags == 0);
    CHECK("init", r.opts == NULL);
    CHECK("init", r.opts_cap == 0);
    CHECK("init", r.opts_len == 0);
    CHECK("init", r.opts_total == 0);
    CHECK("init", r.positional == NULL);
    CHECK("init", r.positional_cap == 0);
    CHECK("init", r.positional_len == 0);
    CHECK("init", r.positional_total == 0);
    CHECK("init", r.err_index == 0);
    CHECK("init", r.err_char == '\0');
    CHECK("init", r.err_name == NULL);
}

static void test_basic_flags(void)
{
    setup s;
    char *av[] = { "prog", "-v", "--quiet", "-qd", "-v" };

    run_parse(&s, av, 5);
    CHECK("basic", s.res.err == UC_OK);
    CHECK("basic", s.res.opts_len == 5);
    CHECK("basic", s.res.opts_total == 5);
    CHECK("basic", s.res.positional_len == 0);
    CHECK("basic", ucmdline_count(&s.res, OPT_VERBOSE) == 2);
    CHECK("basic", ucmdline_count(&s.res, OPT_QUIET) == 2);
    CHECK("basic", ucmdline_count(&s.res, OPT_DEBUG) == 1);
    CHECK("basic", ucmdline_has(&s.res, OPT_DEBUG) == true);
    CHECK("basic", ucmdline_has(&s.res, OPT_ALPHA) == false);
    CHECK("basic", s.res.opts[0].short_name == 'v');
    CHECK("basic", s.res.opts[0].name == NULL);
    CHECK("basic", s.res.opts[0].value == NULL);
    CHECK("basic", s.res.opts[1].short_name == '\0');
    CHECK("basic", strcmp(s.res.opts[1].name, "quiet") == 0);
    CHECK("basic", s.res.opts[1].value == NULL);
    CHECK("basic", s.res.opts[2].short_name == 'q');
    CHECK("basic", s.res.opts[3].short_name == 'd');
    CHECK("basic", s.res.opts[4].short_name == 'v');
}

static void test_empty_and_only_prog(void)
{
    setup s;
    char *av0[] = { "prog" };
    char *av1[] = { "prog", "-" };
    char *av2[] = { "prog", "" };
    ucmdline_result r;

    run_parse(&s, av0, 1);
    CHECK("empty", s.res.err == UC_OK);
    CHECK("empty", s.res.opts_len == 0);
    CHECK("empty", s.res.positional_len == 0);

    run_parse(&s, av1, 2);
    CHECK("dash", s.res.err == UC_OK);
    CHECK("dash", s.res.opts_len == 0);
    CHECK("dash", s.res.positional_len == 1);
    CHECK("dash", s.res.positional[0] == av1[1]);

    run_parse(&s, av2, 2);
    CHECK("emptyarg", s.res.err == UC_OK);
    CHECK("emptyarg", s.res.opts_len == 0);
    CHECK("emptyarg", s.res.positional_len == 1);
    CHECK("emptyarg", s.res.positional[0] == av2[1]);

    ucmdline_result_init(&r);
    CHECK("zero", ucmdline_parse(spec_table, NSPEC, 0, NULL, &r) == UC_OK);
    CHECK("zero", r.err == UC_OK);
    CHECK("zero", r.opts_len == 0);
}

static void test_bundling(void)
{
    setup s;
    char *av[] = { "prog", "-vqd" };

    run_parse(&s, av, 2);
    CHECK("bundle", s.res.err == UC_OK);
    CHECK("bundle", s.res.opts_len == 3);
    CHECK("bundle", s.res.opts[0].short_name == 'v');
    CHECK("bundle", s.res.opts[1].short_name == 'q');
    CHECK("bundle", s.res.opts[2].short_name == 'd');
    CHECK("bundle", s.res.opts[2].id == OPT_DEBUG);
}

static void test_short_values(void)
{
    setup s;

    {
        char *av[] = { "prog", "-o", "file.txt", "-ox", "-o=z" };
        run_parse(&s, av, 5);
        CHECK("shortval", s.res.err == UC_OK);
        CHECK("shortval", s.res.opts_len == 3);
        CHECK("shortval", s.res.opts[0].id == OPT_OUTPUT);
        CHECK("shortval", s.res.opts[0].value == av[2]);
        CHECK("shortval", s.res.opts[1].value == av[3] + 2);
        CHECK("shortval", strcmp(s.res.opts[2].value, "=z") == 0);
    }

    {
        char *av2[] = { "prog", "-o", "--verbose" };
        run_parse(&s, av2, 3);
        CHECK("consume", s.res.err == UC_OK);
        CHECK("consume", s.res.opts_len == 1);
        CHECK("consume", s.res.opts[0].id == OPT_OUTPUT);
        CHECK("consume", s.res.opts[0].value == av2[2]);
        CHECK("consume", ucmdline_count(&s.res, OPT_VERBOSE) == 0);
        CHECK("consume", s.res.positional_len == 0);
    }

    {
        char *av3[] = { "prog", "-xl" };
        run_parse(&s, av3, 2);
        CHECK("attached", s.res.err == UC_OK);
        CHECK("attached", s.res.opts_len == 1);
        CHECK("attached", s.res.opts[0].id == OPT_X);
        CHECK("attached", strcmp(s.res.opts[0].value, "l") == 0);
    }

    {
        char *av4[] = { "prog", "-v", "-o" };
        run_parse(&s, av4, 3);
        CHECK("attached2", s.res.err == UC_ERR_MISSING_VALUE);
        CHECK("attached2", s.res.err_index == 2);
        CHECK("attached2", s.res.err_char == 'o');
        CHECK("attached2", s.res.opts_len == 1);
        CHECK("attached2", s.res.opts[0].short_name == 'v');
    }
}

static void test_long_values(void)
{
    setup s;

    {
        char *av[] = { "prog", "--output=log.txt", "--output",
                       "stdout", "--level=x" };
        run_parse(&s, av, 5);
        CHECK("longval", s.res.err == UC_OK);
        CHECK("longval", s.res.opts_len == 3);
        CHECK("longval", s.res.opts[0].id == OPT_OUTPUT);
        CHECK("longval", strcmp(s.res.opts[0].value, "log.txt") == 0);
        CHECK("longval", s.res.opts[1].value == av[3]);
        CHECK("longval", s.res.opts[2].id == OPT_LEVEL);
        CHECK("longval", strcmp(s.res.opts[2].value, "x") == 0);
        CHECK("longval", strcmp(s.res.opts[0].name, "output") == 0);
        CHECK("longval", s.res.opts[0].short_name == '\0');
    }

    {
        char *av2[] = { "prog", "--output=" };
        run_parse(&s, av2, 2);
        CHECK("emptyval", s.res.err == UC_OK);
        CHECK("emptyval", s.res.opts_len == 1);
        CHECK("emptyval", s.res.opts[0].value != NULL);
        CHECK("emptyval", *(s.res.opts[0].value) == '\0');
        CHECK("emptyval", s.res.opts[0].value == av2[1] + 9);
    }

    {
        char *av3[] = { "prog", "--alpha=zz", "--alpha", "aa" };
        run_parse(&s, av3, 4);
        CHECK("alpha", s.res.err == UC_OK);
        CHECK("alpha", s.res.opts_len == 2);
        CHECK("alpha", strcmp(s.res.opts[0].value, "zz") == 0);
        CHECK("alpha", s.res.opts[1].value == av3[3]);
    }
}

static void test_optional_args(void)
{
    setup s;
    char *av[] = { "prog", "--level=3", "--level", "-l5", "-l", "-l=7" };

    run_parse(&s, av, 6);
    CHECK("opt", s.res.err == UC_OK);
    CHECK("opt", s.res.opts_len == 5);
    CHECK("opt", s.res.opts[0].id == OPT_LEVEL);
    CHECK("opt", strcmp(s.res.opts[0].value, "3") == 0);
    CHECK("opt", s.res.opts[1].value == NULL);
    CHECK("opt", strcmp(s.res.opts[2].value, "5") == 0);
    CHECK("opt", s.res.opts[3].value == NULL);
    CHECK("opt", strcmp(s.res.opts[4].value, "=7") == 0);

    {
        char *av4[] = { "prog", "--level=" };
        run_parse(&s, av4, 2);
        CHECK("opt-empty", s.res.err == UC_OK);
        CHECK("opt-empty", s.res.opts_len == 1);
        CHECK("opt-empty", s.res.opts[0].value != NULL);
        CHECK("opt-empty", *(s.res.opts[0].value) == '\0');
    }
}

static void test_double_dash(void)
{
    setup s;
    char *av[] = { "prog", "-v", "--", "-q", "--output", "plain", "-" };

    run_parse(&s, av, 7);
    CHECK("dd", s.res.err == UC_OK);
    CHECK("dd", s.res.opts_len == 1);
    CHECK("dd", s.res.opts[0].short_name == 'v');
    CHECK("dd", s.res.positional_len == 4);
    CHECK("dd", s.res.positional_total == 4);
    CHECK("dd", s.res.positional[0] == av[3]);
    CHECK("dd", s.res.positional[1] == av[4]);
    CHECK("dd", s.res.positional[2] == av[5]);
    CHECK("dd", s.res.positional[3] == av[6]);
}

static void test_mixed(void)
{
    setup s;
    char *av[] = { "prog", "f1", "-v", "f2", "-o", "out", "f3",
                   "--alpha", "a1" };

    run_parse(&s, av, 9);
    CHECK("mixed", s.res.err == UC_OK);
    CHECK("mixed", s.res.opts_len == 3);
    CHECK("mixed", s.res.positional_len == 3);
    CHECK("mixed", s.res.positional[0] == av[1]);
    CHECK("mixed", s.res.positional[1] == av[3]);
    CHECK("mixed", s.res.positional[2] == av[6]);
    CHECK("mixed", s.res.opts[0].id == OPT_VERBOSE);
    CHECK("mixed", s.res.opts[1].id == OPT_OUTPUT);
    CHECK("mixed", s.res.opts[1].value == av[5]);
    CHECK("mixed", s.res.opts[2].id == OPT_ALPHA);
    CHECK("mixed", s.res.opts[2].value == av[8]);
}

static void test_errors(void)
{
    setup s;

    {
        char *av[] = { "prog", "--nope" };
        run_parse(&s, av, 2);
        CHECK("err unknown long", s.res.err == UC_ERR_UNKNOWN_OPT);
        CHECK("err unknown long", s.res.err_index == 1);
        CHECK("err unknown long", s.res.err_name == av[1]);
        CHECK("err unknown long", s.res.opts_len == 0);
    }
    {
        char *av[] = { "prog", "--nope=x" };
        run_parse(&s, av, 2);
        CHECK("err unknown longv", s.res.err == UC_ERR_UNKNOWN_OPT);
        CHECK("err unknown longv", s.res.err_name == av[1]);
    }
    {
        char *av[] = { "prog", "-z" };
        run_parse(&s, av, 2);
        CHECK("err unknown short", s.res.err == UC_ERR_UNKNOWN_OPT);
        CHECK("err unknown short", s.res.err_char == 'z');
        CHECK("err unknown short", s.res.err_index == 1);
    }
    {
        char *av[] = { "prog", "-vbz" };
        run_parse(&s, av, 2);
        CHECK("err bundle char", s.res.err == UC_ERR_UNKNOWN_OPT);
        CHECK("err bundle char", s.res.err_char == 'b');
        CHECK("err bundle char", s.res.err_index == 1);
        CHECK("err bundle char", s.res.opts_len == 1);
        CHECK("err bundle char", s.res.opts[0].short_name == 'v');
    }
    {
        char *av[] = { "prog", "-v=1" };
        run_parse(&s, av, 2);
        CHECK("err flag eq", s.res.err == UC_ERR_UNKNOWN_OPT);
        CHECK("err flag eq", s.res.err_char == '=');
    }
    {
        char *av[] = { "prog", "--verbose=yes" };
        run_parse(&s, av, 2);
        CHECK("err unexpected", s.res.err == UC_ERR_UNEXPECTED_VALUE);
        CHECK("err unexpected", s.res.err_index == 1);
        CHECK("err unexpected", s.res.err_name == av[1]);
    }
    {
        char *av[] = { "prog", "-o" };
        run_parse(&s, av, 2);
        CHECK("err missing short", s.res.err == UC_ERR_MISSING_VALUE);
        CHECK("err missing short", s.res.err_char == 'o');
        CHECK("err missing short", s.res.err_index == 1);
    }
    {
        char *av[] = { "prog", "--output" };
        run_parse(&s, av, 2);
        CHECK("err missing long", s.res.err == UC_ERR_MISSING_VALUE);
        CHECK("err missing long", s.res.err_name == av[1]);
        CHECK("err missing long", s.res.err_index == 1);
    }
    {
        char *av[] = { "prog", "-v", "-o" };
        run_parse(&s, av, 3);
        CHECK("err after flag", s.res.err == UC_ERR_MISSING_VALUE);
        CHECK("err after flag", s.res.err_index == 2);
        CHECK("err after flag", s.res.opts_len == 1);
        CHECK("err after flag", s.res.opts[0].short_name == 'v');
    }
    {
        char *av[] = { "prog", "---x" };
        run_parse(&s, av, 2);
        CHECK("err triple dash", s.res.err == UC_ERR_UNKNOWN_OPT);
        CHECK("err triple dash", s.res.err_name == av[1]);
    }
    {
        char *av[] = { "prog", "--=x" };
        run_parse(&s, av, 2);
        CHECK("err empty long", s.res.err == UC_ERR_UNKNOWN_OPT);
    }
    {
        char *av[] = { "prog", "-q", "-w" };
        run_parse(&s, av, 3);
        CHECK("err second", s.res.err == UC_ERR_UNKNOWN_OPT);
        CHECK("err second", s.res.err_char == 'w');
        CHECK("err second", s.res.opts_len == 1);
    }
}

static void test_overflow(void)
{
    ucmdline_opt small[2];
    const char *poss[1];
    ucmdline_result res;
    char *av[] = { "prog", "-v", "-q", "-d", "p1", "p2" };

    ucmdline_result_init(&res);
    res.opts = small;
    res.opts_cap = 2;
    res.positional = poss;
    res.positional_cap = 1;

    CHECK("of", ucmdline_parse(spec_table, NSPEC, 6, av, &res) ==
          UC_ERR_OUT_OF_SPACE);
    CHECK("of", res.err == UC_ERR_OUT_OF_SPACE);
    CHECK("of", res.opts_len == 2);
    CHECK("of", res.opts_total == 3);
    CHECK("of", res.positional_len == 1);
    CHECK("of", res.positional_total == 2);
    CHECK("of", res.opts[0].short_name == 'v');
    CHECK("of", res.opts[1].short_name == 'q');
    CHECK("of", res.positional[0] == av[4]);

    {
        ucmdline_result r2;
        ucmdline_result_init(&r2);
        r2.opts = NULL;
        r2.opts_cap = 0;
        r2.positional = NULL;
        r2.positional_cap = 0;
        CHECK("of-count", ucmdline_parse(spec_table, NSPEC, 6, av, &r2) ==
              UC_ERR_OUT_OF_SPACE);
        CHECK("of-count", r2.opts_len == 0);
        CHECK("of-count", r2.opts_total == 3);
        CHECK("of-count", r2.positional_len == 0);
        CHECK("of-count", r2.positional_total == 2);
    }
}

struct ctx_sum {
    int sum;
};

static void visit_sum(void *ctx, const ucmdline_opt *opt)
{
    ((struct ctx_sum *)ctx)->sum += opt->id;
}

static void test_helpers(void)
{
    struct ctx_sum c;
    setup s;
    char *av[] = { "prog", "-v", "--output=x", "-v", "-q" };

    run_parse(&s, av, 5);
    CHECK("h", s.res.err == UC_OK);
    CHECK("h", ucmdline_has(&s.res, OPT_VERBOSE) == true);
    CHECK("h", ucmdline_has(&s.res, OPT_OUTPUT) == true);
    CHECK("h", ucmdline_has(&s.res, OPT_DEBUG) == false);
    CHECK("h", ucmdline_count(&s.res, OPT_VERBOSE) == 2);
    CHECK("h", ucmdline_count(&s.res, OPT_QUIET) == 1);
    CHECK("h", ucmdline_count(&s.res, OPT_DEBUG) == 0);
    CHECK("h", ucmdline_find(&s.res, OPT_VERBOSE) == &s.res.opts[0]);
    CHECK("h", ucmdline_find(&s.res, OPT_OUTPUT) == &s.res.opts[1]);
    CHECK("h", ucmdline_find(&s.res, OPT_DEBUG) == NULL);
    CHECK("h", ucmdline_find(NULL, 1) == NULL);
    CHECK("h", ucmdline_count(NULL, 1) == 0);

    c.sum = 0;
    ucmdline_each(&s.res, visit_sum, &c);
    CHECK("h", c.sum == OPT_VERBOSE + OPT_OUTPUT + OPT_VERBOSE + OPT_QUIET);

    c.sum = 0;
    ucmdline_each(NULL, visit_sum, &c);
    CHECK("h", c.sum == 0);
    ucmdline_each(&s.res, NULL, &c);

    CHECK("h", s.res.opts[2].id == OPT_VERBOSE);
    CHECK("h", strcmp(s.res.opts[1].value, "x") == 0);
}

static void test_reentrancy(void)
{
    setup s1, s2;
    char *av1[] = { "prog", "-v", "-o", "one" };
    char *av2[] = { "prog", "--quiet", "-x", "two" };

    run_parse(&s1, av1, 4);
    run_parse(&s2, av2, 4);

    CHECK("rent", s1.res.err == UC_OK);
    CHECK("rent", s2.res.err == UC_OK);
    CHECK("rent", s1.res.opts_len == 2);
    CHECK("rent", s2.res.opts_len == 2);
    CHECK("rent", s1.res.opts[0].id == OPT_VERBOSE);
    CHECK("rent", s2.res.opts[0].id == OPT_QUIET);
    CHECK("rent", s1.res.opts[1].value == av1[3]);
    CHECK("rent", s2.res.opts[1].value == av2[3]);
    CHECK("rent", s2.res.positional_len == 0);
    CHECK("rent", s2.res.positional_total == 0);

    run_parse(&s1, av1, 4);
    CHECK("rent", s1.res.opts_len == 2);
    CHECK("rent", s1.res.opts[0].id == OPT_VERBOSE);
    CHECK("rent", s1.res.opts[0].short_name == 'v');
    CHECK("rent", s2.res.opts[0].id == OPT_QUIET);
}

static void test_degenerate(void)
{
    static const ucmdline_spec dead[] = {
        { NULL, '\0', UC_ARG_NONE, 99 }
    };
    setup s;
    char *av[] = { "prog", "-v", "--anything", "p" };
    ucmdline_result r;
    ucmdline_err e;

    run_parse(&s, av, 3);
    CHECK("deg", s.res.err == UC_ERR_UNKNOWN_OPT);
    CHECK("deg", s.res.err_index == 2);

    ucmdline_result_init(&r);
    e = ucmdline_parse(NULL, 0, 3, av, &r);
    CHECK("deg", e == UC_ERR_UNKNOWN_OPT);
    CHECK("deg", r.err == UC_ERR_UNKNOWN_OPT);

    CHECK("deg", ucmdline_parse(spec_table, NSPEC, 1, NULL, &s.res) ==
          UC_OK);
    CHECK("deg", ucmdline_parse(NULL, 1, 1, av, &s.res) ==
          UC_ERR_INVALID);
    CHECK("deg", ucmdline_parse(spec_table, NSPEC, 1, av, NULL) ==
          UC_ERR_INVALID);
    CHECK("deg", ucmdline_parse(NULL, 0, 1, NULL, NULL) ==
          UC_ERR_INVALID);
    CHECK("deg", ucmdline_parse(dead, 1, 2, av, &s.res) ==
          UC_ERR_UNKNOWN_OPT);
}

static void test_long_value(void)
{
    setup s;
    static char buf[4097];
    char *av[3];
    size_t k;

    for (k = 0; k < sizeof(buf) - 1; ++k)
        buf[k] = 'x';
    buf[sizeof(buf) - 1] = '\0';

    av[0] = "prog";
    av[1] = "-o";
    av[2] = buf;

    run_parse(&s, av, 3);
    CHECK("long-in", s.res.err == UC_OK);
    CHECK("long-in", s.res.opts_len == 1);
    CHECK("long-in", s.res.opts[0].id == OPT_OUTPUT);
    CHECK("long-in", s.res.opts[0].value == buf);
    CHECK("long-in", strlen(s.res.opts[0].value) == 4096);
}

static void test_dup_spec(void)
{
    static const ucmdline_spec dup[] = {
        UC_FLAG_LONG("dup", 1),
        UC_FLAG_LONG("dup", 2)
    };
    ucmdline_result r;
    ucmdline_opt o[4];
    const char *p[4];
    char *av[] = { "prog", "--dup", "-d" };

    ucmdline_result_init(&r);
    r.opts = o;
    r.opts_cap = 4;
    r.positional = p;
    r.positional_cap = 4;

    CHECK("dup", ucmdline_parse(dup, 2, 3, av, &r) ==
          UC_ERR_UNKNOWN_OPT);
    CHECK("dup", r.err == UC_ERR_UNKNOWN_OPT);
    CHECK("dup", r.opts_len == 1);
    CHECK("dup", r.opts[0].id == 1);
    CHECK("dup", strcmp(r.opts[0].name, "dup") == 0);
}

static void test_assign_format(void)
{
    setup s;
    char *av0[] = { "prog", "output=x" };

    run_parse(&s, av0, 2);
    CHECK("assign-off", s.res.err == UC_OK);
    CHECK("assign-off", s.res.opts_len == 0);
    CHECK("assign-off", s.res.positional_len == 1);
    CHECK("assign-off", s.res.positional[0] == av0[1]);

    {
        char *av[] = { "prog", "output=log.txt" };
        run_parse_f(&s, av, 2, UC_PARSE_ASSIGN);
        CHECK("assign", s.res.err == UC_OK);
        CHECK("assign", s.res.opts_len == 1);
        CHECK("assign", s.res.opts[0].id == OPT_OUTPUT);
        CHECK("assign", s.res.opts[0].src == UC_SRC_ASSIGN);
        CHECK("assign", s.res.opts[0].short_name == '\0');
        CHECK("assign", strcmp(s.res.opts[0].name, "output") == 0);
        CHECK("assign", s.res.opts[0].value == av[1] + 7);
        CHECK("assign", s.res.positional_len == 0);
    }

    {
        char *av[] = { "prog", "level=5", "plain", "verbose=1" };
        run_parse_f(&s, av, 4, UC_PARSE_ASSIGN);
        CHECK("assign-err", s.res.err == UC_ERR_UNEXPECTED_VALUE);
        CHECK("assign-err", s.res.err_index == 3);
        CHECK("assign-err", s.res.err_name == av[3]);
        CHECK("assign-err", s.res.opts_len == 1);
        CHECK("assign-err", s.res.opts[0].id == OPT_LEVEL);
        CHECK("assign-err", strcmp(s.res.opts[0].value, "5") == 0);
        CHECK("assign-err", s.res.positional_len == 1);
        CHECK("assign-err", s.res.positional[0] == av[2]);
    }

    {
        char *av[] = { "prog", "output=" };
        run_parse_f(&s, av, 2, UC_PARSE_ASSIGN);
        CHECK("assign-empty", s.res.err == UC_OK);
        CHECK("assign-empty", s.res.opts_len == 1);
        CHECK("assign-empty", s.res.opts[0].value != NULL);
        CHECK("assign-empty", *(s.res.opts[0].value) == '\0');
    }

    {
        char *av[] = { "prog", "a=b=c", "output", "unknown=x", "=x" };
        run_parse_f(&s, av, 5, UC_PARSE_ASSIGN);
        CHECK("assign-pos", s.res.err == UC_OK);
        CHECK("assign-pos", s.res.opts_len == 0);
        CHECK("assign-pos", s.res.positional_len == 4);
        CHECK("assign-pos", s.res.positional[0] == av[1]);
        CHECK("assign-pos", s.res.positional[1] == av[2]);
        CHECK("assign-pos", s.res.positional[2] == av[3]);
        CHECK("assign-pos", s.res.positional[3] == av[4]);
    }

    {
        char *av[] = { "prog", "-v", "output=y" };
        run_parse_f(&s, av, 3, UC_PARSE_ASSIGN);
        CHECK("assign-mix", s.res.err == UC_OK);
        CHECK("assign-mix", s.res.opts_len == 2);
        CHECK("assign-mix", s.res.opts[0].src == UC_SRC_SHORT);
        CHECK("assign-mix", s.res.opts[1].src == UC_SRC_ASSIGN);
        CHECK("assign-mix", s.res.opts[1].value == av[2] + 7);
    }

    {
        char *av[] = { "prog", "--", "output=x" };
        run_parse_f(&s, av, 3, UC_PARSE_ASSIGN);
        CHECK("assign-dd", s.res.err == UC_OK);
        CHECK("assign-dd", s.res.opts_len == 0);
        CHECK("assign-dd", s.res.positional_len == 1);
        CHECK("assign-dd", s.res.positional[0] == av[2]);
    }
}

typedef struct ref_result {
    int err;              /* 0 ok, 1 unknown, 2 missing, 3 unexpected */
    int err_idx;
    char err_char;
    const char *err_name;
    int ids[256];
    const char *names[256];
    char shorts[256];
    int srcs[256];
    const char *vals[256];
    int n;
    const char *pos[256];
    int npos;
} ref_result;

static void ref_parse(const ucmdline_spec *spec, size_t nspec,
                      int argc, char *const argv[], int assign,
                      ref_result *r)
{
    int i;
    int stop = 0;

    r->err = 0;
    r->err_idx = -1;
    r->err_char = '\0';
    r->err_name = NULL;
    r->n = 0;
    r->npos = 0;

    for (i = 1; i < argc; ++i) {
        char *arg = argv[i];

        if (stop || arg[0] != '-' || arg[1] == '\0') {
            if (!stop && assign) {
                const char *eq = strchr(arg, '=');
                if (eq != NULL) {
                    const ucmdline_spec *sp = NULL;
                    size_t namelen = (size_t)(eq - arg);
                    const char *value = eq + 1;
                    size_t k;

                    for (k = 0; k < nspec; ++k) {
                        const char *ln = spec[k].long_name;
                        if (ln != NULL && strlen(ln) == namelen &&
                            strncmp(ln, arg, namelen) == 0) {
                            sp = &spec[k];
                            break;
                        }
                    }

                    if (sp != NULL) {
                        if (sp->argkind == UC_ARG_NONE) {
                            r->err = 3;
                            r->err_idx = i;
                            r->err_name = arg;
                            return;
                        }
                        r->ids[r->n] = sp->id;
                        r->names[r->n] = sp->long_name;
                        r->shorts[r->n] = '\0';
                        r->srcs[r->n] = 2;
                        r->vals[r->n] = value;
                        r->n += 1;
                        continue;
                    }
                }
            }
            r->pos[r->npos++] = arg;
            continue;
        }

        if (arg[1] == '-') {
            const ucmdline_spec *sp = NULL;
            const char *eq;
            const char *value = NULL;
            const char *name = arg + 2;
            size_t namelen;
            size_t k;

            if (arg[2] == '\0') {
                stop = 1;
                continue;
            }

            eq = strchr(arg, '=');
            if (eq != NULL) {
                namelen = (size_t)((eq - arg) - 2);
                value = eq + 1;
            } else {
                namelen = strlen(arg + 2);
            }

            for (k = 0; k < nspec; ++k) {
                const char *ln = spec[k].long_name;
                if (ln != NULL && strlen(ln) == namelen &&
                    strncmp(ln, name, namelen) == 0) {
                    sp = &spec[k];
                    break;
                }
            }

            if (sp == NULL) {
                r->err = 1;
                r->err_idx = i;
                r->err_name = arg;
                return;
            }

            if (sp->argkind == UC_ARG_NONE) {
                if (value != NULL) {
                    r->err = 3;
                    r->err_idx = i;
                    r->err_name = arg;
                    return;
                }
            } else if (sp->argkind == UC_ARG_REQUIRED) {
                if (value == NULL) {
                    if (i + 1 >= argc) {
                        r->err = 2;
                        r->err_idx = i;
                        r->err_name = arg;
                        return;
                    }
                    value = argv[i + 1];
                    i += 1;
                }
            }

            r->ids[r->n] = sp->id;
            r->names[r->n] = sp->long_name;
            r->shorts[r->n] = '\0';
            r->srcs[r->n] = 1;
            r->vals[r->n] = value;
            r->n += 1;
        } else {
            size_t j = 1;

            while (arg[j] != '\0') {
                const ucmdline_spec *sp = NULL;
                size_t k;

                for (k = 0; k < nspec; ++k) {
                    if (spec[k].short_name != '\0' &&
                        spec[k].short_name == arg[j]) {
                        sp = &spec[k];
                        break;
                    }
                }

                if (sp == NULL) {
                    r->err = 1;
                    r->err_idx = i;
                    r->err_char = arg[j];
                    return;
                }

                if (sp->argkind == UC_ARG_NONE) {
                    r->ids[r->n] = sp->id;
                    r->names[r->n] = NULL;
                    r->shorts[r->n] = arg[j];
                    r->srcs[r->n] = 0;
                    r->vals[r->n] = NULL;
                    r->n += 1;
                    j += 1;
                } else {
                    const char *value = NULL;

                    if (arg[j + 1] != '\0') {
                        value = &arg[j + 1];
                    } else if (sp->argkind == UC_ARG_REQUIRED) {
                        if (i + 1 >= argc) {
                            r->err = 2;
                            r->err_idx = i;
                            r->err_char = arg[j];
                            return;
                        }
                        value = argv[i + 1];
                        i += 1;
                    }

                    r->ids[r->n] = sp->id;
                    r->names[r->n] = NULL;
                    r->shorts[r->n] = arg[j];
                    r->srcs[r->n] = 0;
                    r->vals[r->n] = value;
                    r->n += 1;
                    break;
                }
            }
        }
    }
}

static ucmdline_err ref_err_map(int e)
{
    if (e == 0)
        return UC_OK;
    if (e == 1)
        return UC_ERR_UNKNOWN_OPT;
    if (e == 2)
        return UC_ERR_MISSING_VALUE;
    return UC_ERR_UNEXPECTED_VALUE;
}

static void cmp_results(const char *tag, const ref_result *rf,
                        const ucmdline_result *rc)
{
    int k;

    CHECK(tag, ref_err_map(rf->err) == rc->err);
    if (ref_err_map(rf->err) != rc->err)
        return;

    if (rf->err != 0) {
        CHECK(tag, rf->err_idx == (int)rc->err_index);
        if (rf->err_char != '\0')
            CHECK(tag, rf->err_char == rc->err_char);
        if (rf->err_name != NULL)
            CHECK(tag, rf->err_name == rc->err_name);
        return;
    }

    CHECK(tag, rf->n == (int)rc->opts_len);
    CHECK(tag, rf->npos == (int)rc->positional_len);
    if (rf->n != (int)rc->opts_len)
        return;

    for (k = 0; k < rf->n; ++k) {
        CHECK(tag, rf->ids[k] == rc->opts[k].id);
        CHECK(tag, rf->shorts[k] == rc->opts[k].short_name);
        CHECK(tag, rf->names[k] == rc->opts[k].name);
        CHECK(tag, rf->srcs[k] == (int)rc->opts[k].src);
        CHECK(tag, rf->vals[k] == rc->opts[k].value);
    }
    for (k = 0; k < rf->npos; ++k)
        CHECK(tag, rf->pos[k] == rc->positional[k]);
}

static uint32_t lcg_state = 0x12345678u;

static uint32_t lcg_next(void)
{
    lcg_state = lcg_state * 1664525u + 1013904223u;
    return lcg_state;
}

static void test_differential(void)
{
    static const char *pool[] = {
        "-v", "-q", "-o", "-ofoo", "-o=foo", "-ox", "-l", "-l3",
        "-l=3", "-x", "-xv", "-xfo", "-d", "-dv", "-vq", "-qv",
        "-ov", "-va", "-vd", "-vv", "-qq", "-dl", "-ld", "-xvq",
        "-lv", "--verbose", "--quiet", "--output", "--output=x",
        "--output=", "--level", "--level=z", "--alpha", "--alpha=z",
        "--debug", "--wat", "--wat=z", "-z", "-", "--", "---", "file",
        "-file", "--file", "", "=x", "123", "two words", "-2", "+x",
        "output=x", "output=", "output", "verbose=x", "level=z",
        "alpha=q", "wat=x", "unknown=x", "a=b", "x=y=z", "debug=1",
        "level", "quiet=0", "verbose"
    };
    char *av[80];
    char *av_snapshot[80];
    ucmdline_result rc;
    ucmdline_opt ropts[128];
    const char *rpos[64];
    int iter;
    int mode;

    for (iter = 0; iter < 50000; ++iter) {
        for (mode = 0; mode < 2; ++mode) {
            ref_result rf;
            size_t k;
            int argc = 1;

            av[0] = "prog";
            for (k = 0; k < (sizeof(pool) / sizeof(pool[0])); ++k) {
                if ((lcg_next() & 1u) != 0u && argc < 79)
                    av[argc++] = (char *)pool[k];
            }
            for (k = 0; k < (size_t)argc; ++k)
                av_snapshot[k] = av[k];

            ref_parse(spec_table, NSPEC, argc, av, mode, &rf);

            ucmdline_result_init(&rc);
            rc.flags = (mode != 0) ? UC_PARSE_ASSIGN : 0u;
            rc.opts = ropts;
            rc.opts_cap = 128;
            rc.positional = rpos;
            rc.positional_cap = 64;
        (void)ucmdline_parse(spec_table, NSPEC, argc, av, &rc);

        cmp_results("fuzz", &rf, &rc);
        CHECK("fuzz-nomutate",
              memcmp(av_snapshot, av, (size_t)argc * sizeof(char *)) == 0);

        if (g_fail > 250) {
            printf("fuzz: too many failures, aborting\n");
            return;
        }
        }
    }
}

int main(void)
{
    test_init_zeroes();
    test_basic_flags();
    test_empty_and_only_prog();
    test_bundling();
    test_short_values();
    test_long_values();
    test_optional_args();
    test_double_dash();
    test_mixed();
    test_errors();
    test_overflow();
    test_helpers();
    test_reentrancy();
    test_degenerate();
    test_long_value();
    test_dup_spec();
    test_assign_format();
    test_differential();

    printf("ucmdline tests: %u passed, %u failed\n",
           (unsigned)g_pass, (unsigned)g_fail);
    return g_fail == 0 ? 0 : 1;
}
