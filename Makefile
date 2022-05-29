NAME = ft_ping
SRCS =	main.c \
		config.c \
		show_help.c \
		ft_ping.c \
		validators.c \
		utils.c \
		print.c

INCLUDES = ft_ping.h

LIBS = libft/ft

MAKE_DEP = ./libft/libft.a

LIB_ARG = $(foreach path, $(LIBS), -L $(dir $(path)) -l $(notdir $(path)))

OBJS := $(SRCS:%.c=%.o)

CFLAGS = -Wall -Werror -Wextra -g3

CC = cc

all: $(NAME)

$(NAME): $(OBJS) $(MAKE_DEP)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIB_ARG)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(MAKE_DEP):
	make -C $(dir $@) $(notdir $@)

clean:
	rm $(OBJS)

fclean: clean
	rm $(NAME)

re: fclean all

.PHONY: clean all fclean re