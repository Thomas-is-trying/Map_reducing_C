# ============================================================
# Attrezzi del mestiere: compilatore, flag, archiviatore
# ============================================================
CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -pthread -g -Iinclude
AR := ar
ARFLAGS := rcs

# ============================================================
# Directory del progetto
# ============================================================
SRC_DIR := src
UTL_DIR := src/utilities
EXAMPLES_DIR := examples
TESTS_DIR := tests
CODES_DIR := tests/tests_codes
BUILD_DIR := build

# VPATH: dice a make dove cercare i sorgenti .c quando le regole
# pattern (es. %.o) non li trovano nella directory corrente.
VPATH := $(SRC_DIR):$(UTL_DIR)

# ============================================================
# Elenco sorgenti / oggetti della libreria
# ============================================================

# Tutti i .c del framework: src/ e src/utilities/
LIB_SRCS := $(wildcard $(SRC_DIR)/*.c) $(wildcard $(UTL_DIR)/*.c)

# Ogni .c diventa un .o dentro build/, indipendentemente
# da dove si trovava il sorgente originale.
LIB_OBJS := $(addprefix $(BUILD_DIR)/,$(notdir $(LIB_SRCS:.c=.o)))

LIBMR := $(BUILD_DIR)/libmr.a

# Sorgenti dei test: tests/ e tests/tests_codes/
TEST_SRCS := $(wildcard $(TESTS_DIR)/*.c) $(wildcard $(CODES_DIR)/*.c)

# ============================================================
# Target fittizi (non producono un file con questo nome)
# ============================================================
.PHONY: all clean test test-valgrind test-valgrind-check

# Target di default: costruisce libreria + tutti gli esempi
all: $(LIBMR) $(BUILD_DIR)/wordcount $(BUILD_DIR)/line_length_stats $(BUILD_DIR)/print_line_length_stats 

# Crea la cartella build/ se non esiste
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ============================================================
# Regola pattern: compila un qualsiasi .c in build/*.o
# (VPATH si occupa di trovare il .c anche in sottocartelle)
# ============================================================
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Impacchetta tutti gli oggetti nella libreria statica libmr.a
$(LIBMR): $(LIB_OBJS)
	$(AR) $(ARFLAGS) $@ $^

# ============================================================
# Programmi di esempio (collegati contro libmr.a)
# ============================================================
$(BUILD_DIR)/wordcount: $(EXAMPLES_DIR)/wordcount.c $(LIBMR)
	$(CC) $(CFLAGS) $< -L$(BUILD_DIR) -lmr -o $@

$(BUILD_DIR)/line_length_stats: $(EXAMPLES_DIR)/line_length_stats.c $(LIBMR)
	$(CC) $(CFLAGS) $< -L$(BUILD_DIR) -lmr -o $@

# Programma standalone: legge/stampa l'output .mro, non usa il framework
$(BUILD_DIR)/print_line_length_stats: $(EXAMPLES_DIR)/print_line_length_stats.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

# ============================================================
# Test di integrazione
# ============================================================
$(BUILD_DIR)/integration_test: $(TEST_SRCS) $(LIBMR)
	$(CC) $(CFLAGS) -I$(TESTS_DIR) -I$(CODES_DIR) $(TEST_SRCS) -L$(BUILD_DIR) -lmr -o $@

test: $(BUILD_DIR)/integration_test
	$(BUILD_DIR)/integration_test

# ============================================================
# Test con valgrind: controlla leak e file descriptor,
# tracciando anche i processi figli (fork di mapper/reducer)
# ============================================================
test-valgrind: $(BUILD_DIR)/integration_test
	@which valgrind > /dev/null 2>&1 || { echo "valgrind non installato, target saltato"; exit 0; }
	@rm -f $(BUILD_DIR)/valgrind_report_*.txt
	valgrind --trace-children=yes --leak-check=full --track-fds=yes \
		--error-exitcode=1 --log-file=$(BUILD_DIR)/valgrind_report_%p.txt \
		$(BUILD_DIR)/integration_test
	@echo "=== report per processo in $(BUILD_DIR)/valgrind_report_*.txt ==="

# Analizza i report valgrind e fallisce se trova errori o leak
test-valgrind-check: test-valgrind
	@echo "--- Verifica errori (esclusi processi puliti) ---"
	@grep -h "ERROR SUMMARY" $(BUILD_DIR)/valgrind_report_*.txt | grep -v "0 errors from 0 contexts" > $(BUILD_DIR)/.vg_errors.tmp; \
	if [ -s $(BUILD_DIR)/.vg_errors.tmp ]; then \
		cat $(BUILD_DIR)/.vg_errors.tmp; \
		echo "FAIL: valgrind ha trovato errori in almeno un processo"; \
		rm -f $(BUILD_DIR)/.vg_errors.tmp; \
		exit 1; \
	else \
		echo "OK: nessun errore in nessun processo tracciato"; \
		rm -f $(BUILD_DIR)/.vg_errors.tmp; \
	fi
	@echo "--- Verifica leak di memoria ---"
	@grep -h -E "definitely lost|indirectly lost" $(BUILD_DIR)/valgrind_report_*.txt | grep -v ": 0 bytes in 0 blocks" > $(BUILD_DIR)/.vg_leaks.tmp; \
	if [ -s $(BUILD_DIR)/.vg_leaks.tmp ]; then \
		cat $(BUILD_DIR)/.vg_leaks.tmp; \
		echo "FAIL: valgrind ha trovato leak di memoria"; \
		rm -f $(BUILD_DIR)/.vg_leaks.tmp; \
		exit 1; \
	else \
		echo "OK: nessun leak di memoria in nessun processo tracciato"; \
		rm -f $(BUILD_DIR)/.vg_leaks.tmp; \
	fi
	@echo "=== valgrind: tutto pulito ==="

# ============================================================
# Pulizia
# ============================================================
clean:
	rm -rf $(BUILD_DIR) *.log *.mro *.txt

