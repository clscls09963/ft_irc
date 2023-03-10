#include <iostream>
#include <cstdlib>
#include "../include/Server.hpp"
#include <signal.h>

Server *server;

static int port_num(char *port)
{
	int i = 0;
	while(port[i])
	{
		if (isdigit(port[i]) == 0)
			return -1;
		i++;
	}
	return (0);
}

void	sigIntHandler(int num)
{
	if (num == SIGINT)
		delete server;
	exit(1);
}

void	sigQuitHandler(int num)
{
	if (num == SIGQUIT)
		delete server;
	exit(1);
}

int	start_server(char **argv)
{
	server = new Server(atoi(argv[1]), argv[2]);
	if (server->execute() < 0)
	{
		delete server;
		return 1;
	}
	return 0;
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cout << "Error: ./server <port> <password>\n";
        return 1;
    }
	if (port_num(argv[1]) == -1)
	{
		std::cout << "Error: port_number error\n";
		return 1;
	}
	
	signal(SIGINT, sigIntHandler);
	signal(SIGQUIT, sigQuitHandler);

	if (start_server(argv))
		return 1;

	delete server;
	return 0;
}
