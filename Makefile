NAME = usb-see
CC = gcc

CFLAGS = -Iinclude -Wall -Wextra -Werror
DEBUGFLAGS = -g
OPTIMIZEFLAGS = -O3
LDFLAGS = -ludev

OBJDIR = obj
BINDIR = bin
SRCDIR = src

SRC = $(wildcard $(SRCDIR)/*.c)
OBJ = $(SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
DEBUGOBJ = $(SRC:$(SRCDIR)/%.c=$(OBJDIR)/debug_%.o)

# ====================================

all: CFLAGS += $(OPTIMIZEFLAGS)
all: $(BINDIR)/$(NAME)

debug: CFLAGS += $(DEBUGFLAGS)
debug: $(BINDIR)/$(NAME)-debug

install-system: all
	install -m 755 $(BINDIR)/$(NAME) /usr/local/bin/$(NAME)
install-user: all
	install -m 755 $(BINDIR)/$(NAME) $(HOME)/.local/bin/$(NAME)

uninstall-system:
	rm -f /usr/local/bin/$(NAME)
uninstall-user:
	rm -f $(HOME)/.local/bin/$(NAME)

# ====================================

$(BINDIR)/$(NAME): $(OBJ)
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(BINDIR)/$(NAME)-debug: $(DEBUGOBJ)
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/debug_%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: all debug clean
