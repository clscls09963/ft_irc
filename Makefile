NAME = ircserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
INC = -Iinclude/

SRC_DIR = src/

SRC_NAME = main.cpp\
			Channel.cpp\
			Client.cpp\
			Server.cpp\
			Command.cpp\
			Util.cpp

SRCS = $(addprefix $(SRC_DIR), $(SRC_NAME))

OBJS = $(SRCS:.cpp=.o)

.cpp.o :
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@


all : $(NAME)

$(NAME) : $(OBJS)
		$(CXX) $(CXXFLAGS) $(INC) $(OBJS) -o $(NAME)

clean :
		rm -rf $(OBJS)

fclean :clean
		rm -rf $(NAME)

re : fclean all

.PHONY : all clean fclean re
