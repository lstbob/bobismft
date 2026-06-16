#include "storage.h"
#include "meal_plan.h"
#include "date_utils.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int tests_run = 0;
static int tests_failed = 0;

#define TEST(name) do { printf("  %s ... ", name); tests_run++; } while(0)
#define PASS() printf("ok\n")
#define FAIL(msg) do { printf("FAIL (%s)\n", msg); tests_failed++; } while(0)
#define ASSERT(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)
#define ASSERT_EQ(a, b, msg) do { if ((a) != (b)) { FAIL(msg); return; } } while(0)
#define ASSERT_STR_EQ(a, b, msg) do { if (strcmp(a, b) != 0) { FAIL(msg); return; } } while(0)

static void test_init(void) {
    TEST("storage_init");
    rmdir("test_data");
    ASSERT(storage_init(), "init should succeed");
    PASS();
}

static void test_save_and_load(void) {
    TEST("save and load roundtrip");
    DayPlan plan;
    day_plan_clear(&plan);
    plan.date = (Date){2026, 7, 4};

    plan.has_meal[SLOT_BREAKFAST] = true;
    snprintf(plan.meals[SLOT_BREAKFAST].name, MAX_MEAL_NAME_LEN, "Pancakes");
    plan.meals[SLOT_BREAKFAST].ingredient_count = 2;
    snprintf(plan.meals[SLOT_BREAKFAST].ingredients[0], MAX_INGREDIENT_LEN, "Flour");
    snprintf(plan.meals[SLOT_BREAKFAST].ingredients[1], MAX_INGREDIENT_LEN, "Eggs");

    plan.has_meal[SLOT_LUNCH] = true;
    snprintf(plan.meals[SLOT_LUNCH].name, MAX_MEAL_NAME_LEN, "Soup");
    plan.meals[SLOT_LUNCH].ingredient_count = 1;
    snprintf(plan.meals[SLOT_LUNCH].ingredients[0], MAX_INGREDIENT_LEN, "Tomato");

    ASSERT(save_day_plan(&plan), "save should succeed");

    DayPlan loaded;
    day_plan_clear(&loaded);
    ASSERT(load_day_plan(plan.date, &loaded), "load should succeed");

    ASSERT_EQ(loaded.date.year, 2026, "year");
    ASSERT_EQ(loaded.date.month, 7, "month");
    ASSERT_EQ(loaded.date.day, 4, "day");
    ASSERT(loaded.has_meal[SLOT_BREAKFAST], "breakfast should exist");
    ASSERT_STR_EQ(loaded.meals[SLOT_BREAKFAST].name, "Pancakes", "breakfast name");
    ASSERT_EQ(loaded.meals[SLOT_BREAKFAST].ingredient_count, 2, "ingredient count");
    ASSERT_STR_EQ(loaded.meals[SLOT_BREAKFAST].ingredients[0], "Flour", "ingredient 0");
    ASSERT_STR_EQ(loaded.meals[SLOT_BREAKFAST].ingredients[1], "Eggs", "ingredient 1");
    ASSERT(loaded.has_meal[SLOT_LUNCH], "lunch should exist");
    ASSERT_STR_EQ(loaded.meals[SLOT_LUNCH].name, "Soup", "lunch name");
    ASSERT(!loaded.has_meal[SLOT_DINNER], "dinner should be absent");
    ASSERT(!loaded.has_meal[SLOT_SNACK], "snack should be absent");
    PASS();
}

static void test_plan_exists(void) {
    TEST("plan_exists");
    Date d = {2026, 7, 4};
    ASSERT(plan_exists(d), "should exist after save");
    Date missing = {2026, 1, 1};
    ASSERT(!plan_exists(missing), "should not exist");
    PASS();
}

static void test_list_plans_in_range(void) {
    TEST("list_plans_in_range");
    Date start = {2026, 7, 1};
    Date end = {2026, 7, 31};
    Date results[31];
    int count = list_plans_in_range(start, end, results, 31);
    ASSERT_EQ(count, 1, "should find 1 plan");
    ASSERT_EQ(results[0].year, 2026, "year");
    ASSERT_EQ(results[0].month, 7, "month");
    ASSERT_EQ(results[0].day, 4, "day");
    PASS();
}

static void test_checksum_corruption(void) {
    TEST("checksum rejects corruption");
    Date d = {2026, 7, 4};
    ASSERT(plan_exists(d), "plan should exist");

    char path[256];
    snprintf(path, sizeof(path), "data/%04d-%02d-%02d.ohb", d.year, d.month, d.day);
    FILE *f = fopen(path, "r+b");
    ASSERT(f, "open for corruption");
    fseek(f, 100, SEEK_SET);
    fputc(0xFF, f);
    fclose(f);

    DayPlan loaded;
    day_plan_clear(&loaded);
    ASSERT(!load_day_plan(d, &loaded), "should reject corrupted file");
    PASS();
}

static void test_cleanup(void) {
    TEST("cleanup test data");
    unlink("data/2026-07-04.ohb");
    rmdir("test_data");
    PASS();
}

int main(void) {
    printf("storage tests:\n");
    test_init();
    test_save_and_load();
    test_plan_exists();
    test_list_plans_in_range();
    test_checksum_corruption();
    test_cleanup();

    printf("\n%d tests, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
