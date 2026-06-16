#ifndef DATE_UTILS_H
#define DATE_UTILS_H

#include "meal_plan.h"
#include <stdbool.h>
#include <stddef.h>

#define MAX_DATE_STR_LEN 64

bool parse_date(const char *input, Date *out);
bool parse_english_date(const char *input, Date *out);
int format_date(const char *input, Date d, char *out, size_t out_len);
Date date_today(void);
Date date_tomorrow(void);
Date add_days(Date d, int days);
int day_of_week(Date d);
Date week_start(Date d);
const char *month_name(int month);

#endif
