#include "meal_plan.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int tests_run = 0;
static int tests_failed = 0;

#define TEST(name) do { printf("  %s ... ", name); tests_run++; } while(0)
#define PASS() printf("ok\n")
#define FAIL(msg) do { printf("FAIL (%s)\n", msg); tests_failed++; } while(0)
#define ASSERT(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)
#define ASSERT_STR_EQ(a, b, msg) do { if (strcmp(a, b) != 0) { FAIL(msg); return; } } while(0)
#define ASSERT_EQ(a, b, msg) do { if ((a) != (b)) { FAIL(msg); return; } } while(0)

static void test_serialize_empty(void) {
    TEST("serialize empty plan");
    DayPlan p;
    day_plan_clear(&p);
    char buf[256];
    size_t n = day_plan_serialize(&p, buf, sizeof(buf));
    ASSERT(n > 0, "nonzero length for empty (template)");
    ASSERT(strstr(buf, "Breakfast") != NULL ||
           strstr(buf, "edit") != NULL, "template present");
    PASS();
}

static void test_roundtrip(void) {
    TEST("serialize/parse roundtrip");
    DayPlan p;
    day_plan_clear(&p);
    p.date = (Date){2026, 6, 29};
    p.has_meal[SLOT_BREAKFAST] = true;
    snprintf(p.meals[SLOT_BREAKFAST].name, MAX_MEAL_NAME_LEN, "Oatmeal");
    snprintf(p.meals[SLOT_BREAKFAST].ingredients[0], MAX_INGREDIENT_LEN, "oats");
    snprintf(p.meals[SLOT_BREAKFAST].ingredients[1], MAX_INGREDIENT_LEN, "milk");
    p.meals[SLOT_BREAKFAST].ingredient_count = 2;

    p.has_meal[SLOT_DINNER] = true;
    snprintf(p.meals[SLOT_DINNER].name, MAX_MEAL_NAME_LEN, "Pasta");

    char buf[1024];
    size_t n = day_plan_serialize(&p, buf, sizeof(buf));
    ASSERT(n > 0, "serialize produced output");

    DayPlan out;
    day_plan_clear(&out);
    bool ok = day_plan_parse(buf, &out);
    ASSERT(ok, "parse ok");
    ASSERT(out.has_meal[SLOT_BREAKFAST], "breakfast set");
    ASSERT(out.has_meal[SLOT_DINNER], "dinner set");
    ASSERT(!out.has_meal[SLOT_LUNCH], "lunch not set");
    ASSERT_STR_EQ(out.meals[SLOT_BREAKFAST].name, "Oatmeal", "name match");
    ASSERT_EQ(out.meals[SLOT_BREAKFAST].ingredient_count, 2, "ingredient count");
    ASSERT_STR_EQ(out.meals[SLOT_BREAKFAST].ingredients[0], "oats", "ing0");
    ASSERT_STR_EQ(out.meals[SLOT_DINNER].name, "Pasta", "dinner name");
    PASS();
}

static void test_parse_ignores_garbage(void) {
    TEST("parse ignores garbage outside sections");
    const char *text =
        "random stray line\n"
        "# Breakfast\n"
        "Eggs\n"
        "- salt\n"
        "\n"
        "another stray\n";
    DayPlan out;
    day_plan_clear(&out);
    bool ok = day_plan_parse(text, &out);
    ASSERT(ok, "parse ok");
    ASSERT(out.has_meal[SLOT_BREAKFAST], "breakfast set");
    ASSERT_STR_EQ(out.meals[SLOT_BREAKFAST].name, "Eggs", "name");
    ASSERT_EQ(out.meals[SLOT_BREAKFAST].ingredient_count, 1, "one ingredient");
    ASSERT_STR_EQ(out.meals[SLOT_BREAKFAST].ingredients[0], "salt", "ing");
    PASS();
}

static void test_parse_caps_ingredients(void) {
    TEST("parse caps ingredients at MAX");
    char text[1024];
    snprintf(text, sizeof(text), "# Snack\nTrailMix\n");
    for (int i = 0; i < MAX_INGREDIENTS + 5; i++) {
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "- i%d\n", i);
        strcat(text, tmp);
    }
    DayPlan out;
    day_plan_clear(&out);
    bool ok = day_plan_parse(text, &out);
    ASSERT(ok, "parse ok");
    ASSERT_EQ(out.meals[SLOT_SNACK].ingredient_count, MAX_INGREDIENTS, "capped");
    PASS();
}

int main(void) {
    printf("meal_plan serialize tests:\n");
    test_serialize_empty();
    test_roundtrip();
    test_parse_ignores_garbage();
    test_parse_caps_ingredients();

    printf("\n%d tests, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}