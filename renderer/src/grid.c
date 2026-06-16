#include "grid.h"
#include "tui.h"
#include "date_utils.h"
#include "storage.h"

#include <stdio.h>
#include <string.h>

const char *VIEW_NAMES[VIEW_COUNT] = {
    "Daily",
    "Weekly",
    "Monthly"
};

void grid_clear(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
}

static void print_trunc(const char *s, int max_width) {
    int len = (int)strlen(s);
    if (len <= max_width) {
        printf("%s", s);
        for (int i = len; i < max_width; i++) putchar(' ');
    } else {
        printf("%.*s", max_width - 1, s);
        putchar('>');
    }
}

static int days_in_month(int year, int month) {
    int dim[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) {
        return 29;
    }
    return dim[month - 1];
}

static const char *DOW_SHORT[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void grid_draw_day(const DayPlan *plan, TermSize term) {
    char date_str[MAX_DATE_STR_LEN];
    format_date("long", plan->date, date_str, sizeof(date_str));
    (void)term;

    printf("\n  " ANSI_BOLD ANSI_CYAN "%s" ANSI_RESET "\n\n", date_str);

    int meals_found = 0;
    for (int i = 0; i < SLOT_COUNT; i++) {
        if (plan->has_meal[i]) {
            printf("  " ANSI_BOLD ANSI_YELLOW "%s:" ANSI_RESET " %s\n",
                   SLOT_NAMES[i], plan->meals[i].name);
            for (int j = 0; j < plan->meals[i].ingredient_count; j++) {
                printf("    " ANSI_DIM "- %s" ANSI_RESET "\n",
                       plan->meals[i].ingredients[j]);
            }
            meals_found++;
        }
    }

    if (meals_found == 0) {
        printf("  " ANSI_DIM "(no meals planned)" ANSI_RESET "\n");
    }
}

static int col_width(TermSize term) {
    int w = (term.cols - 8) / 7;
    if (w < 8) w = 8;
    return w;
}

void grid_draw_week(DayPlan days[7], int count, TermSize term) {
    int cw = col_width(term);

    printf("\n  " ANSI_BOLD ANSI_CYAN "Weekly Meal Plan" ANSI_RESET "\n\n");

    for (int d = 0; d < count; d++) {
        if (d > 0) printf(" ");
        char buf[32];
        int dow = day_of_week(days[d].date);
        snprintf(buf, sizeof(buf), "%s %02d", DOW_SHORT[dow], days[d].date.day);
        printf(ANSI_BOLD ANSI_YELLOW "%-*s" ANSI_RESET, cw, buf);
    }
    printf("\n");

    for (int slot = 0; slot < SLOT_COUNT; slot++) {
        for (int d = 0; d < count; d++) {
            if (d > 0) printf(" ");
            char cell[128];
            if (days[d].has_meal[slot]) {
                snprintf(cell, sizeof(cell), "%s", days[d].meals[slot].name);
                printf(ANSI_GREEN);
                print_trunc(cell, cw);
                printf(ANSI_RESET);
            } else {
                cell[0] = '\0';
                printf(ANSI_DIM);
                print_trunc(cell, cw);
                printf(ANSI_RESET);
            }
        }
        printf("\n");

        for (int d = 0; d < count; d++) {
            if (d > 0) printf(" ");
            if (days[d].has_meal[slot] && days[d].meals[slot].ingredient_count > 0) {
                char line[256];
                int pos = 0;
                for (int j = 0; j < days[d].meals[slot].ingredient_count && pos < cw; j++) {
                    if (j > 0) {
                        line[pos++] = ',';
                        line[pos++] = ' ';
                    }
                    const char *ing = days[d].meals[slot].ingredients[j];
                    while (*ing && pos < cw) {
                        line[pos++] = *ing;
                        ing++;
                    }
                }
                line[pos] = '\0';
                printf(ANSI_DIM);
                print_trunc(line, cw);
                printf(ANSI_RESET);
            } else {
                printf("%-*s", cw, "");
            }
        }
        printf("\n");
    }
}

void grid_draw_month(int year, int month, const Date *plans, int plan_count, TermSize term) {
    (void)term;
    const char *mn = month_name(month);
    char month_str[64];
    if (mn && mn[0]) {
        char cap[32];
        cap[0] = (char)(mn[0] - 32);
        size_t i = 1;
        while (mn[i] && i < sizeof(cap) - 1) {
            cap[i] = mn[i];
            i++;
        }
        cap[i] = '\0';
        snprintf(month_str, sizeof(month_str), "%s %d", cap, year);
    } else {
        snprintf(month_str, sizeof(month_str), "%d", year);
    }

    printf("\n  " ANSI_BOLD ANSI_CYAN "%s" ANSI_RESET "\n\n", month_str);

    for (int i = 0; i < 7; i++) {
        if (i > 0) printf(" ");
        printf(ANSI_BOLD ANSI_YELLOW "%3s" ANSI_RESET, DOW_SHORT[i]);
    }
    printf("\n");

    Date first = {year, month, 1};
    int start_dow = day_of_week(first);
    int dim = days_in_month(year, month);

    for (int i = 0; i < start_dow; i++) {
        if (i > 0) printf(" ");
        printf("    ");
    }

    for (int day = 1; day <= dim; day++) {
        if ((day + start_dow - 1) % 7 == 0 && day > 1) {
            printf("\n");
        } else if (day > 1) {
            printf(" ");
        }

        int has_plan = 0;
        for (int i = 0; i < plan_count; i++) {
            if (plans[i].year == year && plans[i].month == month && plans[i].day == day) {
                has_plan = 1;
                break;
            }
        }

        if (has_plan) {
            printf(ANSI_REVERSE ANSI_GREEN "%3d" ANSI_RESET, day);
        } else {
            printf("%3d", day);
        }
    }
    printf("\n");
}
