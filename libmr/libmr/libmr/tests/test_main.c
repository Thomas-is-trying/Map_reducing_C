#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include "test_utils.h"
#include "test_list.h"

int main(void) {
    system("mkdir -p build/logs build/stats");

    test1_basic();
    test2_multi_instance();
    test3_edge_cases();
    test4_param_validation();
    test5_determinism();
    test6_opaque_nulls();
    test7_single_file_input();
    test8_recursive_scan();
    test9_zero_size();
    test10_variable_size();
    test11_zero_results();
    test12_custom_hash();
    test13_line_numbering();
    test14_tiny_queue();
    test15_log_content();
    test16_many_distinct_tokens();
    test17_large_repeated();

    printf("\n=== Risultato finale: %d fallimenti ===\n", g_failures);
    if (g_failures == 0) {
        printf("Log consultabili in build/logs/, statistiche in build/stats/\n");
    }
    return g_failures == 0 ? 0 : 1;
}
