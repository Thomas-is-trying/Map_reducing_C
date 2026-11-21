#ifndef TEST_LIST_H
#define TEST_LIST_H

//conteggio base + ordine lessicografico + tutte e 5 le stats
void test1_basic(void);
//due istanze mr_t nello stesso processo, verifica non-interferenza
void test2_multi_instance(void);
//casi limite sez. 4: file vuoto, righe vuote, riga singola, ultima riga senza '\n'
void test3_edge_cases(void);
//rifiuto di parametri non validi, con errno corretto
void test4_param_validation(void);
//determinismo: due esecuzioni identiche, output confrontato byte per byte
void test5_determinism(void);
//value/result con byte nulli interni, multi-emit per lo stesso token
void test6_opaque_nulls(void);
//input come singolo file regolare, non directory
void test7_single_file_input(void);
//scansione ricorsiva delle sottodirectory (addendum)
void test8_recursive_scan(void);
//value/result di dimensione 0 con puntatore NULL
void test9_zero_size(void);
//value di dimensione variabile per lo stesso token
void test10_variable_size(void);
//reducer che non emette alcun risultato per un token
void test11_zero_results(void);
//funzione di hash personalizzata, anche degenere (addendum)
void test12_custom_hash(void);
//numerazione delle righe corretta, nessuna perdita/duplicazione
void test13_line_numbering(void);
//code interne di capacità minima (queue_size=1), nessun deadlock
void test14_tiny_queue(void);
//verifica automatica del contenuto del file di log
void test15_log_content(void);
//molti token distinti, stress della ricerca lineare nel raggruppamento
void test16_many_distinct_tokens(void);
//file di grandi dimensioni con vocabolario ripetuto, stress di throughput
void test17_large_repeated(void);

#endif