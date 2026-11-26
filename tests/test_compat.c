#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

typedef struct dirent DIRENT;

extern int bx_scandir(const char *name,
                      DIRENT ***list,
                      int (*selector)(DIRENT *),
                      int (*sorter)(const void *, const void *));

extern int bsd_setenv(const char *name, const char *value, int rewrite);

/* --- scandir sorters and selectors --- */

static int test_alphasort(const void *a, const void *b) {
    const DIRENT *d1 = *(const DIRENT **)a;
    const DIRENT *d2 = *(const DIRENT **)b;
    return strcmp(d1->d_name, d2->d_name);
}

static int select_all(DIRENT *d) {
    (void) d;
    return 1;
}

static int select_none(DIRENT *d) {
    (void) d;
    return 0;
}

static int select_starts_with_dot(DIRENT *d) {
    return d->d_name[0] == '.';
}

/* --- scandir tests (good and bad) --- */

static void test_scandir_valid_dir(void **state) {
    (void) state;
    DIRENT **list = NULL;
    int count = bx_scandir(".", &list, select_all, test_alphasort);
    assert_true(count > 0);
    assert_non_null(list);
    for (int i = 0; i < count; i++) {
        assert_non_null(list[i]);
        free(list[i]);
    }
    free(list);
}

static void test_scandir_selector_filter(void **state) {
    (void) state;
    DIRENT **list = NULL;
    int count = bx_scandir(".", &list, select_starts_with_dot, test_alphasort);
    assert_true(count >= 0);
    if (count > 0) {
        assert_non_null(list);
        for (int i = 0; i < count; i++) {
            assert_int_equal(list[i]->d_name[0], '.');
            free(list[i]);
        }
        free(list);
    }
}

static void test_scandir_selector_none(void **state) {
    (void) state;
    DIRENT **list = NULL;
    int count = bx_scandir(".", &list, select_none, test_alphasort);
    assert_int_equal(count, 0);
    if (list) {
        free(list);
    }
}

static void test_scandir_null_selector(void **state) {
    (void) state;
    DIRENT **list = NULL;
    int count = bx_scandir(".", &list, NULL, test_alphasort);
    assert_true(count > 0);
    assert_non_null(list);
    for (int i = 0; i < count; i++) {
        free(list[i]);
    }
    free(list);
}

static void test_scandir_nonexistent_path(void **state) {
    (void) state;
    DIRENT **list = NULL;
    int count = bx_scandir("/nonexistent/path/for/unit/test/12345", &list, NULL, NULL);
    assert_int_equal(count, -1);
}

static void test_scandir_regular_file(void **state) {
    (void) state;
    char temp_file[] = "/tmp/bx_test_file_XXXXXX";
    int fd = mkstemp(temp_file);
    assert_true(fd >= 0);
    close(fd);

    DIRENT **list = NULL;
    int count = bx_scandir(temp_file, &list, NULL, NULL);
    assert_int_equal(count, -1);
    unlink(temp_file);
}

/* --- bsd_setenv tests (good and bad) --- */

static void test_setenv_new_variable(void **state) {
    (void) state;
    const char *key = "BX_TEST_VAR_NEW_1";
    const char *val = "alpha_val_42";

    int rc = bsd_setenv(key, val, 1);
    assert_int_equal(rc, 0);
    assert_string_equal(getenv(key), val);
}

static void test_setenv_overwrite_allowed(void **state) {
    (void) state;
    const char *key = "BX_TEST_VAR_OVERWRITE";
    bsd_setenv(key, "first_val", 1);
    assert_string_equal(getenv(key), "first_val");

    int rc = bsd_setenv(key, "second_val", 1);
    assert_int_equal(rc, 0);
    assert_string_equal(getenv(key), "second_val");
}

static void test_setenv_overwrite_denied(void **state) {
    (void) state;
    const char *key = "BX_TEST_VAR_NO_OVERWRITE";
    bsd_setenv(key, "original_val", 1);
    assert_string_equal(getenv(key), "original_val");

    int rc = bsd_setenv(key, "rejected_val", 0);
    assert_int_equal(rc, 0);
    assert_string_equal(getenv(key), "original_val");
}

static void test_setenv_null_name(void **state) {
    (void) state;
    int rc = bsd_setenv(NULL, "some_value", 1);
    assert_int_equal(rc, -1);
    assert_int_equal(errno, EINVAL);
}

static void test_setenv_empty_name(void **state) {
    (void) state;
    int rc = bsd_setenv("", "some_value", 1);
    assert_int_equal(rc, -1);
    assert_int_equal(errno, EINVAL);
}

static void test_setenv_name_with_equals(void **state) {
    (void) state;
    int rc = bsd_setenv("INVALID=KEY", "some_value", 1);
    assert_int_equal(rc, -1);
    assert_int_equal(errno, EINVAL);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        /* scandir unit tests (good & bad) */
        cmocka_unit_test(test_scandir_valid_dir),
        cmocka_unit_test(test_scandir_selector_filter),
        cmocka_unit_test(test_scandir_selector_none),
        cmocka_unit_test(test_scandir_null_selector),
        cmocka_unit_test(test_scandir_nonexistent_path),
        cmocka_unit_test(test_scandir_regular_file),
        /* bsd_setenv unit tests (good & bad) */
        cmocka_unit_test(test_setenv_new_variable),
        cmocka_unit_test(test_setenv_overwrite_allowed),
        cmocka_unit_test(test_setenv_overwrite_denied),
        cmocka_unit_test(test_setenv_null_name),
        cmocka_unit_test(test_setenv_empty_name),
        cmocka_unit_test(test_setenv_name_with_equals),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
