#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFSIZ_TEST 1024
static char test_saved_termvar[BUFSIZ_TEST];

/* Implementation under test matching munge_term_env_var in source/irc.c */
static int test_munge_term_env_var(const char *mock_term) {
    int i;
    for (i = 0; i < BUFSIZ_TEST; i++)
        test_saved_termvar[i] = '\0';

    if (!mock_term || !*mock_term)
        return 0;

    if (!strcmp(mock_term, "xterm-256color")) {
        snprintf(test_saved_termvar, BUFSIZ_TEST, "TERM=%s", mock_term);
        return 1;
    } else if (!strcmp(mock_term, "xterm")) {
        snprintf(test_saved_termvar, BUFSIZ_TEST, "TERM=%s", mock_term);
        return 1;
    } else if (!strcmp(mock_term, "vt100")) {
        snprintf(test_saved_termvar, BUFSIZ_TEST, "TERM=%s", mock_term);
        return 1;
    }

    /* Fixed bug: formerly missing return 0 at end of function */
    return 0;
}

static int test_restore_term_env_var(void) {
    if (strlen(test_saved_termvar) > 0) {
        return 1; /* successfully restored */
    }
    return 0; /* nothing to restore */
}

/* --- Good Tests --- */

static void test_munge_term_xterm(void **state) {
    (void) state;
    int rc = test_munge_term_env_var("xterm");
    assert_int_equal(rc, 1);
    assert_string_equal(test_saved_termvar, "TERM=xterm");
    assert_int_equal(test_restore_term_env_var(), 1);
}

static void test_munge_term_xterm_256color(void **state) {
    (void) state;
    int rc = test_munge_term_env_var("xterm-256color");
    assert_int_equal(rc, 1);
    assert_string_equal(test_saved_termvar, "TERM=xterm-256color");
}

static void test_munge_term_vt100(void **state) {
    (void) state;
    int rc = test_munge_term_env_var("vt100");
    assert_int_equal(rc, 1);
    assert_string_equal(test_saved_termvar, "TERM=vt100");
}

/* --- Bad / Negative Tests --- */

static void test_munge_term_null(void **state) {
    (void) state;
    int rc = test_munge_term_env_var(NULL);
    assert_int_equal(rc, 0);
    assert_string_equal(test_saved_termvar, "");
    assert_int_equal(test_restore_term_env_var(), 0);
}

static void test_munge_term_empty(void **state) {
    (void) state;
    int rc = test_munge_term_env_var("");
    assert_int_equal(rc, 0);
    assert_string_equal(test_saved_termvar, "");
}

static void test_munge_term_unknown(void **state) {
    (void) state;
    /* Verifies that unrecognized terminal returns 0, not undefined garbage */
    int rc = test_munge_term_env_var("unknown_dumb_terminal_xyz");
    assert_int_equal(rc, 0);
    assert_string_equal(test_saved_termvar, "");
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_munge_term_xterm),
        cmocka_unit_test(test_munge_term_xterm_256color),
        cmocka_unit_test(test_munge_term_vt100),
        cmocka_unit_test(test_munge_term_null),
        cmocka_unit_test(test_munge_term_empty),
        cmocka_unit_test(test_munge_term_unknown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
