#include <stdio.h>
#include "test_utils.h"

void test15_log_content(void) {
    printf("\n=== Test 15: contenuto del file di log (eventi richiesti da sez. 11) ===\n");
    const char *log_path = "build/logs/test1.log.txt";

    const char *events[] = {
        "LOG_INIT", "PIPES_CREATED", "PROCESS_CREATED", "THREAD_START",
        "THREAD_END", "LINES_SENT", "PAIRS_PRODUCED", "DISTINCT_TOKENS",
        "RESULTS_EMITTED", "OUTPUT_WRITTEN", "PROCESS_END","INPUT_CLOSED", "OUTPUT_OPEN", "OUTPUT_CLOSED", NULL
    };
    for (int i = 0; events[i] != NULL; i++) {
        char msg[128];
        snprintf(msg, sizeof(msg), "evento '%s' presente nel file di log", events[i]);
        int c = file_contains_string(log_path, events[i]);
        CHECK(c == 1, msg);
    }

    const char *tags[] = { "[main]", "[mapper]", "[reducer]", "[reader]", "[worker]", NULL };
    for (int i = 0; tags[i] != NULL; i++) {
        char msg[128];
        snprintf(msg, sizeof(msg), "tag '%s' presente nel file di log", tags[i]);
        int c = file_contains_string(log_path, tags[i]);
        CHECK(c == 1, msg);
    }
}
