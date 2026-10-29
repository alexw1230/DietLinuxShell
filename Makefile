BIN_DIR := bin
SOURCES := shell.c part1_shell.c part2_shell.c part3_shell.c bonus_shell.c
TARGETS := $(SOURCES:.c=)
BIN_TARGETS := $(addprefix $(BIN_DIR)/,$(TARGETS))

all: $(BIN_DIR) $(BIN_TARGETS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/shell: shell.c | $(BIN_DIR)
	gcc -o $@ $<

$(BIN_DIR)/part1_shell: part1_shell.c | $(BIN_DIR)
	gcc -o $@ $<

$(BIN_DIR)/part2_shell: part2_shell.c | $(BIN_DIR)
	gcc -o $@ $<

$(BIN_DIR)/part3_shell: part3_shell.c | $(BIN_DIR)
	gcc -o $@ $<

$(BIN_DIR)/bonus_shell: bonus_shell.c | $(BIN_DIR)
	gcc -o $@ $<

run_shell: $(BIN_DIR)/shell
	./$(BIN_DIR)/shell

run1: $(BIN_DIR)/part1_shell
	./$(BIN_DIR)/part1_shell

run2: $(BIN_DIR)/part2_shell
	./$(BIN_DIR)/part2_shell

run3: $(BIN_DIR)/part3_shell
	./$(BIN_DIR)/part3_shell

run_bonus: $(BIN_DIR)/bonus_shell
	./$(BIN_DIR)/bonus_shell

clean:
	rm -rf $(BIN_DIR)
	
help:
	@echo "Makefile targets:"
	@echo "  all          - Build all shell programs"
	@echo "  clean        - Remove the bin directory and all compiled programs"
	@echo "  run_shell    - Run the basic shell program"
	@echo "  run1         - Run the part1 shell program"
	@echo "  run2         - Run the part2 shell program"
	@echo "  run3         - Run the part3 shell program"
	@echo "  run_bonus    - Run the bonus shell program"
	@echo "  help         - Display this help message"

.PHONY: all clean run_shell run0 run1 run3 run_bonus
