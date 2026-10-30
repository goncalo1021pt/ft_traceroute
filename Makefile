NAME = ft_traceroute
NAME_ASAN = $(NAME)_asan

SRCS_DIR = srcs/
OBJS_DIR = objs/
OBJS_DIR_ASAN = objs_asan/
SRCS = $(wildcard $(SRCS_DIR)*.c)
OBJS = $(patsubst $(SRCS_DIR)%.c,$(OBJS_DIR)%.o,$(SRCS))
OBJS_ASAN = $(patsubst $(SRCS_DIR)%.c,$(OBJS_DIR_ASAN)%.o,$(SRCS))
HEADERS = -I includes/headers/
HDRS = $(wildcard includes/headers/*.h)

# unit tests: each tests/unit/test_*.c is its own program, linked with every source except main.c
UNIT_SRCS = $(wildcard tests/unit/*.c)
UNIT_TESTS = $(patsubst %.c,%,$(wildcard tests/unit/test_*.c))
UNIT_OBJS = $(UNIT_SRCS:.c=.o)
APP_OBJS = $(filter-out $(OBJS_DIR)main.o,$(OBJS))

RM = rm -f
CFLAGS = -Wall -Wextra -Werror -g
SFLAGS = -fsanitize=address,undefined -fno-omit-frame-pointer
WFLAGS = -Wshadow -Wconversion -Wpedantic -Wstrict-prototypes
VFLAGS = --leak-check=full --show-leak-kinds=all --track-origins=yes --track-fds=yes
CC = cc

# libft
LIBFT_DIR = ./includes/libft
LIBFT = $(LIBFT_DIR)/libft.a

# tooling
FORMAT_FILES = $(SRCS) $(HDRS) $(UNIT_SRCS) $(wildcard tests/unit/*.h)

#color codes
GREEN = \033[0;32m
RED = \033[0;31m
BLUE = \033[0;34m
ORANGE = \033[0;33m
NC = \033[0m

all: $(NAME)

$(OBJS_DIR)%.o: $(SRCS_DIR)%.c $(HDRS)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(HEADERS) -c $< -o $@

$(OBJS_DIR_ASAN)%.o: $(SRCS_DIR)%.c $(HDRS)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(SFLAGS) $(HEADERS) -c $< -o $@

$(NAME): $(OBJS) $(LIBFT)
	@echo "$(GREEN)$(NAME)$(NC) compiling..."
	@$(CC) $(CFLAGS) -o $(NAME) $(OBJS) $(LIBFT) $(HEADERS)
	@echo "$(GREEN)$(NAME)$(NC) ready!"

$(LIBFT):
	@echo "$(ORANGE)libft$(NC) compiling..."
	@$(MAKE) -C $(LIBFT_DIR) --no-print-directory
	@echo "$(ORANGE)libft$(NC) ready!"

# AddressSanitizer + UBSan build, kept apart from the normal binary
asan: $(OBJS_ASAN) $(LIBFT)
	@echo "$(GREEN)$(NAME_ASAN)$(NC) compiling with $(ORANGE)$(SFLAGS)$(NC)..."
	@$(CC) $(CFLAGS) $(SFLAGS) -o $(NAME_ASAN) $(OBJS_ASAN) $(LIBFT)

# extra-strict warnings, compile only (nothing is linked)
warnings:
	@for f in $(SRCS); do \
		$(CC) $(CFLAGS) $(WFLAGS) $(HEADERS) -fsyntax-only $$f || exit 1; \
	done
	@echo "$(GREEN)warnings$(NC) clean!"

# unit tests
tests/unit/%.o: tests/unit/%.c $(HDRS) $(wildcard tests/unit/*.h)
	@$(CC) $(CFLAGS) $(HEADERS) -I tests/unit -c $< -o $@

tests/unit/test_%: tests/unit/test_%.o $(APP_OBJS) $(LIBFT)
	@$(CC) $(CFLAGS) -o $@ $< $(APP_OBJS) $(LIBFT)

unit: $(UNIT_TESTS)
	@for t in $(UNIT_TESTS); do ./$$t || exit 1; done

.SECONDARY: $(UNIT_OBJS)

# the same unit tests built with AddressSanitizer + UBSan (catches out-of-bounds reads)
unit-asan: $(LIBFT)
	@for t in $(UNIT_TESTS); do \
		$(CC) $(CFLAGS) $(SFLAGS) $(HEADERS) -I tests/unit -o $$t.asan $$t.c \
			$(filter-out $(SRCS_DIR)main.c,$(SRCS)) $(LIBFT) && ./$$t.asan || exit 1; \
	done; $(RM) tests/unit/*.asan

# command-line tests, none of them need root
cli: $(NAME)
	@sh tests/cli.sh ./$(NAME)

# real traces against 127.0.0.1, raw sockets need root: sudo make root-tests
root-tests: all
	@sh tests/root.sh ./$(NAME)

root-tests-asan: asan
	@sh tests/root.sh ./$(NAME_ASAN)

test: unit cli

format:
	clang-format -i $(FORMAT_FILES)

format-check:
	clang-format --dry-run --Werror $(FORMAT_FILES)

lint:
	cppcheck --enable=warning,style,performance --inline-suppr --error-exitcode=1 \
		--suppress=missingIncludeSystem $(HEADERS) $(SRCS_DIR)

clean:
	@$(RM) -r $(OBJS_DIR)
	@$(RM) -r $(OBJS_DIR_ASAN)
	@$(RM) $(UNIT_OBJS) $(UNIT_TESTS)
	@$(MAKE) -C $(LIBFT_DIR) clean --no-print-directory
	@echo "$(RED)$(NAME)$(NC) OBJS cleaned!"

fclean: clean
	@$(RM) $(NAME) $(NAME_ASAN)
	@$(MAKE) -C $(LIBFT_DIR) fclean --no-print-directory
	@echo "$(RED)$(NAME)$(NC) cleaned!"

v: re
	sudo valgrind $(VFLAGS) ./$(NAME) 127.0.0.1

fcount:
	@echo "you wrote $(RED)$(shell cat $(SRCS) | wc -l)$(NC)lines of code"

run: all
	./$(NAME)

re: fclean all

.PHONY: all asan warnings unit unit-asan cli root-tests root-tests-asan test format format-check lint \
	clean fclean v fcount run re
