#include "commands.h"
#include "meal_plan.h"
#include "date_utils.h"
#include "storage.h"
#include "input.h"
#include "sanitize.h"
#include "tui.h"
#include "grid.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_day_plan(const DayPlan *plan, const char *label) {
    char date_str[MAX_DATE_STR_LEN];
    format_date("long", plan->date, date_str, sizeof(date_str));
    printf("\n=== %s: %s ===\n", label, date_str);
    for (int i = 0; i < SLOT_COUNT; i++) {
        if (plan->has_meal[i]) {
            printf("  %s: %s\n", SLOT_NAMES[i], plan->meals[i].name);
            for (int j = 0; j < plan->meals[i].ingredient_count; j++) {
                printf("      - %s\n", plan->meals[i].ingredients[j]);
            }
        }
    }
}

static bool plan_day_interactive(Date date, bool *skipped) {
    char date_str[MAX_DATE_STR_LEN];
    format_date("long", date, date_str, sizeof(date_str));
    printf("\n--- Planning %s ---\n", date_str);

    if (plan_exists(date)) {
        DayPlan existing;
        load_day_plan(date, &existing);
        print_day_plan(&existing, "Existing plan");

        if (!confirm("Overwrite existing plan")) {
            if (skipped) *skipped = true;
            return false;
        }
    }

    DayPlan plan;
    day_plan_clear(&plan);
    plan.date = date;

    for (int i = 0; i < SLOT_COUNT; i++) {
        printf("Meal for %s (empty to skip): ", SLOT_NAMES[i]);
        fflush(stdout);

        char meal_name[MAX_MEAL_NAME_LEN];
        if (!read_line(meal_name, sizeof(meal_name))) return false;
        if (meal_name[0] == '\0') continue;

        snprintf(plan.meals[i].name, MAX_MEAL_NAME_LEN, "%s", meal_name);

        printf("  Number of ingredients (max %d): ", MAX_INGREDIENTS);
        fflush(stdout);

        char count_buf[16];
        if (!read_line(count_buf, sizeof(count_buf))) return false;
        int count = parse_positive_int(count_buf);
        if (count > MAX_INGREDIENTS) count = MAX_INGREDIENTS;

        for (int j = 0; j < count; j++) {
            printf("  Ingredient %d: ", j + 1);
            fflush(stdout);
            if (!read_line(plan.meals[i].ingredients[j], MAX_INGREDIENT_LEN)) return false;
        }
        plan.meals[i].ingredient_count = count;
        plan.has_meal[i] = true;
    }

    if (!save_day_plan(&plan)) {
        fprintf(stderr, "error: could not save plan for %s\n", date_str);
        return false;
    }

    printf("Saved plan for %s\n", date_str);
    if (skipped) *skipped = false;
    return true;
}

static Date prompt_date(void) {
    printf("Date for meal plan (Enter for tomorrow, or type a date): ");
    fflush(stdout);

    char input[128];
    if (!read_line(input, sizeof(input)) || input[0] == '\0') {
        return date_tomorrow();
    }

    Date d;
    if (parse_date(input, &d)) return d;

    printf("Could not parse date. Using tomorrow.\n");
    return date_tomorrow();
}

int cmd_nmp(int argc, char *argv[]) {
    bool weekly = (argc >= 3 && strcmp(argv[2], "-w") == 0);

    if (weekly) {
        Date start = date_today();
        printf("Planning weekly meals starting %04d-%02d-%02d\n",
               start.year, start.month, start.day);

        int planned = 0;
        int skipped = 0;
        for (int i = 0; i < 7; i++) {
            Date d = add_days(start, i);
            bool was_skipped = false;
            if (!plan_day_interactive(d, &was_skipped)) {
                if (was_skipped) skipped++;
            } else {
                planned++;
            }
        }
        printf("\nWeekly plan complete: %d days planned, %d days kept existing.\n",
               planned, skipped);
    } else {
        Date target = prompt_date();
        plan_day_interactive(target, NULL);
    }

    return 0;
}

static void load_day_plans_for_month(int year, int month, DayPlan *out, int *count) {
    Date start = {year, month, 1};
    int dim = 31;
    if (month == 2) {
        dim = ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0) ? 29 : 28;
    } else if (month == 4 || month == 6 || month == 9 || month == 11) {
        dim = 30;
    }
    Date end = {year, month, dim};

    Date dates[31];
    int found = list_plans_in_range(start, end, dates, 31);
    *count = 0;
    for (int i = 0; i < found && *count < 31; i++) {
        if (load_day_plan(dates[i], &out[*count])) {
            (*count)++;
        }
    }
}

int cmd_smp(int argc, char *argv[]) {
    Date cursor = date_today();
    ViewMode view = VIEW_DAILY;

    if (argc >= 4 && strcmp(argv[2], "-d") == 0) {
        Date d;
        if (parse_date(argv[3], &d)) {
            cursor = d;
        }
    }

    raw_mode_enable();
    atexit(raw_mode_disable);

    int running = 1;
    while (running) {
        TermSize term = get_term_size();
        grid_clear();

        switch (view) {
            case VIEW_DAILY: {
                DayPlan plan;
                if (load_day_plan(cursor, &plan)) {
                    grid_draw_day(&plan, term);
                } else {
                    char ds[MAX_DATE_STR_LEN];
                    format_date("long", cursor, ds, sizeof(ds));
                    printf("\n  \033[1m%s\033[0m\n\n", ds);
                    printf("  (no meal plan for this day)\n");
                }
                break;
            }
            case VIEW_WEEKLY: {
                Date start = week_start(cursor);
                DayPlan days[7];
                int count = 0;
                for (int i = 0; i < 7; i++) {
                    Date d = add_days(start, i);
                    day_plan_clear(&days[i]);
                    days[i].date = d;
                    if (load_day_plan(d, &days[i])) {
                        count++;
                    }
                }
                if (count == 0) {
                    printf("\n  \033[1mWeekly View\033[0m\n\n");
                    printf("  (no meal plans this week)\n");
                } else {
                    grid_draw_week(days, 7, term);
                }
                break;
            }
            case VIEW_MONTHLY: {
                DayPlan plans[31];
                int count;
                load_day_plans_for_month(cursor.year, cursor.month, plans, &count);
                Date plan_dates[31];
                for (int i = 0; i < count; i++) {
                    plan_dates[i] = plans[i].date;
                }
                grid_draw_month(cursor.year, cursor.month, plan_dates, count, term);
                break;
            }
            default:
                break;
        }

        printf("\n  [\033[1mi\033[0m]%s  [\033[1mn\033[0m]next  [\033[1mN\033[0m]prev  [\033[1mq\033[0m]quit",
               view == VIEW_DAILY ? " weekly" : (view == VIEW_WEEKLY ? " monthly" : " daily"));
        printf("\n");
        fflush(stdout);

        int key = read_key();
        switch (key) {
            case 'q':
            case KEY_ESC:
                running = 0;
                break;
            case 'i':
                view = (ViewMode)((view + 1) % VIEW_COUNT);
                break;
            case 'n':
                switch (view) {
                    case VIEW_DAILY:   cursor = add_days(cursor, 1);       break;
                    case VIEW_WEEKLY:  cursor = add_days(cursor, 7);       break;
                    case VIEW_MONTHLY:
                        if (cursor.month == 12) {
                            cursor.year++;
                            cursor.month = 1;
                        } else {
                            cursor.month++;
                        }
                        break;
                    default: break;
                }
                break;
            case 'N':
                switch (view) {
                    case VIEW_DAILY:   cursor = add_days(cursor, -1);      break;
                    case VIEW_WEEKLY:  cursor = add_days(cursor, -7);      break;
                    case VIEW_MONTHLY:
                        if (cursor.month == 1) {
                            cursor.year--;
                            cursor.month = 12;
                        } else {
                            cursor.month--;
                        }
                        break;
                    default: break;
                }
                break;
            case KEY_RIGHT:
                if (view == VIEW_DAILY) cursor = add_days(cursor, 1);
                else if (view == VIEW_WEEKLY) cursor = add_days(cursor, 7);
                break;
            case KEY_LEFT:
                if (view == VIEW_DAILY) cursor = add_days(cursor, -1);
                else if (view == VIEW_WEEKLY) cursor = add_days(cursor, -7);
                break;
            default:
                break;
        }
    }

    raw_mode_disable();
    printf("\n");
    return 0;
}
