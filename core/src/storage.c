#include "storage.h"
#include "date_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#define MAGIC 0x4F484F42
#define VERSION 2

static uint8_t compute_checksum(const uint8_t *data, size_t len) {
    uint8_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum ^= data[i];
    }
    return sum;
}

static const char *file_path(Date date, char *buf, size_t buf_size) {
    snprintf(buf, buf_size, DATA_DIR "/%04d-%02d-%02d.ohb",
             date.year, date.month, date.day);
    return buf;
}

bool storage_init(void) {
    struct stat st = {0};
    if (stat(DATA_DIR, &st) == -1) {
        if (mkdir(DATA_DIR, 0700) == -1) {
            return false;
        }
    }
    return true;
}

bool save_day_plan(const DayPlan *plan) {
    char path[256];
    file_path(plan->date, path, sizeof(path));

    FILE *f = fopen(path, "w+b");
    if (!f) return false;

    uint32_t magic = MAGIC;
    uint8_t version = VERSION;
    fwrite(&magic, sizeof(magic), 1, f);
    fwrite(&version, sizeof(version), 1, f);
    fputc(0, f);

    fwrite(&plan->date.year, sizeof(plan->date.year), 1, f);
    fwrite(&plan->date.month, sizeof(plan->date.month), 1, f);
    fwrite(&plan->date.day, sizeof(plan->date.day), 1, f);

    for (int i = 0; i < SLOT_COUNT; i++) {
        uint8_t has = plan->has_meal[i] ? 1 : 0;
        fwrite(&has, sizeof(has), 1, f);
        if (plan->has_meal[i]) {
            fwrite(&plan->meals[i], sizeof(Meal), 1, f);
        }
    }

    {
        long data_end = ftell(f);
        uint8_t *buf = (uint8_t *)malloc((size_t)data_end);
        if (buf) {
            fseek(f, 0, SEEK_SET);
            size_t nread = fread(buf, 1, (size_t)data_end, f);
            if (nread > 0) {
                uint8_t sum = compute_checksum(buf + 6, nread - 6);
                fseek(f, 5, SEEK_SET);
                fwrite(&sum, 1, 1, f);
            }
            free(buf);
        }
    }

    fclose(f);
    return true;
}

bool load_day_plan(Date date, DayPlan *out) {
    char path[256];
    file_path(date, path, sizeof(path));

    FILE *f = fopen(path, "rb");
    if (!f) return false;

    uint32_t magic;
    uint8_t version;
    if (fread(&magic, sizeof(magic), 1, f) != 1 || magic != MAGIC) {
        fclose(f);
        return false;
    }
    if (fread(&version, sizeof(version), 1, f) != 1 || version != VERSION) {
        fclose(f);
        return false;
    }

    uint8_t stored_checksum;
    if (fread(&stored_checksum, 1, 1, f) != 1) {
        fclose(f);
        return false;
    }

    long data_start = ftell(f);
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, data_start, SEEK_SET);

    size_t data_len = (size_t)(file_size - data_start);
    uint8_t *data = (uint8_t *)malloc(data_len + 1);
    if (!data) {
        fclose(f);
        return false;
    }
    size_t nread = fread(data, 1, data_len, f);
    if (nread != data_len || compute_checksum(data, data_len) != stored_checksum) {
        free(data);
        fclose(f);
        return false;
    }

    fseek(f, (long)data_start, SEEK_SET);
    fread(&out->date.year, sizeof(out->date.year), 1, f);
    fread(&out->date.month, sizeof(out->date.month), 1, f);
    fread(&out->date.day, sizeof(out->date.day), 1, f);

    for (int i = 0; i < SLOT_COUNT; i++) {
        uint8_t has;
        fread(&has, sizeof(has), 1, f);
        out->has_meal[i] = has ? true : false;
        if (out->has_meal[i]) {
            fread(&out->meals[i], sizeof(Meal), 1, f);
        } else {
            meal_clear(&out->meals[i]);
        }
    }

    free(data);
    fclose(f);
    return true;
}

bool plan_exists(Date date) {
    char path[256];
    file_path(date, path, sizeof(path));
    return access(path, F_OK) == 0;
}

int list_plans_in_range(Date start, Date end, Date *out, size_t max_count) {
    int count = 0;
    Date current = start;

    while (date_cmp(current, end) <= 0 && (size_t)count < max_count) {
        if (plan_exists(current)) {
            out[count] = current;
            count++;
        }
        current = add_days(current, 1);
    }

    return count;
}
