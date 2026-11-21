#include <stdio.h>
#include "test_utils.h"
#include "test_mappers.h"

void test1_basic(void) {
    printf("\n=== Test 1: conteggio base, ordine lessicografico, statistiche complete ===\n");
    write_base_input("build/it_input_1");
    run_instance(it_mapper, it_reducer,
                 "build/it_input_1", "build/it_output_1.mro",
                 "build/logs/test1.log.txt", "build/stats/test1.stats.txt");

    long count;
    int found;

    found = find_token_count("build/it_output_1.mro", "the", &count);
    CHECK(found == 1 && count == 5, "conteggio di 'the' e' 5");

    found = find_token_count("build/it_output_1.mro", "fox", &count);
    CHECK(found == 1 && count == 3, "conteggio di 'fox' e' 3");

    found = find_token_count("build/it_output_1.mro", "quick", &count);
    CHECK(found == 1 && count == 1, "conteggio di 'quick' e' 1");

    found = find_token_count("build/it_output_1.mro", "nonexistent_token_xyz", &count);
    CHECK(found == 0, "token inesistente non compare nell'output");

    int order = check_lexicographic_order("build/it_output_1.mro");
    CHECK(order == 1, "i token compaiono in ordine lessicografico non decrescente");

    size_t v;
    int sf;

    sf = parse_stats_field("build/stats/test1.stats.txt", "Elapsed_seconds", &v);
    CHECK(sf == 1, "campo Elapsed_seconds presente nelle stats");

    sf = parse_stats_field("build/stats/test1.stats.txt", "Lines_read", &v);
    CHECK(sf == 1 && v == 2, "Lines_read e' 2 (2 file, 1 riga ciascuno)");

    sf = parse_stats_field("build/stats/test1.stats.txt", "Pairs_produced", &v);
    CHECK(sf == 1 && v == 18, "Pairs_produced e' 18 (12+6 parole totali)");

    sf = parse_stats_field("build/stats/test1.stats.txt", "Distinct_tokens", &v);
    CHECK(sf == 1 && v == 11, "Distinct_tokens e' 11");

    sf = parse_stats_field("build/stats/test1.stats.txt", "Results_emitted", &v);
    long records = count_records("build/it_output_1.mro");
    CHECK(sf == 1 && v == 11 && (long)v == records, "Results_emitted e' 11 e combacia coi record scritti");
}
