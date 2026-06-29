#include "meal_plan.h"
#include "sanitize.h"
#include <stdio.h>
#include <stdlib.h>
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

static size_t copy_truncated(char *dst, size_t dst_size, const char *src) {
    if (dst_size == 0) return 0;
    size_t i = 0;
    for (; i + 1 < dst_size && src[i] != '\0'; i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
    sanitize_string(dst, dst_size);
    return i;
}

size_t day_plan_serialize(const DayPlan *plan, char *out, size_t out_size) {
    if (!plan || !out || out_size == 0) return 0;

    size_t pos = 0;
    for (int i = 0; i < SLOT_COUNT; i++) {
        if (!plan->has_meal[i]) continue;

        int n = snprintf(out + pos, out_size - pos, "# %s\n",
                         SLOT_NAMES[i]);
        if (n < 0 || (size_t)n >= out_size - pos) return pos; /* truncated */
        pos += (size_t)n;

        if (plan->meals[i].name[0] != '\0') {
            n = snprintf(out + pos, out_size - pos, "%s\n",
                         plan->meals[i].name);
            if (n < 0 || (size_t)n >= out_size - pos) return pos;
            pos += (size_t)n;
        }

        for (int j = 0; j < plan->meals[i].ingredient_count; j++) {
            if (plan->meals[i].ingredients[j][0] == '\0') continue;
            n = snprintf(out + pos, out_size - pos, "- %s\n",
                         plan->meals[i].ingredients[j]);
            if (n < 0 || (size_t)n >= out_size - pos) return pos;
            pos += (size_t)n;
        }

        n = snprintf(out + pos, out_size - pos, "\n");
        if (n < 0 || (size_t)n >= out_size - pos) return pos;
        pos += (size_t)n;
    }

    if (pos == 0) {
        int n = snprintf(out, out_size,
            "# Edit this file. Lines beginning with '#' start a section\n"
            "# (e.g. # Breakfast). The next non-empty line is the meal name.\n"
            "# Lines starting with '- ' list ingredients (max %d).\n"
            "# A blank line ends a section. Save & quit to confirm.\n\n",
            MAX_INGREDIENTS);
        if (n > 0 && (size_t)n < out_size) pos = (size_t)n;
    }

    out[out_size - 1] = '\0';
    return pos;
}

static int find_slot_by_name(const char *name) {
    for (int i = 0; i < SLOT_COUNT; i++) {
        if (strcmp(name, SLOT_NAMES[i]) == 0) return i;
    }
    return -1;
}

static void rstrip(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r' ||
                       s[len - 1] == ' '  || s[len - 1] == '\t')) {
        s[--len] = '\0';
    }
}

static char *ltrim(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

bool day_plan_parse(const char *text, DayPlan *out) {
    if (!text || !out) return false;

    day_plan_clear(out);

    char *buf = strdup(text);
    if (!buf) return false;

    int current = -1;            /* index of section being filled, -1 none */
    bool have_name = false;      /* meal name set for current section */

    char *line = buf;
    while (line && *line) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        rstrip(line);

        /* section header: "# <SlotName>" */
        if (strncmp(line, "# ", 2) == 0) {
            char *name = ltrim(line + 2);
            int slot = find_slot_by_name(name);
            if (slot >= 0) {
                if (current >= 0 && have_name) {
                    out->has_meal[current] = true;
                }
                current = slot;
                have_name = false;
                meal_clear(&out->meals[slot]);
                out->has_meal[slot] = false;
            } else {
                current = -1;
                have_name = false;
            }
            if (!nl) break;
            line = nl + 1;
            continue;
        }

        /* other comment lines (e.g. "#!..." or "#standalone") ignored */
        if (line[0] == '#') {
            if (!nl) break;
            line = nl + 1;
            continue;
        }

        /* blank line ends the current section */
        if (line[0] == '\0') {
            if (current >= 0 && have_name) {
                out->has_meal[current] = true;
            }
            current = -1;
            have_name = false;
            if (!nl) break;
            line = nl + 1;
            continue;
        }

        if (current < 0) {
            /* stray non-empty line outside a section: ignore */
            if (!nl) break;
            line = nl + 1;
            continue;
        }

        char *p = ltrim(line);

        /* ingredient line */
        if ((p[0] == '-' && p[1] == ' ') ||
            (p[0] == '*' && p[1] == ' ')) {
            char *ing = p + 2;
            int cnt = out->meals[current].ingredient_count;
            if (cnt < MAX_INGREDIENTS) {
                copy_truncated(out->meals[current].ingredients[cnt],
                               sizeof(out->meals[current].ingredients[cnt]),
                               ing);
                if (out->meals[current].ingredients[cnt][0] != '\0') {
                    out->meals[current].ingredient_count = cnt + 1;
                }
            }
            if (!nl) break;
            line = nl + 1;
            continue;
        }

        /* meal name */
        if (!have_name) {
            copy_truncated(out->meals[current].name,
                           sizeof(out->meals[current].name), p);
            have_name = (out->meals[current].name[0] != '\0');
        }

        if (!nl) break;
        line = nl + 1;
    }

    /* flush the last open section */
    if (current >= 0 && have_name) {
        out->has_meal[current] = true;
    }

    free(buf);
    return true;
}
