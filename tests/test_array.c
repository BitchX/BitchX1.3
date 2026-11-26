#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>

#define RESIZE(x, y, z)	do { (x) = (y *)new_realloc((char *)(x), (size_t)((z) * sizeof(y))); } while (0)

void *new_malloc(size_t size) {
    return malloc(size);
}

void *new_realloc(void *ptr, size_t size) {
    return realloc(ptr, size);
}

void *new_free(char **ptr) {
    if (ptr && *ptr) {
        free(*ptr);
        *ptr = NULL;
    }
    return NULL;
}

typedef struct an_array_struct {
    char **item;
    long *index;
    long size;
} an_array;

/* Function under test: insert_index */
static void insert_index(long **index, long *size, long newIndex) {
    long cnt;

    if (*size)
        RESIZE(*index, long, *size + 1);
    else {
        *index = (long *)new_malloc(sizeof(long));
        newIndex = 0;
    }

    if (newIndex < 0)
        newIndex = 0;
    if (newIndex > *size)
        newIndex = *size;

    for (cnt = *size; cnt > newIndex; cnt--)
        (*index)[cnt] = (*index)[cnt - 1];
    (*index)[newIndex] = *size;
    (*size)++;
}

/* Function under test: find_item */
static long find_item(an_array array, const char *find) {
    long top, bottom, key, cmp;

    if (!find || array.size <= 0 || !array.item || !array.index)
        return -1;

    top = array.size - 1;
    bottom = 0;

    while (top >= bottom) {
        key = (top - bottom) / 2 + bottom;
        cmp = strcmp(find, array.item[array.index[key]]);
        if (cmp == 0)
            return key;
        if (cmp < 0)
            top = key - 1;
        else
            bottom = key + 1;
    }
    return -bottom - 1;
}

/* --- Tests for insert_index (Good and Bad) --- */

static void test_insert_index_empty(void **state) {
    (void) state;
    long *idx = NULL;
    long size = 0;

    insert_index(&idx, &size, 0);
    assert_non_null(idx);
    assert_int_equal(size, 1);
    assert_int_equal(idx[0], 0);

    free(idx);
}

static void test_insert_index_sequential(void **state) {
    (void) state;
    long *idx = NULL;
    long size = 0;

    for (int i = 0; i < 5; i++) {
        insert_index(&idx, &size, i);
    }
    assert_int_equal(size, 5);
    for (int i = 0; i < 5; i++) {
        assert_int_equal(idx[i], i);
    }

    free(idx);
}

static void test_insert_index_front(void **state) {
    (void) state;
    long *idx = NULL;
    long size = 0;

    /* Inserting repeatedly at front (newIndex = 0) */
    insert_index(&idx, &size, 0); /* [0] */
    insert_index(&idx, &size, 0); /* [1, 0] */
    insert_index(&idx, &size, 0); /* [2, 1, 0] */

    assert_int_equal(size, 3);
    assert_int_equal(idx[0], 2);
    assert_int_equal(idx[1], 1);
    assert_int_equal(idx[2], 0);

    free(idx);
}

static void test_insert_index_middle(void **state) {
    (void) state;
    long *idx = NULL;
    long size = 0;

    insert_index(&idx, &size, 0); /* size=1, idx[0]=0 */
    insert_index(&idx, &size, 1); /* size=2, idx=[0, 1] */
    insert_index(&idx, &size, 1); /* insert at 1: size=3, idx=[0, 2, 1] */

    assert_int_equal(size, 3);
    assert_int_equal(idx[0], 0);
    assert_int_equal(idx[1], 2);
    assert_int_equal(idx[2], 1);

    free(idx);
}

static void test_insert_index_stress_resizing(void **state) {
    (void) state;
    long *idx = NULL;
    long size = 0;

    /* Stress test RESIZE macro with 100 elements */
    for (int i = 0; i < 100; i++) {
        insert_index(&idx, &size, i / 2);
    }
    assert_int_equal(size, 100);
    assert_non_null(idx);

    free(idx);
}

static void test_insert_index_bad_negative(void **state) {
    (void) state;
    long *idx = NULL;
    long size = 0;

    insert_index(&idx, &size, 0);
    /* Bad newIndex < 0 clamped to 0 */
    insert_index(&idx, &size, -5);
    assert_int_equal(size, 2);
    assert_int_equal(idx[0], 1);
    assert_int_equal(idx[1], 0);

    free(idx);
}

static void test_insert_index_bad_out_of_bounds(void **state) {
    (void) state;
    long *idx = NULL;
    long size = 0;

    insert_index(&idx, &size, 0);
    /* Bad newIndex > size clamped to size */
    insert_index(&idx, &size, 999);
    assert_int_equal(size, 2);
    assert_int_equal(idx[0], 0);
    assert_int_equal(idx[1], 1);

    free(idx);
}

/* --- Tests for find_item (Good and Bad) --- */

static void test_find_item_existing(void **state) {
    (void) state;
    char *items[] = { "apple", "banana", "cherry", "date" };
    long index[] = { 0, 1, 2, 3 };
    an_array arr = { items, index, 4 };

    long found = find_item(arr, "banana");
    assert_int_equal(found, 1);

    found = find_item(arr, "apple");
    assert_int_equal(found, 0);

    found = find_item(arr, "date");
    assert_int_equal(found, 3);
}

static void test_find_item_missing(void **state) {
    (void) state;
    char *items[] = { "apple", "cherry" };
    long index[] = { 0, 1 };
    an_array arr = { items, index, 2 };

    /* "banana" should be between "apple" (idx 0) and "cherry" (idx 1) -> negative insertion point */
    long found = find_item(arr, "banana");
    assert_true(found < 0);
}

static void test_find_item_empty_array(void **state) {
    (void) state;
    an_array arr = { NULL, NULL, 0 };
    long found = find_item(arr, "apple");
    assert_int_equal(found, -1);
}

static void test_find_item_null_search(void **state) {
    (void) state;
    char *items[] = { "apple" };
    long index[] = { 0 };
    an_array arr = { items, index, 1 };

    long found = find_item(arr, NULL);
    assert_int_equal(found, -1);
}

static void test_find_item_case_sensitive(void **state) {
    (void) state;
    char *items[] = { "apple" };
    long index[] = { 0 };
    an_array arr = { items, index, 1 };

    /* "Apple" != "apple" */
    long found = find_item(arr, "Apple");
    assert_true(found < 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_insert_index_empty),
        cmocka_unit_test(test_insert_index_sequential),
        cmocka_unit_test(test_insert_index_front),
        cmocka_unit_test(test_insert_index_middle),
        cmocka_unit_test(test_insert_index_stress_resizing),
        cmocka_unit_test(test_insert_index_bad_negative),
        cmocka_unit_test(test_insert_index_bad_out_of_bounds),
        cmocka_unit_test(test_find_item_existing),
        cmocka_unit_test(test_find_item_missing),
        cmocka_unit_test(test_find_item_empty_array),
        cmocka_unit_test(test_find_item_null_search),
        cmocka_unit_test(test_find_item_case_sensitive),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
