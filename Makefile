CC := gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -I$(SRCDIR)

SRCDIR := src
OBJDIR = $(BLDDIR)/obj
BLDDIR := build

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(addprefix $(OBJDIR)/, $(SRCS:.c=.o))
TARGET = $(BLDDIR)/cinvader

DEPS = $(OBJS:.o=.d)

.PHONY: all debug format clean

.PRECIOUS: $(OBJDIR)/%.o

all: format $(TARGET)

debug: CFLAGS += -g -O0
debug: $(TARGET)

-include $(DEPS)

format:
	@clang-format -i ./src/*

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -MMD -MP $< -o $@

clean:
	$(RM) -r $(BLDDIR)
