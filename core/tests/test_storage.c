#include "storage.h"
#include "meal_plan.h"
#include "date_utils.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
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

/*
 * Write a raw .ohb file with a correct header and checksum but caller-supplied
 * (potentially hostile) date/meal bytes. Mirrors the on-disk layout written by
 * save_day_plan so we can forge a structurally valid but malicious record.
 * MAGIC/VERSION are duplicated from storage.c intentionally.
 */
static void write_raw_plan(const char *path, Date date,
                           const uint8_t has[SLOT_COUNT], const Meal meals[SLOT_COUNT]) {
    uint8_t data[16 + SLOT_COUNT * (1 + sizeof(Meal))];
    size_t pos = 0;
    memcpy(data + pos, &date.year, sizeof(int));  pos += sizeof(int);
    memcpy(data + pos, &date.month, sizeof(int)); pos += sizeof(int);
    memcpy(data + pos, &date.day, sizeof(int));   pos += sizeof(int);
    for (int i = 0; i < SLOT_COUNT; i++) {
        data[pos++] = has[i];
        if (has[i]) {
            memcpy(data + pos, &meals[i], sizeof(Meal));
            pos += sizeof(Meal);
        }
    }
    uint8_t sum = 0;
    for (size_t k = 0; k < pos; k++) sum ^= data[k];

    FILE *f = fopen(path, "wb");
    if (!f) return;
    uint32_t magic = 0x4F484F42;
    uint8_t version = 2;
    fwrite(&magic, sizeof(magic), 1, f);
    fwrite(&version, sizeof(version), 1, f);
    fputc((int)sum, f);
    fwrite(data, 1, pos, f);
    fclose(f);
}

/* A crafted file with a huge ingredient_count must not yield an out-of-bounds
 * count; load must clamp it to [0, MAX_INGREDIENTS] and NUL-terminate strings. */
static void test_malicious_ingredient_count(void) {
    TEST("rejects oversized ingredient_count");
    Date d = {2026, 8, 15};
    Meal meals[SLOT_COUNT];
    uint8_t has[SLOT_COUNT] = {1, 0, 0, 0};
    memset(meals, 0, sizeof(meals));
    memset(meals[0].name, 'A', sizeof(meals[0].name));        /* no NUL terminator */
    for (int j = 0; j < MAX_INGREDIENTS; j++)
        memset(meals[0].ingredients[j], 'B', sizeof(meals[0].ingredients[j]));
    meals[0].ingredient_count = 1000000;                      /* hostile */

    const char *path = "data/2026-08-15.ohb";
    write_raw_plan(path, d, has, meals);

    DayPlan loaded;
    day_plan_clear(&loaded);
    ASSERT(load_day_plan(d, &loaded), "structurally valid file should load");
    ASSERT(loaded.meals[0].ingredient_count >= 0 &&
           loaded.meals[0].ingredient_count <= MAX_INGREDIENTS,
           "ingredient_count must be clamped");
    ASSERT(strlen(loaded.meals[0].name) < MAX_MEAL_NAME_LEN, "name must be terminated");
    for (int j = 0; j < MAX_INGREDIENTS; j++)
        ASSERT(strlen(loaded.meals[0].ingredients[j]) < MAX_INGREDIENT_LEN,
               "ingredient must be terminated");
    unlink(path);
    PASS();
}

/* A negative ingredient_count must not become a negative loop bound. */
static void test_negative_ingredient_count(void) {
    TEST("rejects negative ingredient_count");
    Date d = {2026, 8, 16};
    Meal meals[SLOT_COUNT];
    uint8_t has[SLOT_COUNT] = {1, 0, 0, 0};
    memset(meals, 0, sizeof(meals));
    snprintf(meals[0].name, MAX_MEAL_NAME_LEN, "Eggs");
    meals[0].ingredient_count = -5;

    const char *path = "data/2026-08-16.ohb";
    write_raw_plan(path, d, has, meals);

    DayPlan loaded;
    day_plan_clear(&loaded);
    ASSERT(load_day_plan(d, &loaded), "structurally valid file should load");
    ASSERT_EQ(loaded.meals[0].ingredient_count, 0, "negative count must clamp to 0");
    unlink(path);
    PASS();
}

/* An out-of-range month would index MONTH_NAMES[month-1] in the renderer;
 * load must reject the record outright. */
static void test_bogus_date_rejected(void) {
    TEST("rejects out-of-range date");
    Date stored = {2026, 0, 15};   /* month 0 -> MONTH_NAMES[-1] if not rejected */
    Meal meals[SLOT_COUNT];
    uint8_t has[SLOT_COUNT] = {0, 0, 0, 0};
    memset(meals, 0, sizeof(meals));

    /* file_path() derives the name from the requested date, so name it for a
     * valid lookup date but store a bogus date inside. */
    const char *path = "data/2026-08-17.ohb";
    write_raw_plan(path, stored, has, meals);

    Date lookup = {2026, 8, 17};
    DayPlan loaded;
    day_plan_clear(&loaded);
    ASSERT(!load_day_plan(lookup, &loaded), "bogus stored date must be rejected");
    unlink(path);
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
    test_malicious_ingredient_count();
    test_negative_ingredient_count();
    test_bogus_date_rejected();
    test_cleanup();

    printf("\n%d tests, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
