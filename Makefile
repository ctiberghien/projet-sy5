CC := gcc
# Options de compilation
CFLAGS := -L/usr/include -O -Wall -Wextra -Werror

# Dossier source
SRCDIR := src
# Dossier objet
OBJDIR := obj
#Librairie utiliser
LIBS := -lreadline

# Liste de tous les fichiers source dans le dossier src
SRC := $(wildcard $(SRCDIR)/*.c)

# Générer la liste des fichiers objet en remplaçant .c par .o
OBJ := $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRC))

# Nom de l'exécutable
TARGET := jsh

# Règle par défaut
all: $(TARGET)

# Règle de compilation pour chaque fichier objet avec la creation du dossier obj
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

# Règle de création de l'exécutable
$(TARGET): $(OBJ)
	$(CC) $^ -o $@ $(LIBS)

# Nettoyer les fichiers générés
clean:
	rm -rf $(OBJDIR) $(TARGET)

# Ignorer la création de fichiers avec ces noms
.PHONY: all clean
