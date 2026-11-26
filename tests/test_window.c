#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <stdlib.h>

#define REDRAW_DISPLAY_FULL 1
#define REDRAW_DISPLAY_FAST 2
#define UPDATE_STATUS       4
#define REDRAW_STATUS       8

/* Mock window structure matching the flag fields from source/window.c */
typedef struct {
    int update;
    int lines_held;
    int last_lines_held;
} MockWindow;

/* Fixed logic in source/window.c: BX_set_hold_mode */
static void fixed_hold_mode_status_clear(MockWindow *window) {
    if (window->update & UPDATE_STATUS)
        window->update &= ~UPDATE_STATUS;
}

/* Old buggy logic from original code before fix */
static void buggy_hold_mode_status_clear(MockWindow *window) {
    if (window->update | UPDATE_STATUS)
        window->update -= UPDATE_STATUS;
}

/* --- Good Tests for fixed implementation --- */

static void test_status_flag_cleared_when_present(void **state) {
    (void) state;
    MockWindow win = { .update = UPDATE_STATUS | REDRAW_DISPLAY_FULL };

    fixed_hold_mode_status_clear(&win);

    /* UPDATE_STATUS must be cleared */
    assert_false(win.update & UPDATE_STATUS);
    /* REDRAW_DISPLAY_FULL must be preserved */
    assert_true(win.update & REDRAW_DISPLAY_FULL);
    assert_int_equal(win.update, REDRAW_DISPLAY_FULL);
}

static void test_status_flag_untouched_when_not_present(void **state) {
    (void) state;
    MockWindow win = { .update = REDRAW_DISPLAY_FULL | REDRAW_STATUS };

    fixed_hold_mode_status_clear(&win);

    /* All original flags must remain intact without change */
    assert_int_equal(win.update, REDRAW_DISPLAY_FULL | REDRAW_STATUS);
    assert_false(win.update & UPDATE_STATUS);
}

static void test_status_flag_zero_stays_zero(void **state) {
    (void) state;
    MockWindow win = { .update = 0 };

    fixed_hold_mode_status_clear(&win);
    assert_int_equal(win.update, 0);
}

static void test_status_all_flags_except_status_preserved(void **state) {
    (void) state;
    int all_other = REDRAW_DISPLAY_FULL | REDRAW_DISPLAY_FAST | REDRAW_STATUS;
    MockWindow win = { .update = all_other | UPDATE_STATUS };

    fixed_hold_mode_status_clear(&win);
    assert_int_equal(win.update, all_other);
}

/* --- Negative / Defect Reproduction Tests --- */

static void test_reproduce_original_bitwise_bug(void **state) {
    (void) state;
    /* In original code: if (window->update | UPDATE_STATUS) window->update -= UPDATE_STATUS;
     * When UPDATE_STATUS is NOT set: window->update was 1 (REDRAW_DISPLAY_FULL).
     * 1 | 4 is 5 != 0, so it executed 1 - 4 = -3, corrupting window->update!
     */
    MockWindow win_buggy = { .update = REDRAW_DISPLAY_FULL };
    buggy_hold_mode_status_clear(&win_buggy);
    /* Demonstrates that buggy version corrupted the flags (1 - 4 = -3) */
    assert_int_not_equal(win_buggy.update, REDRAW_DISPLAY_FULL);

    /* Fixed version preserves flags cleanly */
    MockWindow win_fixed = { .update = REDRAW_DISPLAY_FULL };
    fixed_hold_mode_status_clear(&win_fixed);
    assert_int_equal(win_fixed.update, REDRAW_DISPLAY_FULL);
}

static void test_permutation_robustness(void **state) {
    (void) state;
    /* Test all 16 flag combinations from 0 to 15 */
    for (int mask = 0; mask < 16; mask++) {
        MockWindow win = { .update = mask };
        fixed_hold_mode_status_clear(&win);
        assert_int_equal(win.update, mask & ~UPDATE_STATUS);
    }
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_status_flag_cleared_when_present),
        cmocka_unit_test(test_status_flag_untouched_when_not_present),
        cmocka_unit_test(test_status_flag_zero_stays_zero),
        cmocka_unit_test(test_status_all_flags_except_status_preserved),
        cmocka_unit_test(test_reproduce_original_bitwise_bug),
        cmocka_unit_test(test_permutation_robustness),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
