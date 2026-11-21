# libmr

Framework MapReduce in C per l'analisi di file di testo su singolo calcolatore.
Progetto di Laboratorio 2A, a.a. 2025-26.

`libmr` permette a un programma C di analizzare uno o più file di testo secondo
il paradigma MapReduce: una funzione **mapper** trasforma ogni riga di input in
coppie `<token, valore>`, il framework raggruppa i valori per token, e una
funzione **reducer** combina i valori di ogni gruppo in uno o più risultati
finali. Il framework si occupa internamente di creazione dei processi
(`fork`/`pipe`/`dup2`/`waitpid`), sincronizzazione dei thread C11,
comunicazione fra processi e produzione dell'output — l'utente scrive solo la
logica applicativa (mapper e reducer).

## Requisiti

- Linux (sviluppato e testato su Ubuntu 24.04)
- `gcc` con supporto C11
- `make`
- `valgrind` (opzionale, solo per `make test-valgrind`/`make test-valgrind-check`)

## Compilazione

```
make
```

Compila `libmr.a` in `build/`, insieme agli esempi (`wordcount`,
`line_length_stats`, `print_line_length_stats`).

```
make clean
```

Rimuove `build/` e i file temporanei (`*.log`, `*.mro`, `*.txt`) generati
nella directory corrente.

## Test

```
make test
```

Compila ed esegue la batteria di test in `tests/`: 17 test con assertion vere
(215 verifiche totali), che copre l'interfaccia pubblica, tutti i casi limite
indicati dalla specifica (file vuoti, righe vuote, ultima riga senza `\n`,
ecc.) e tutte le funzionalità dell'addendum (scansione ricorsiva, istanze
multiple, statistiche, hash personalizzata), oltre a due test di volume
(migliaia di token distinti / centomila coppie). Il target fallisce (exit code
diverso da 0) se una qualunque assertion non è soddisfatta.

```
make test-valgrind-check
```

Esegue la stessa batteria sotto `valgrind` (con `--trace-children=yes`, così
da tracciare anche i processi mapper e reducer generati via `fork()`), poi
verifica automaticamente che non ci siano errori né leak di memoria in nessun
processo. Se `valgrind` non è installato, il target viene saltato senza far
fallire la build. I report per processo restano in `build/valgrind_report_*.txt`
per un'ispezione manuale, se necessaria.

```
make test-valgrind
```

Solo la run sotto valgrind, senza il controllo automatico finale (utile per
ispezionare i report a mano).

## Utilizzo

```c
#include "mr.h"

int my_mapper(const mr_file_line_t *line, mr_emit_pair_t emit,
              void *emit_arg, void *user_arg) {
    /* estrae token dalla riga, chiama emit(token, value, value_size, emit_arg) */
}

int my_reducer(const char *token, const mr_value_t *values, size_t values_count,
                mr_emit_result_t emit, void *emit_arg, void *user_arg) {
    /* combina i valori, chiama emit(token, result, result_size, emit_arg) */
}

int main(void) {
    mr_t mr;
    mr_attr_t attr;

    mr_attr_init(&attr);
    mr_attr_set_mapper_threads(&attr, 4);
    mr_attr_set_reducer_threads(&attr, 4);
    mr_attr_set_queue_size(&attr, 64);
    mr_attr_set_log_file(&attr, "mr.log");

    mr_create(&mr, &attr, my_mapper, my_reducer, NULL);
    mr_start(mr, "input", "output.mro");

    mr_destroy(mr);
    mr_attr_destroy(&attr);
    return 0;
}
```

`input` può essere un singolo file o una directory (scandita ricorsivamente).
`output.mro` è il file binario prodotto (formato descritto sotto).

Tutte le funzioni ritornano `0` in caso di successo e `-1` in caso di errore,
impostando `errno`.

## Formato del file di output (.mro)

`mr_start` scrive un record binario per ogni risultato emesso dal reducer,
concatenati senza separatori, in ordine lessicografico crescente per token:

```
[size_t token_len][token, token_len byte][size_t result_len][result, result_len byte]
```

`token_len` non include il terminatore. Il framework conosce solo il formato
generico (le lunghezze): non interpreta mai il contenuto di `result`, che
resta un dato opaco dipendente da come il proprio reducer lo ha costruito.
Per leggere un file `.mro` serve quindi conoscere, oltre al formato sopra,
anche cosa rappresenta `result` per il reducer specifico che lo ha prodotto —
si veda `examples/print_line_length_stats.c` per un lettore completo di
riferimento (per l'output di `line_length_stats.c`).

## Esempi

- **`examples/wordcount.c`**: conta le occorrenze di ogni token. L'esempio
  base.
- **`examples/line_length_stats.c`**: per ogni token calcola la lunghezza
  minima e massima delle righe in cui compare, emettendo due risultati per
  token. Dimostra value/result binari  e l'emissione multipla per
  lo stesso token.
- **`examples/print_line_length_stats.c`**: programma ausiliario che
  stampa in forma leggibile l'output di `line_length_stats`.

Esecuzione:
```
build/wordcount <input> <output.mro>
build/line_length_stats <input> <output.mro>
build/print_line_length_stats <output.mro>
```

## Funzionalità aggiuntive (addendum)

- **Scansione ricorsiva delle sottodirectory** di input, in ordine
  lessicografico globale sui path completi.
- **Istanze multiple indipendenti** di `mr_t` nello stesso processo
  chiamante, senza stato globale né interferenze reciproche.
- **Statistiche finali** su file separato (`mr_attr_set_stats_file`):
  tempo di esecuzione, righe lette, coppie prodotte, token distinti,
  risultati emessi.
- **Funzione di hashing configurabile** (`mr_attr_set_hash_function`) per
  instradare i token ai thread reducer interni; se non specificata, viene
  usata una funzione di default (djb2).

## Struttura del progetto

```
include/
  mr.h                              interfaccia pubblica

src/
  mr.c                              mr_create / mr_start / mr_destroy
  mr_internal.h                     struct mr e dichiarazioni interne
  mr_log.c / mr_log.h               log su file, sincronizzato fra processi
                                     con semafori POSIX nominati
  mapper_process.c                  processo mapper multithread
  reducer_process.c                 processo reducer multithread

  utilities/
    mr_protocol_utilities.c/h            readn/writen, protocollo a lunghezze esplicite
    mr_queue_utilities.c/h               coda produttore-consumatore (mtx_t/cnd_t)
    mr_path_list_utilities.c/h           scansione input, lettura righe, invio al mapper
    mr_token_value_list_utilities.c/h    raggruppamento coppie per token (reducer)
    mr_token_result_list_utilities.c/h   accumulo/ordinamento risultati (reducer)
    mr_stats_utilities.c/h               scrittura file statistiche
    mr_attr_utilities.c/h                gestione mr_attr_t
    mr_hash_utilities.c/h                funzione di hash di default

examples/
  wordcount.c
  line_length_stats.c
  print_line_length_stats.c

tests/
  test_main.c                       punto di ingresso, esegue i 17 test in sequenza
  test_list.h                       dichiarazioni dei test
  test_codes/
    test1.c ... test17.c            un file per ciascun test
    test_utils.c/h                  parsing/confronto del formato .mro e delle statistiche
    test_mappers.c/h                coppie mapper/reducer dedicate ai singoli test

Makefile
README.md
```

## Log e statistiche

Ogni esecuzione produce un file di log (default `mr_log.txt`, configurabile)
con una riga per evento nel formato `[timestamp] [processo] [thread] [evento]
messaggio`. Il file di log è aperto in append: non viene mai sovrascritto fra
un'esecuzione e l'altra, accumula la cronologia di tutte le run. Se richiesto,
viene inoltre prodotto un file di statistiche testuale separato — a differenza
del log, questo viene sovrascritto a ogni esecuzione, poiché rappresenta una
fotografia della singola run appena conclusa, non una cronologia cumulativa.
Entrambi i percorsi sono configurabili tramite `mr_attr_t`.
