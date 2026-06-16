#include "meal_plan.h"
#include <stdio.h>
#include <string.h>

static int tests_run = 0;
static int tests_failed = 0;

#define TEST(name) do { printf("  %s ... ", name); tests_run++; } while(0)
#define PASS() printf("ok\n")
#define FAIL(msg) do { printf("FAIL (%s)\n", msg); tests_failed++; } while(0)
#define ASSERT(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)

static void test_date_valid(void) {
    TEST("valid date");
    Date d = {2026, 6, 17};
    ASSERT(date_valid(d), "2026-06-17 should be valid");
    PASS();
}

static void test_date_invalid(void) {
    TEST("invalid month");
    Date d = {2026, 13, 1};
    ASSERT(!date_valid(d), "month 13 should be invalid");
    PASS();
}

static void test_date_invalid_day(void) {
    TEST("invalid day");
    Date d = {2026, 2, 30};
    ASSERT(!date_valid(d), "Feb 30 should be invalid");
    PASS();
}

static void test_leap_year(void) {
    TEST("leap year Feb 29");
    Date d = {2024, 2, 29};
    ASSERT(date_valid(d), "2024-02-29 should be valid (leap year)");
    PASS();
}

static void test_non_leap_year(void) {
    TEST("non-leap Feb 28");
    Date d = {2025, 2, 28};
    ASSERT(date_valid(d), "2025-02-28 should be valid");
    PASS();
}

static void test_date_cmp_equal(void) {
    TEST("date_cmp equal");
    Date a = {2026, 6, 17};
    Date b = {2026, 6, 17};
    ASSERT(date_cmp(a, b) == 0, "identical dates should return 0");
    PASS();
}

static void test_date_cmp_before(void) {
    TEST("date_cmp before");
    Date a = {2026, 6, 16};
    Date b = {2026, 6, 17};
    ASSERT(date_cmp(a, b) < 0, "earlier date should return negative");
    PASS();
}

static void test_date_cmp_after(void) {
    TEST("date_cmp after");
    Date a = {2026, 6, 18};
    Date b = {2026, 6, 17};
    ASSERT(date_cmp(a, b) > 0, "later date should return positive");
    PASS();
}

static void test_meal_clear(void) {
    TEST("meal_clear");
    Meal m;
    strcpy(m.name, "test");
    m.ingredient_count = 3;
    meal_clear(&m);
    ASSERT(m.name[0] == '\0', "name should be empty");
    ASSERT(m.ingredient_count == 0, "count should be 0");
    PASS();
}

static void test_day_plan_clear(void) {
    TEST("day_plan_clear");
    DayPlan p;
    p.date.year = 2026;
    p.has_meal[0] = true;
    day_plan_clear(&p);
    ASSERT(p.date.year == 0, "year should be 0");
    ASSERT(!p.has_meal[0], "has_meal should be false");
    PASS();
}

static void test_week_plan_clear(void) {
    TEST("week_plan_clear");
    WeekPlan w;
    w.day_count = 5;
    week_plan_clear(&w);
    ASSERT(w.day_count == 0, "day_count should be 0");
    PASS();
}

int main(void) {
    printf("meal_plan tests:\n");
    test_date_valid();
    test_date_invalid();
    test_date_invalid_day();
    test_leap_year();
    test_non_leap_year();
    test_date_cmp_equal();
    test_date_cmp_before();
    test_date_cmp_after();
    test_meal_clear();
    test_day_plan_clear();
    test_week_plan_clear();

    printf("\n%d tests, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
