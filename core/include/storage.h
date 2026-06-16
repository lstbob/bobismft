#ifndef STORAGE_H
#define STORAGE_H

#include "meal_plan.h"
#include <stdbool.h>
#include <stddef.h>

#define DATA_DIR "data"

bool storage_init(void);
bool save_day_plan(const DayPlan *plan);
bool load_day_plan(Date date, DayPlan *out);
bool plan_exists(Date date);
int list_plans_in_range(Date start, Date end, Date *out, size_t max_count);

#endif
