#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

typedef enum {
    SILENT = 1,
    QUIET = 2,
    NORMAL = 3,
    NOISY = 4
} Noise;

typedef struct hook_stru {
    struct hook_stru *next;
    char *nick;
    char *stuff;
    Noise noisy;
    int not;
    int sernum;
    int flexible;
    int global;
    char *filename;
    int debug;
} Hook;

typedef struct numericlist_stru {
    struct numericlist_stru *next;
    int numeric;
    char name[8]; /* Hardened: expanded from 4 to 8 to prevent buffer overflow */
    Hook *list;
    char *filename;
} NumericList;

static void upper(char *str) {
    if (!str) return;
    while (*str) {
        *str = (char)toupper((unsigned char)*str);
        str++;
    }
}

static void malloc_strcpy(char **dest, const char *src) {
    if (*dest) {
        free(*dest);
        *dest = NULL;
    }
    if (src) {
        *dest = strdup(src);
    }
}

/* Extracted and hardened populate_hook function from source/hook.c */
static void populate_hook(Hook *new_hook, const char *nick, const char *stuff,
                          Noise noisy, int not_flag, int sernum, int flexible) {
    malloc_strcpy(&new_hook->nick, nick);
    malloc_strcpy(&new_hook->stuff, stuff);
    new_hook->noisy = noisy;
    new_hook->not = not_flag;
    new_hook->sernum = sernum;
    new_hook->flexible = flexible;
    new_hook->global = 0;
    malloc_strcpy(&new_hook->filename, "unit_test_package");
    new_hook->next = NULL;
    upper(new_hook->nick);
}

/* Safe numeric formatting from source/hook.c */
static void format_numeric_name(char *dest, size_t dest_size, int numeric) {
    snprintf(dest, dest_size, "%03u", (unsigned int)numeric);
}

/* --- Good Tests for populate_hook --- */

static void test_populate_hook_basic(void **state) {
    (void) state;
    Hook hook;
    memset(&hook, 0, sizeof(hook));

    populate_hook(&hook, "john_doe", "echo hello world", NORMAL, 0, 100, 1);

    assert_non_null(hook.nick);
    assert_string_equal(hook.nick, "JOHN_DOE"); /* Verifies uppercase normalization */
    assert_non_null(hook.stuff);
    assert_string_equal(hook.stuff, "echo hello world");
    assert_int_equal(hook.noisy, NORMAL);
    assert_int_equal(hook.not, 0);
    assert_int_equal(hook.sernum, 100);
    assert_int_equal(hook.flexible, 1);
    assert_null(hook.next);

    free(hook.nick);
    free(hook.stuff);
    free(hook.filename);
}

static void test_populate_hook_not_flag(void **state) {
    (void) state;
    Hook hook;
    memset(&hook, 0, sizeof(hook));

    populate_hook(&hook, "bot", "quit", SILENT, 1, 42, 0);

    assert_string_equal(hook.nick, "BOT");
    assert_int_equal(hook.not, 1);
    assert_int_equal(hook.noisy, SILENT);
    assert_int_equal(hook.sernum, 42);

    free(hook.nick);
    free(hook.stuff);
    free(hook.filename);
}

/* --- Boundary and Negative Tests for Numeric Formatting --- */

static void test_format_numeric_standard(void **state) {
    (void) state;
    char buf[8];

    format_numeric_name(buf, sizeof(buf), 1);
    assert_string_equal(buf, "001");

    format_numeric_name(buf, sizeof(buf), 42);
    assert_string_equal(buf, "042");

    format_numeric_name(buf, sizeof(buf), 999);
    assert_string_equal(buf, "999");
}

static void test_format_numeric_zero(void **state) {
    (void) state;
    char buf[8];

    format_numeric_name(buf, sizeof(buf), 0);
    assert_string_equal(buf, "000");
}

static void test_format_numeric_overflow_prevention(void **state) {
    (void) state;
    /* In the legacy code, char name[4] overflowed on 4-digit numerics >= 1000.
     * With 8 bytes and snprintf, 1000 and 65535 format safely without overflow.
     */
    char buf[8];
    memset(buf, 0xAA, sizeof(buf));

    format_numeric_name(buf, sizeof(buf), 1000);
    assert_string_equal(buf, "1000");
    assert_int_equal(buf[4], '\0');

    format_numeric_name(buf, sizeof(buf), 65535);
    assert_string_equal(buf, "65535");
    assert_int_equal(buf[5], '\0');
}

static void test_format_numeric_buffer_limit_protection(void **state) {
    (void) state;
    /* Boundary check: large numeric with small buffer is safely truncated and null-terminated */
    char small_buf[4];
    format_numeric_name(small_buf, sizeof(small_buf), 12345);
    assert_int_equal(small_buf[3], '\0');
    assert_int_equal(strncmp(small_buf, "123", 3), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_populate_hook_basic),
        cmocka_unit_test(test_populate_hook_not_flag),
        cmocka_unit_test(test_format_numeric_standard),
        cmocka_unit_test(test_format_numeric_zero),
        cmocka_unit_test(test_format_numeric_overflow_prevention),
        cmocka_unit_test(test_format_numeric_buffer_limit_protection),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
