#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include <sys/types.h>

// [Keep all the typedef structs from before - snapshot_t, etc.]
typedef struct snapshot {
    uint64_t timestamp_sec;
    uint32_t kmalloc_1k_active;
    uint32_t kmalloc_4k_active;
    uint32_t slab_reclaimable_objs;
    uint32_t slab_unreclaimable_objs;
    uint64_t slabs_scanned;
    uint64_t pgalloc_dma;
    uint64_t pgsteal_kswapd;
    uint32_t order2_free_pages;
    uint32_t order3_free_pages;
    uint64_t metaspace_used_kb;
    uint64_t metaspace_committed_kb;
    double slabs_scanned_per_sec;
    double allocation_rate_kb_per_sec;
    double fragmentation_index;
    struct snapshot *next;
} snapshot_t;

typedef struct {
    snapshot_t *head;
    snapshot_t *tail;
    size_t count;
} snapshot_list_t;

typedef struct {
    double correlation;
    double coefficient_var;
    double mean_pressure;
} correlation_result_t;

volatile sig_atomic_t running = 1;
int debug_mode = 0;  // NEW: Debug flag

#define INTERVAL_STARTUP  1
#define INTERVAL_NORMAL   5
#define INTERVAL_IDLE     10

void signal_handler(int signum);
double calculate_mean(double *data, size_t n);
double calculate_stddev(double *data, size_t n);
double pearson_correlation(double *x, double *y, size_t n);
correlation_result_t analyze_correlation(snapshot_list_t *list);
void generate_report(snapshot_list_t *list);
void cleanup_list(snapshot_list_t *list);

void signal_handler(int signum) {
    (void)signum;
    running = 0;
    printf("\n\nReceived interrupt signal. Generating report...\n");
}

// IMPROVED: Better slabinfo parser with debug output
int parse_slabinfo(snapshot_t *snap) {
    FILE *fp = fopen("/proc/slabinfo", "r");
    if (!fp) {
        perror("Cannot open /proc/slabinfo");
        return -1;
    }

    char line[512];
    int found_1k = 0, found_4k = 0;

    // Skip 2 header lines
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp)) {
        char name[64];
        unsigned long active_objs, num_objs, objsize;

        if (sscanf(line, "%63s %lu %lu %lu",
                   name, &active_objs, &num_objs, &objsize) < 4) {
            continue;
        }

        // Try multiple possible names
        if (strcmp(name, "kmalloc-1024") == 0 ||
            strcmp(name, "kmalloc-1k") == 0 ||
            strcmp(name, "kmalloc-0001024") == 0) {
            snap->kmalloc_1k_active = active_objs;
            found_1k = 1;
        }
        else if (strcmp(name, "kmalloc-4096") == 0 ||
                 strcmp(name, "kmalloc-4k") == 0 ||
                 strcmp(name, "kmalloc-0004096") == 0) {
            snap->kmalloc_4k_active = active_objs;
            found_4k = 1;
        }
    }

    if (debug_mode && (!found_1k || !found_4k)) {
        fprintf(stderr, "DEBUG: Slab parsing - 1K found: %d, 4K found: %d\n",
                found_1k, found_4k);
    }

    fclose(fp);
    return 0;
}

int parse_vmstat(snapshot_t *snap) {
    FILE *fp = fopen("/proc/vmstat", "r");
    if (!fp) {
        perror("Cannot open /proc/vmstat");
        return -1;
    }

    char key[64];
    uint64_t value;

    while (fscanf(fp, "%63s %lu", key, &value) == 2) {
        if (strcmp(key, "slabs_scanned") == 0) {
            snap->slabs_scanned = value;
        }
        else if (strcmp(key, "pgalloc_dma") == 0) {
            snap->pgalloc_dma = value;
        }
        else if (strcmp(key, "pgsteal_kswapd") == 0) {
            snap->pgsteal_kswapd = value;
        }
        else if (strcmp(key, "nr_slab_reclaimable") == 0) {
            snap->slab_reclaimable_objs = value;
        }
        else if (strcmp(key, "nr_slab_unreclaimable") == 0) {
            snap->slab_unreclaimable_objs = value;
        }
    }

    fclose(fp);
    return 0;
}

int parse_buddyinfo(snapshot_t *snap) {
    FILE *fp = fopen("/proc/buddyinfo", "r");
    if (!fp) {
        perror("Cannot open /proc/buddyinfo");
        return -1;
    }

    char line[512];

    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "zone") == NULL) continue;

        char zone[32];
        uint32_t orders[11];

        int items = sscanf(line,
            "Node %*d, zone %31s %u %u %u %u %u %u %u %u %u %u %u",
            zone,
            &orders[0], &orders[1], &orders[2], &orders[3],
            &orders[4], &orders[5], &orders[6], &orders[7],
            &orders[8], &orders[9], &orders[10]);

        if (items >= 4) {
            snap->order2_free_pages = orders[2];
            snap->order3_free_pages = orders[3];
        }
    }

    fclose(fp);
    return 0;
}

// COMPLETELY REWRITTEN: Better JVM metaspace parser
int get_jvm_metaspace(pid_t pid, snapshot_t *snap) {
    char cmd[512];
    FILE *pipe;

    // Get the Both: line and clean up extra spaces
    snprintf(cmd, sizeof(cmd),
             "jcmd %d VM.metaspace 2>/dev/null | grep 'Both:' | sed 's/  */ /g'",
             pid);

    pipe = popen(cmd, "r");
    if (!pipe) return -1;

    char line[1024];

    if (fgets(line, sizeof(line), pipe)) {
        if (debug_mode) {
            fprintf(stderr, "DEBUG: Cleaned line: %s", line);
        }

        // After cleaning: "Both: 2422 chunks, 40.63 MB capacity, 40.20 MB ( 99%) committed, 39.67 MB ( 98%) used, ..."
        // Extract all float+MB patterns

        float values[10];
        int value_count = 0;
        char *ptr = line;

        while (*ptr && value_count < 10) {
            // Skip non-digit characters
            while (*ptr && (*ptr < '0' || *ptr > '9')) ptr++;
            if (!*ptr) break;

            // Try to read a float
            float val;
            int chars_read;
            if (sscanf(ptr, "%f%n", &val, &chars_read) == 1) {
                ptr += chars_read;

                // Check if followed by "MB" (with optional spaces)
                while (*ptr == ' ' || *ptr == '\t') ptr++;
                if (ptr[0] == 'M' && ptr[1] == 'B') {
                    values[value_count++] = val;
                    if (debug_mode) {
                        fprintf(stderr, "DEBUG: Found MB value[%d]: %.2f\n", value_count-1, val);
                    }
                    ptr += 2;
                }
            } else {
                ptr++;
            }
        }

        // The pattern is: capacity MB, committed MB, used MB
        // So we want values[1] (committed) and values[2] (used)
        if (value_count >= 3) {
            snap->metaspace_committed_kb = (uint64_t)(values[1] * 1024);
            snap->metaspace_used_kb = (uint64_t)(values[2] * 1024);

            if (debug_mode) {
                fprintf(stderr, "DEBUG: Final - Committed=%.2f MB (%lu KB), Used=%.2f MB (%lu KB)\n",
                        values[1], snap->metaspace_committed_kb,
                        values[2], snap->metaspace_used_kb);
            }

            pclose(pipe);
            return 0;
        }
    }

    pclose(pipe);
    return -1;
}


double calculate_fragmentation_index(snapshot_t *snap) {
    double weighted_sum = 0.0;
    double total_free = 0.0;

    weighted_sum += snap->order2_free_pages * 2;
    weighted_sum += snap->order3_free_pages * 3;

    total_free = snap->order2_free_pages + snap->order3_free_pages;

    if (total_free == 0) return 1.0;

    return 1.0 - (weighted_sum / (total_free * 3.0));
}

double calculate_mean(double *data, size_t n) {
    if (n == 0) return 0.0;
    double sum = 0.0;
    for (size_t i = 0; i < n; i++) sum += data[i];
    return sum / n;
}

double calculate_stddev(double *data, size_t n) {
    if (n == 0) return 0.0;
    double mean = calculate_mean(data, n);
    double variance = 0.0;
    for (size_t i = 0; i < n; i++) {
        double diff = data[i] - mean;
        variance += diff * diff;
    }
    return sqrt(variance / n);
}

double pearson_correlation(double *x, double *y, size_t n) {
    if (n < 2) return 0.0;

    double mean_x = calculate_mean(x, n);
    double mean_y = calculate_mean(y, n);

    double numerator = 0.0;
    double sum_sq_x = 0.0;
    double sum_sq_y = 0.0;

    for (size_t i = 0; i < n; i++) {
        double dx = x[i] - mean_x;
        double dy = y[i] - mean_y;
        numerator += dx * dy;
        sum_sq_x += dx * dx;
        sum_sq_y += dy * dy;
    }

    double denominator = sqrt(sum_sq_x * sum_sq_y);
    return (denominator == 0.0) ? 0.0 : (numerator / denominator);
}

correlation_result_t analyze_correlation(snapshot_list_t *list) {
    size_t n = list->count;
    correlation_result_t result = {0};

    if (n < 2) return result;

    double *jvm_metaspace = malloc(n * sizeof(double));
    double *kernel_slabs = malloc(n * sizeof(double));
    double *slab_scan_rates = malloc(n * sizeof(double));

    if (!jvm_metaspace || !kernel_slabs || !slab_scan_rates) {
        free(jvm_metaspace);
        free(kernel_slabs);
        free(slab_scan_rates);
        return result;
    }

    snapshot_t *snap = list->head;
    for (size_t i = 0; i < n; i++) {
        jvm_metaspace[i] = snap->metaspace_used_kb;
        kernel_slabs[i] = snap->kmalloc_1k_active + snap->kmalloc_4k_active;
        slab_scan_rates[i] = snap->slabs_scanned_per_sec;
        snap = snap->next;
    }

    result.correlation = pearson_correlation(jvm_metaspace, kernel_slabs, n);
    double mean = calculate_mean(slab_scan_rates, n);
    double stddev = calculate_stddev(slab_scan_rates, n);
    result.coefficient_var = (mean != 0.0) ? (stddev / mean) : 0.0;
    result.mean_pressure = mean;

    free(jvm_metaspace);
    free(kernel_slabs);
    free(slab_scan_rates);

    return result;
}

void display_live_stats(snapshot_t *snap) {
    printf("[%zu] Metaspace: %lu KB | Slabs/sec: %.2f | 1K: %u | 4K: %u | Frag: %.3f\n",
           snap->timestamp_sec,
           snap->metaspace_used_kb,
           snap->slabs_scanned_per_sec,
           snap->kmalloc_1k_active,
           snap->kmalloc_4k_active,
           snap->fragmentation_index);
}

void export_csv(snapshot_list_t *list, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Cannot create CSV file");
        return;
    }

    fprintf(fp, "timestamp,metaspace_kb,slabs_scanned_per_sec,"
                "kmalloc_1k,kmalloc_4k,fragmentation_index\n");

    snapshot_t *snap = list->head;
    while (snap) {
        fprintf(fp, "%lu,%lu,%.4f,%u,%u,%.6f\n",
                snap->timestamp_sec,
                snap->metaspace_used_kb,
                snap->slabs_scanned_per_sec,
                snap->kmalloc_1k_active,
                snap->kmalloc_4k_active,
                snap->fragmentation_index);
        snap = snap->next;
    }

    fclose(fp);
    printf("\nData exported to %s\n", filename);
}

void generate_report(snapshot_list_t *list) {
    printf("\n\n=== SLABSIGHT ANALYSIS REPORT ===\n\n");
    printf("Total samples: %zu\n", list->count);

    if (list->count > 0 && list->head && list->tail) {
        printf("Duration: %lu seconds\n\n",
               list->tail->timestamp_sec - list->head->timestamp_sec);
    }

    if (list->count < 2) {
        printf("Not enough samples for analysis.\n");
        return;
    }

    correlation_result_t corr = analyze_correlation(list);

    printf("--- Correlation Analysis ---\n");
    printf("JVM-Kernel Correlation: %.4f ", corr.correlation);
    if (corr.correlation > 0.7) printf("(STRONG - Reflection impacts kernel)\n");
    else if (corr.correlation > 0.4) printf("(MODERATE)\n");
    else printf("(WEAK)\n");

    printf("\n--- Memory Pattern ---\n");
    printf("Coefficient of Variation: %.4f ", corr.coefficient_var);
    if (corr.coefficient_var > 0.5) printf("(ERRATIC - Reflection causes instability)\n");
    else if (corr.coefficient_var > 0.2) printf("(MODERATE variability)\n");
    else printf("(STABLE pattern)\n");

    printf("\n--- Kernel Pressure ---\n");
    printf("Average slabs scanned/sec: %.2f\n", corr.mean_pressure);
    printf("\n=================================\n");
}

void cleanup_list(snapshot_list_t *list) {
    snapshot_t *current = list->head;
    while (current) {
        snapshot_t *next = current->next;
        free(current);
        current = next;
    }
    list->head = list->tail = NULL;
    list->count = 0;
}

void collection_loop(pid_t jvm_pid, int interval_sec) {
    snapshot_list_t list = {NULL, NULL, 0};

    printf("SlabSight - Kernel-Level JVM Memory Analyzer\n");
    printf("Target PID: %d | Interval: %ds", jvm_pid, interval_sec);
    if (debug_mode) printf(" | DEBUG MODE");
    printf("\n\nPress Ctrl+C to stop and generate report...\n\n");

    while (running) {
        snapshot_t *snap = calloc(1, sizeof(snapshot_t));
        if (!snap) {
            fprintf(stderr, "Memory allocation failed\n");
            break;
        }

        snap->timestamp_sec = time(NULL);

        parse_slabinfo(snap);
        parse_vmstat(snap);
        parse_buddyinfo(snap);
        get_jvm_metaspace(jvm_pid, snap);

        if (list.tail != NULL) {
            uint64_t dt = snap->timestamp_sec - list.tail->timestamp_sec;

            if (dt > 0) {
                uint64_t delta_scanned = snap->slabs_scanned - list.tail->slabs_scanned;
                snap->slabs_scanned_per_sec = (double)delta_scanned / dt;

                uint64_t delta_alloc = snap->pgalloc_dma - list.tail->pgalloc_dma;
                snap->allocation_rate_kb_per_sec = (double)(delta_alloc * 4) / dt;
            }

            snap->fragmentation_index = calculate_fragmentation_index(snap);
        }

        if (list.tail == NULL) {
            list.head = list.tail = snap;
        } else {
            list.tail->next = snap;
            list.tail = snap;
        }
        list.count++;

        display_live_stats(snap);
        sleep(interval_sec);
    }

    generate_report(&list);
    export_csv(&list, "slabsight_data.csv");
    cleanup_list(&list);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <jvm-pid> [interval-seconds] [--debug]\n", argv[0]);
        fprintf(stderr, "Example: %s 12345 5\n", argv[0]);
        fprintf(stderr, "         %s 12345 2 --debug\n", argv[0]);
        return 1;
    }

    pid_t jvm_pid = atoi(argv[1]);
    int interval = (argc >= 3) ? atoi(argv[2]) : 5;

    // Check for debug flag
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0) {
            debug_mode = 1;
        }
    }

    if (jvm_pid <= 0) {
        fprintf(stderr, "Invalid PID: %d\n", jvm_pid);
        return 1;
    }

    if (interval < 1) interval = 5;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    collection_loop(jvm_pid, interval);

    return 0;
}
