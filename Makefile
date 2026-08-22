# 1. Compiler & Flags

CC = gcc
CFLAGS = -Wall -ansi -pedantic 

# 2. Files & Directories
SRC_DIR = src
TARGET = sudoku

# Βρίσκω τα .c αρχεία στον φάκελο src/ (sudoku.c, grid.c)
# και τα περνάω σε μια λίστα. (SRCS)
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Δημιουργώ μια λίστα με τα επιθυμητά .o διαβάζοντας τα αντίστοιχα .c.
# Η OBJS είναι μια λίστα με "lebels" των object file. (sudoku.o, grid.o)
# Δεν τα έχω δημιουργήσει ακόμα.
OBJS = $(SRCS:.c=.o)


# Rules:


# Default target (when just 'make') - the what I am trying to do and let the dependencies do their job.
all: $(TARGET)

# [LINKING STEP]
# Ενώνω τα object files (.o) για να φτιάξω το τελικό εκτελέσιμο
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)
	@echo "Build successful! Executable: $(TARGET)"

# [PREPROCESS, COMPILE, ASSEMBLE STEP]
# Μεταφράζω κάθε .c αρχείο στο αντίστοιχο .o object file.
# Το flag '-c' λέει στον gcc να σταματήσει πριν το Linking.
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# [CLEAN TARGET]
# Σβήνω το εκτελέσιμο και τα object files, αφήνοντας μόνο τον πηγαίο κώδικα
clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Cleaned generated files."

# Δηλώνω ότι τα 'all' και 'clean' δεν είναι πραγματικά αρχεία - οπότε stick to the plan as-is
.PHONY: all clean
