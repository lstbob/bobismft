#include "commands.h"
#include "meal_plan.h"
#include "date_utils.h"
#include "storage.h"
#include "input.h"
#include "sanitize.h"
#include "editor.h"
#include "tui.h"
#include "grid.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_day_plan(const DayPlan *plan, const char *label) {
    char date_str[MAX_DATE_STR_LEN];
    format_date("long", plan->date, date_str, sizeof(date_str));
    printf("\n" ANSI_BOLD "%s:" ANSI_RESET " %s\n", label, date_str);
    for (int i = 0; i < SLOT_COUNT; i++) {
        if (plan->has_meal[i]) {
            printf("  " ANSI_BOLD ANSI_YELLOW "%s:" ANSI_RESET " %s\n",
                   SLOT_NAMES[i], plan->meals[i].name);
            for (int j = 0; j < plan->meals[i].ingredient_count; j++) {
                printf("    " ANSI_DIM "- %s" ANSI_RESET "\n",
                       plan->meals[i].ingredients[j]);
            }
        }
    }
}

static bool plan_day_via_editor(Date date, bool *skipped) {
    char date_str[MAX_DATE_STR_LEN];
    format_date("long", date, date_str, sizeof(date_str));
    printf("\n--- Meal plan for %s ---\n", date_str);

    char initial[2048];
    size_t initial_len = 0;

    if (plan_exists(date)) {
        DayPlan existing;
        load_day_plan(date, &existing);
        print_day_plan(&existing, "Existing plan");
        initial_len = day_plan_serialize(&existing, initial, sizeof(initial));
    } else {
        DayPlan empty;
        day_plan_clear(&empty);
        empty.date = date;
        initial_len = day_plan_serialize(&empty, initial, sizeof(initial));
    }

    printf("Opening editor...\n");
    char *edited = NULL;
    size_t edited_len = 0;
    if (!open_editor(initial_len > 0 ? initial : NULL, initial_len,
                     &edited, &edited_len)) {
        fprintf(stderr, "error: editor failed\n");
        if (skipped) *skipped = true;
        return false;
    }

    DayPlan plan;
    day_plan_clear(&plan);
    plan.date = date;
    if (!day_plan_parse(edited, &plan)) {
        fprintf(stderr, "error: could not parse plan\n");
        free(edited);
        if (skipped) *skipped = true;
        return false;
    }
    free(edited);

    bool any_meal = false;
    for (int i = 0; i < SLOT_COUNT; i++) {
        if (plan.has_meal[i]) { any_meal = true; break; }
    }

    if (!any_meal) {
        printf("No meals entered — nothing to save.\n");
        if (skipped) *skipped = true;
        return false;
    }

    if (!save_day_plan(&plan)) {
        fprintf(stderr, "error: could not save plan for %s\n", date_str);
        if (skipped) *skipped = true;
        return false;
    }

    printf("Saved plan for %s\n", date_str);
    if (skipped) *skipped = false;
    return true;
}

static Date prompt_date_default(Date def) {
    char ds[MAX_DATE_STR_LEN];
    format_date("long", def, ds, sizeof(ds));
    printf("Date for meal plan (Enter for %s, or type a date): ", ds);
    fflush(stdout);

    char input[128];
    if (!read_line(input, sizeof(input)) || input[0] == '\0') {
        return def;
    }

    Date d;
    if (parse_date(input, &d)) return d;

    printf("Could not parse date. Using %s.\n", ds);
    return def;
}

static Date prompt_date(void) {
    return prompt_date_default(date_tomorrow());
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
            if (!plan_day_via_editor(d, &was_skipped)) {
                if (was_skipped) skipped++;
            } else {
                planned++;
            }
        }
        printf("\nWeekly plan complete: %d days planned, %d days kept existing.\n",
               planned, skipped);
    } else {
        Date target = prompt_date();
        plan_day_via_editor(target, NULL);
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
                    printf("\n  " ANSI_BOLD ANSI_CYAN "%s" ANSI_RESET "\n\n", ds);
                    printf("  " ANSI_DIM "(no meal plan for this day)" ANSI_RESET "\n");
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
                    printf("\n  " ANSI_BOLD ANSI_CYAN "Weekly View" ANSI_RESET "\n\n");
                    printf("  " ANSI_DIM "(no meal plans this week)" ANSI_RESET "\n");
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

        printf("\n  " ANSI_DIM "[" ANSI_RESET ANSI_BOLD ANSI_GREEN "i" ANSI_RESET ANSI_DIM "]" ANSI_RESET "%s"
               "  " ANSI_DIM "[" ANSI_RESET ANSI_BOLD ANSI_GREEN "n" ANSI_RESET ANSI_DIM "]next" ANSI_RESET
               "  " ANSI_DIM "[" ANSI_RESET ANSI_BOLD ANSI_GREEN "N" ANSI_RESET ANSI_DIM "]prev" ANSI_RESET
               "  " ANSI_DIM "[" ANSI_RESET ANSI_BOLD ANSI_GREEN "e" ANSI_RESET ANSI_DIM "]edit" ANSI_RESET
               "  " ANSI_DIM "[" ANSI_RESET ANSI_BOLD ANSI_RED "q" ANSI_RESET ANSI_DIM "]quit" ANSI_RESET,
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
            case 'e': {
                raw_mode_disable();
                printf("\n");
                Date d = prompt_date_default(cursor);
                plan_day_via_editor(d, NULL);
                raw_mode_enable();
                break;
            }
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
