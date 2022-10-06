NAME = ft_ping

SRCS =	main.c \
		config.c \
		show_help.c \
		ft_ping.c \
		validators.c \
		utils.c \
		print.c

SRCS_DIR = srcs

OBJS_DIR = .objs

INCLUDE_DIR = includes

LIBS = libft/ft

MAKE_DEP = ./libft/libft.a

CFLAGS = -Wall -Werror -Wextra -g3

include makefile-template/template.mk