NAME = ft_ping
SRCS =	main.c \
		config.c \

INCLUDES = ft_ping.h

LIBS = libft/ft

MAKE_DEP = ./libft/libft.a

LIB_ARG = $(foreach path, $(LIBS), -L $(dir $(path)) -l $(notdir $(path)))

OBJS := $(SRCS:%.c=%.o)

CFLAGS = -Wall -Werror -Wextra

CC = cc

all: $(NAME)

$(NAME): $(OBJS) $(MAKE_DEP)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIB_ARG)

$(OBJS):
	$(CC) $(CFLAGS) -c $(@:.o=.c) -o $@

$(MAKE_DEP):
	make -C $(dir $@) $(notdir $@)

clean:
	rm $(OBJS)

fclean: clean
	rm $(NAME)

re: fclean all

.PHONY: clean all fclean re