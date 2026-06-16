#include "meal_plan.h"
#include <string.h>

const char *SLOT_NAMES[SLOT_COUNT] = {
    "Breakfast",
    "Lunch",
    "Dinner",
    "Snack"
};

void meal_clear(Meal *meal) {
    meal->name[0] = '\0';
    meal->ingredient_count = 0;
    for (int i = 0; i < MAX_INGREDIENTS; i++) {
        meal->ingredients[i][0] = '\0';
    }
}

void day_plan_clear(DayPlan *plan) {
    plan->date.year = 0;
    plan->date.month = 0;
    plan->date.day = 0;
    for (int i = 0; i < SLOT_COUNT; i++) {
        meal_clear(&plan->meals[i]);
        plan->has_meal[i] = false;
    }
}

void week_plan_clear(WeekPlan *plan) {
    plan->day_count = 0;
    for (int i = 0; i < 7; i++) {
        day_plan_clear(&plan->days[i]);
    }
}

bool date_valid(Date d) {
    if (d.year < 2020 || d.year > 2099) return false;
    if (d.month < 1 || d.month > 12) return false;
    if (d.day < 1 || d.day > 31) return false;

    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if ((d.year % 4 == 0 && d.year % 100 != 0) || (d.year % 400 == 0)) {
        days_in_month[1] = 29;
    }

    return d.day <= days_in_month[d.month - 1];
}

int date_cmp(Date a, Date b) {
    if (a.year != b.year) return a.year - b.year;
    if (a.month != b.month) return a.month - b.month;
    return a.day - b.day;
}
