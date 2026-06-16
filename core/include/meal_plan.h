#ifndef MEAL_PLAN_H
#define MEAL_PLAN_H

#include <stdbool.h>

#define MAX_MEAL_NAME_LEN 64
#define MAX_INGREDIENT_LEN 48
#define MAX_INGREDIENTS 5

typedef enum {
    SLOT_BREAKFAST,
    SLOT_LUNCH,
    SLOT_DINNER,
    SLOT_SNACK,
    SLOT_COUNT
} MealSlotType;

extern const char *SLOT_NAMES[SLOT_COUNT];

typedef struct {
    char name[MAX_MEAL_NAME_LEN];
    char ingredients[MAX_INGREDIENTS][MAX_INGREDIENT_LEN];
    int ingredient_count;
} Meal;

typedef struct {
    int year;
    int month;
    int day;
} Date;

typedef struct {
    Date date;
    Meal meals[SLOT_COUNT];
    bool has_meal[SLOT_COUNT];
} DayPlan;

typedef struct {
    DayPlan days[7];
    int day_count;
} WeekPlan;

void meal_clear(Meal *meal);
void day_plan_clear(DayPlan *plan);
void week_plan_clear(WeekPlan *plan);
bool date_valid(Date d);
int date_cmp(Date a, Date b);

#endif
