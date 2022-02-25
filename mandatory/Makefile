
.PHONY = all clean fclean re

NAME = ircserv

CC = clang++

GFLAGS = -Wall -Wextra -Werror 

HEAD = Server.hpp Client.hpp Channel.hpp  include.hpp
 
SRCS = main.cpp  Server.cpp Client.cpp Channel.cpp Command.cpp

OBJS =$(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(GFLAGS) $(OBJS) -o $(NAME)

$(OBJS) : %.o : %.cpp $(HEAD)
	$(CC) $(GFLAGS) -c $< -o $@

clean :
	rm -f $(OBJS)

fclean : clean
	rm -f $(NAME)
	rm -f ircbot

re : fclean all