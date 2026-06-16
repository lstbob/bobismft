#include "sanitize.h"
#include <stdio.h>
#include <string.h>

static int tests_run = 0;
static int tests_failed = 0;

#define TEST(name) do { printf("  %s ... ", name); tests_run++; } while(0)
#define PASS() printf("ok\n")
#define FAIL(msg) do { printf("FAIL (%s)\n", msg); tests_failed++; } while(0)
#define ASSERT(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)
#define ASSERT_EQ(a, b, msg) do { if ((a) != (b)) { FAIL(msg); return; } } while(0)

static void test_sanitize_normal(void) {
    TEST("normal string unchanged");
    char buf[] = "Hello World";
    sanitize_string(buf, sizeof(buf));
    ASSERT(strcmp(buf, "Hello World") == 0, "should be unchanged");
    PASS();
}

static void test_sanitize_control_chars(void) {
    TEST("control chars removed");
    char buf[] = "a\x01" "b\x02" "c";
    sanitize_string(buf, sizeof(buf));
    ASSERT(strcmp(buf, "abc") == 0, "control chars removed");
    PASS();
}

static void test_sanitize_carriage_return(void) {
    TEST("carriage return removed");
    char buf[] = "a\rb";
    sanitize_string(buf, sizeof(buf));
    ASSERT(strcmp(buf, "ab") == 0, "carriage return removed");
    PASS();
}

static void test_sanitize_tab_kept(void) {
    TEST("tab preserved");
    char buf[] = "a\tb";
    sanitize_string(buf, sizeof(buf));
    ASSERT(strcmp(buf, "a\tb") == 0, "tab should be kept");
    PASS();
}

static void test_sanitize_all_invalid(void) {
    TEST("all chars stripped");
    char buf[] = "\001\002\003";
    sanitize_string(buf, sizeof(buf));
    ASSERT(strcmp(buf, "") == 0, "all stripped");
    PASS();
}

static void test_parse_positive_int_valid(void) {
    TEST("parse valid int");
    ASSERT_EQ(parse_positive_int("42"), 42, "42");
    ASSERT_EQ(parse_positive_int("0"), 0, "0");
    ASSERT_EQ(parse_positive_int("1"), 1, "1");
    PASS();
}

static void test_parse_positive_int_negative(void) {
    TEST("parse negative rejected");
    ASSERT_EQ(parse_positive_int("-5"), 0, "negative -> 0");
    PASS();
}

static void test_parse_positive_int_garbage(void) {
    TEST("parse garbage rejected");
    ASSERT_EQ(parse_positive_int("abc"), 0, "garbage -> 0");
    ASSERT_EQ(parse_positive_int(""), 0, "empty -> 0");
    ASSERT_EQ(parse_positive_int("12abc"), 0, "trailing chars -> 0");
    PASS();
}

static void test_parse_positive_int_overflow(void) {
    TEST("parse overflow rejected");
    ASSERT_EQ(parse_positive_int("9999999999999"), 0, "overflow -> 0");
    PASS();
}

int main(void) {
    printf("sanitize tests:\n");
    test_sanitize_normal();
    test_sanitize_control_chars();
    test_sanitize_carriage_return();
    test_sanitize_tab_kept();
    test_sanitize_all_invalid();
    test_parse_positive_int_valid();
    test_parse_positive_int_negative();
    test_parse_positive_int_garbage();
    test_parse_positive_int_overflow();

    printf("\n%d tests, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
