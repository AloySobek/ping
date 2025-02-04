CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -g
LDFLAGS = -lm

SRC = src/main.c src/config.c src/list.c src/socket.c src/print.c
OBJ = $(SRC:.c=.o)

NAME = ft_ping


all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME) $(LDFLAGS)

%.o: %.c

	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean

	rm -f $(NAME)


re: fclean all


.PHONY: all clean fclean re
