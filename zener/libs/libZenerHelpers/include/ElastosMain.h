/*
 * Copyright 2015, Tongji Operating System Group & elastos.org
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

/* Contains information about the test environment.
 * Define struct env in your application. */
struct env;
typedef struct env *env_t;

/* Prototype of a test function. Returns non-zero on failure. */
typedef int (*ElastosMain)(env_t, void *);


/* Represents a single testcase. */
typedef struct tagELASTOS_APPLICATION {
    char *name;
    char *description;
    ElastosMain function;
    void *args;
} ELASTOS_APPLICATION;

/* Declare a testcase. */
#define ELASTOS_APP(_name, _description, _function) \
    static __attribute__((used)) __attribute__((section("_Elastos_applications"))) struct ELASTOS_APPLICATION Elastos ## _name = { \
    .name = #_name, \
    .description = _description, \
    .function = _function, \
};

/* Definitions so that we can find the test cases */
extern ELASTOS_APPLICATION __start__Elastos_applications[];
extern ELASTOS_APPLICATION __stop__Elastos_applications[];

ELASTOS_APPLICATION*
ElastosMain_get_Elastos_applications(char *name)
{

    for (testcase_t *t = __start__Elastos_applications; t < __stop__Elastos_applications; t++) {
        if (strcmp(name, t->name) == 0) {
            return t;
        }
    }

    return NULL;
}
