# Project name
NAME	= webserver

# Directories
SRC_DIR	= src/
INC_DIR	= include/

# Source files
SRC		= main.cpp \
			$(SRC_DIR)WebServer.cpp $(SRC_DIR)ClientConnection.cpp \
			$(SRC_DIR)response/Response.cpp \
			$(SRC_DIR)error/Error.cpp $(SRC_DIR)error/BadRequest.cpp $(SRC_DIR)error/NotFound.cpp $(SRC_DIR)error/NotImplemented.cpp \
			$(SRC_DIR)error/MethodNotAllowed.cpp $(SRC_DIR)error/InternalServerError.cpp $(SRC_DIR)error/ErrorHandler.cpp \
			$(SRC_DIR)request/HttpException.cpp $(SRC_DIR)request/HttpRequest.cpp $(SRC_DIR)request/HttpRequestBuilder.cpp $(SRC_DIR)request/RequestHandler.cpp $(SRC_DIR)request/Get.cpp $(SRC_DIR)request/Post.cpp \
			$(SRC_DIR)config/Block.cpp        $(SRC_DIR)config/Directive.cpp    $(SRC_DIR)config/ServerConfig.cpp $(SRC_DIR)config/ConfigParser.cpp $(SRC_DIR)config/Location.cpp


# Objects
OBJ		= $(SRC:.cpp=.o)

# Compiler settings
CXX		= c++
CFLAGS	= -Wall -Wextra -std=c++98 -g3 -I$(INC_DIR)
LDFLAGS	= -pthread
RM		= rm -rf

# Rules
all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CFLAGS) $(LDFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJ)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re
