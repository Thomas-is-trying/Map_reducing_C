#include <stdio.h>
#include <stdlib.h>
#include "test_utils.h"
#include "test_mappers.h"

void test10_variable_size(void) {
    printf("\n=== Test 10: value di dimensione variabile per lo stesso token ===\n");
    system("mkdir -p build/it_input_var");
    FILE *f = fopen("build/it_input_var/lines.txt", "w");
    fprintf(f, "a\nbb\nccc\n");
    fclose(f);

    run_instance(var_mapper, var_reducer,
                 "build/it_input_var", "build/it_output_var.mro",
                 "build/logs/test10.log.txt", "build/stats/test10.stats.txt");

    const char *parts[] = { "a", "bb", "ccc" };
    size_t expected_total = 0;
    unsigned char expected_xor = 0;
    for (int p = 0; p < 3; p++) {
        for (const char *c = parts[p]; *c; c++) {
            expected_total += 1;
            expected_xor ^= (unsigned char)*c;
        }
    }

    void *data;
    size_t len;
    int r = get_nth_matching_record("build/it_output_var.mro", "varsize", 0, &data, &len);
    CHECK(r == 1 && len == sizeof(value_check_t), "record 'varsize' presente con dimensione attesa");
    if (r == 1 && data != NULL) {
        value_check_t *vc = (value_check_t *)data;
        CHECK(vc->total_size == expected_total,
              "somma delle dimensioni dei value (variabili) e' corretta");
        CHECK(vc->xor_byte == expected_xor,
              "contenuto byte-per-byte dei value di dimensione variabile e' integro");
        free(data);
    }
}
