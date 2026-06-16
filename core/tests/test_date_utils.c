#include "date_utils.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static int tests_run = 0;
static int tests_failed = 0;

#define TEST(name) do { printf("  %s ... ", name); tests_run++; } while(0)
#define PASS() printf("ok\n")
#define FAIL(msg) do { printf("FAIL (%s)\n", msg); tests_failed++; } while(0)
#define ASSERT(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)
#define ASSERT_EQ(a, b, msg) do { if ((a) != (b)) { FAIL(msg); return; } } while(0)

static void test_parse_iso(void) {
    TEST("parse ISO date");
    Date d;
    ASSERT(parse_date("2026-06-17", &d), "should parse ISO");
    ASSERT_EQ(d.year, 2026, "year");
    ASSERT_EQ(d.month, 6, "month");
    ASSERT_EQ(d.day, 17, "day");
    PASS();
}

static void test_parse_us_long(void) {
    TEST("parse US long");
    Date d;
    ASSERT(parse_date("June 17, 2026", &d), "should parse US long");
    ASSERT_EQ(d.year, 2026, "year");
    ASSERT_EQ(d.month, 6, "month");
    ASSERT_EQ(d.day, 17, "day");
    PASS();
}

static void test_parse_eu(void) {
    TEST("parse EU date");
    Date d;
    ASSERT(parse_date("17 Jun 2026", &d), "should parse EU");
    ASSERT_EQ(d.year, 2026, "year");
    ASSERT_EQ(d.month, 6, "month");
    ASSERT_EQ(d.day, 17, "day");
    PASS();
}

static void test_parse_today(void) {
    TEST("parse 'today'");
    Date d;
    ASSERT(parse_date("today", &d), "should parse today");
    Date today = date_today();
    ASSERT_EQ(d.year, today.year, "year");
    ASSERT_EQ(d.month, today.month, "month");
    ASSERT_EQ(d.day, today.day, "day");
    PASS();
}

static void test_parse_tomorrow(void) {
    TEST("parse 'tomorrow'");
    Date d;
    ASSERT(parse_date("tomorrow", &d), "should parse tomorrow");
    Date expected = date_tomorrow();
    ASSERT_EQ(d.year, expected.year, "year");
    ASSERT_EQ(d.month, expected.month, "month");
    ASSERT_EQ(d.day, expected.day, "day");
    PASS();
}

static void test_parse_next_monday(void) {
    TEST("parse 'next monday'");
    Date d;
    ASSERT(parse_date("next monday", &d), "should parse next monday");
    int dow = day_of_week(d);
    ASSERT_EQ(dow, 1, "should be Monday (dow=1)");
    PASS();
}

static void test_parse_invalid(void) {
    TEST("parse invalid string");
    Date d;
    ASSERT(!parse_date("not a date", &d), "should reject garbage");
    PASS();
}

static void test_format_iso(void) {
    TEST("format ISO");
    Date d = {2026, 6, 17};
    char buf[MAX_DATE_STR_LEN];
    format_date("iso", d, buf, sizeof(buf));
    ASSERT(strcmp(buf, "2026-06-17") == 0, "should format as ISO");
    PASS();
}

static void test_format_long(void) {
    TEST("format long");
    Date d = {2026, 6, 17};
    char buf[MAX_DATE_STR_LEN];
    format_date("long", d, buf, sizeof(buf));
    ASSERT(strcmp(buf, "June 17, 2026") == 0, "should format long");
    PASS();
}

static void test_add_days_simple(void) {
    TEST("add_days +1");
    Date d = {2026, 6, 17};
    Date r = add_days(d, 1);
    ASSERT_EQ(r.year, 2026, "year");
    ASSERT_EQ(r.month, 6, "month");
    ASSERT_EQ(r.day, 18, "day");
    PASS();
}

static void test_add_days_negative(void) {
    TEST("add_days -1");
    Date d = {2026, 6, 17};
    Date r = add_days(d, -1);
    ASSERT_EQ(r.year, 2026, "year");
    ASSERT_EQ(r.month, 6, "month");
    ASSERT_EQ(r.day, 16, "day");
    PASS();
}

static void test_add_days_month_boundary(void) {
    TEST("add_days month rollover");
    Date d = {2026, 6, 30};
    Date r = add_days(d, 1);
    ASSERT_EQ(r.year, 2026, "year");
    ASSERT_EQ(r.month, 7, "month");
    ASSERT_EQ(r.day, 1, "day");
    PASS();
}

static void test_add_days_year_boundary(void) {
    TEST("add_days year rollover");
    Date d = {2026, 12, 31};
    Date r = add_days(d, 1);
    ASSERT_EQ(r.year, 2027, "year");
    ASSERT_EQ(r.month, 1, "month");
    ASSERT_EQ(r.day, 1, "day");
    PASS();
}

static void test_day_of_week(void) {
    TEST("day_of_week known date");
    Date d = {2026, 6, 17};
    int dow = day_of_week(d);
    ASSERT_EQ(dow, 3, "2026-06-17 should be Wednesday (dow=3)");
    PASS();
}

static void test_week_start(void) {
    TEST("week_start");
    Date d = {2026, 6, 17};
    Date ws = week_start(d);
    ASSERT_EQ(ws.year, 2026, "year");
    ASSERT_EQ(ws.month, 6, "month");
    ASSERT_EQ(ws.day, 14, "should be Sunday June 14");
    PASS();
}

int main(void) {
    printf("date_utils tests:\n");
    test_parse_iso();
    test_parse_us_long();
    test_parse_eu();
    test_parse_today();
    test_parse_tomorrow();
    test_parse_next_monday();
    test_parse_invalid();
    test_format_iso();
    test_format_long();
    test_add_days_simple();
    test_add_days_negative();
    test_add_days_month_boundary();
    test_add_days_year_boundary();
    test_day_of_week();
    test_week_start();

    printf("\n%d tests, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
