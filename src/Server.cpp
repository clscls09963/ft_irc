#include "../include/Server.hpp"
#include <fcntl.h>

int Server::pollingEvent(){
	_clientLen = sizeof(_clientAddr);
	_clientFd = accept(_serverSocketFd, (struct sockaddr *)&_clientAddr, &_clientLen);
	if (_clientFd < 0) {
		std::cerr << "Error: accepting client" << std::endl;
		exit(1);
	}
	int fcntlRet = fcntl(_clientFd, F_SETFL, O_NONBLOCK);
	if (fcntlRet == -1)
	{
		std::cerr << "fcntlRet" << std::endl;
		exit(1);
	}

	_clientList.insert(std::pair<int, Client *>(_clientFd, new Client(_clientFd)));
	std::cout <<"\n\n*Accept Client fd: " << _clientList.find(_clientFd)->first << "*" <<std::endl;
	int i;
	for (i = 1; i < OPEN_MAX; i++)
	{
		if (_pollClient[i].fd < 0)
		{
			_pollClient[i].fd = _clientFd;
			break;
		}
	}

	_pollClient[i].events = POLLIN;
	if (i > _maxClient)
		_maxClient = i;
	if (--_pollLet <= 0)
		return (-1);
	return (0);
}

Server::Server(int port, std::string password) : _command(this){
	_pollLet = 0;
	_maxClient = 0;
	_port = port;
	_password = password;
	sock_init();
	_pollClient[0].fd = _serverSocketFd;
	_pollClient[0].events = POLLIN;
	for (int i = 1; i < OPEN_MAX; i++){
		_pollClient[i].fd = -1;
	}
}

template <class T1, class T2>
void deleteMap(std::map<T1, T2> &map){
	typename std::map<T1, T2>::iterator it = map.begin();
	while (it != map.end())
	{
		delete it->second;
		it++;
	}
};

Server::~Server()
{
	std::cout << "server destructer called\n";
	std::map<int, Client *>::iterator it = _clientList.begin();
	for(; it != _clientList.end(); it++)
		close(it->first);
	deleteMap(_clientList);
	deleteMap(_channelList);
}

std::map<int, Client *> &Server::getClientList() 
{
	return _clientList;
}

std::string Server::getPass()
{
	return _password;
}

std::map<std::string, Channel *> &Server::getChannelList() 
{
	return _channelList;
}

Client* Server::findClient(int fd) 
{
	std::map<int, Client *>::iterator it = _clientList.find(fd);
	if (it != _clientList.end())
		return it->second;
	return NULL;
}

Client* Server::findClient(std::string nick) 
{
	std::map<int, Client *>::iterator it = _clientList.begin();
	for (; it != _clientList.end(); it++)
	{
		if (it->second->getNickName() == nick)
			return it->second;
	}
	return NULL;
};

Channel* Server::findChannel(std::string name) 
{
	std::map<std::string, Channel *>::iterator find_name = _channelList.find(name);
	
	if (find_name == _channelList.end())
		return NULL;
	return _channelList.find(name)->second;
}

int Server::sock_init()
{
	_serverSocketFd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	_serverSocketAddr.sin_family = AF_INET;
	_serverSocketAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	_serverSocketAddr.sin_port = htons(_port);
	for (int i = 0; i < 8; i++)
		_serverSocketAddr.sin_zero[i] = 0;

	int optval = true;
	socklen_t optlen = sizeof(optval);
	setsockopt(_serverSocketFd, SOL_SOCKET, SO_REUSEADDR, (void *)&optval, optlen);
	if (bind(_serverSocketFd, (const sockaddr *)&_serverSocketAddr, sizeof(_serverSocketAddr)) == -1)
	{
		std::cerr << "Error: binding socket" << std::endl;
		exit(1);
	}
	if (listen(_serverSocketFd, 15) == -1)
	{
		std::cerr << "Error: listening socket" << std::endl;
		exit(1);
	}
	std::cout << "listening" << std::endl;
	return (0);
}

void Server::check_cmd(std::string cmd, std::vector<std::string> cmd_vec, Client *client)
{
	if (cmd == "NICK" || cmd == "nick")
		_command.nick(cmd_vec, client);
	else if (cmd == "JOIN" || cmd == "join")
		_command.join(cmd_vec, client);
	else if (cmd == "KICK" || cmd == "kick")
		_command.kick(cmd_vec, client);
	else if (cmd == "PRIVMSG" || cmd == "privmsg")
		_command.privmsg(cmd_vec, client);
	else if (cmd == "NOTICE" || cmd == "notice")
		_command.notice(cmd_vec, client);
	else if (cmd == "PING" || cmd == "ping")
		_command.pong(cmd_vec, client);
	else if (cmd == "PART" || cmd == "part")
		_command.part(cmd_vec, client);
	else if (cmd == "QUIT" || cmd == "quit")
		_command.quit(cmd_vec, client);
	else if (cmd == "PASS" || cmd == "pass" || cmd == "USER" || cmd == "user")
		_command.alreadyRegist(client);
	else
		std::cout << cmd << "Error: undefined cmd\n\n";
}

void Server::addChannelList(std::string channelName, int fd)
{
	_channelList.insert(std::pair<std::string, Channel *>(channelName, new Channel(channelName, fd)));
}

void Server::removeUnconnectClient(int fd)
{
	Client *tmp = findClient(fd);

	std::string str = tmp->getMsgBuffer();
	send(fd, str.c_str(), str.length(), 0);

	std::cout <<"----- in removeclient sendMsg to <" << fd << "> -------\n";
	std::cout << str;
	std::cout << "--------------------------\n" << std::endl;
	str.clear();
	tmp->clearMsgBuffer();
	getClientList().erase(fd);
    close(fd);
    delete tmp;
}

void Server::send_msg()
{
	std::map<int, Client *>::iterator iter = _clientList.begin();
	for (; iter != _clientList.end(); iter++)
	{
		if (iter->second->getMsgBuffer().empty() == false)
		{
			std::string str = iter->second->getMsgBuffer();
			send(iter->first, str.c_str(), str.length(), 0);
			std::cout <<"----- send_msg to <" << iter->first << "> -------\n";
			std::cout << str;
			std::cout << "--------------------------\n" << std::endl;
			str.clear();
			iter->second->clearMsgBuffer();
		}
	}	
}

int Server::rec_cmd(int i, char *buf)
{
	Client * tmp = (_clientList.find(_pollClient[i].fd))->second;
	tmp->appendRecvBuffer(std::string(buf));

	std::cout << "---- recvMsgBuf ---- \n";
	std::cout << tmp->getRecvBuffer();
	std::cout << "--- endRecvMsgBuf ---\n";
	std::cout << "pollfd : " << _pollClient[i].fd << std::endl << std::endl;

	if (tmp->getRecvBuffer().find("\r\n") == std::string::npos)
		return 1;
	std::vector<std::string> cmd = split(tmp->getRecvBuffer(), "\r\n");
	if (cmd[0] == "")
		return 1;
	print_stringVector(cmd);

	if (!(tmp->getRegist() & REGI))
		_command.welcome(cmd, (_clientList.find(_pollClient[i].fd))->second, _clientList);
	else
	{
		std::vector<std::string>::iterator cmd_it = cmd.begin();
		while (cmd_it != cmd.end())
		{
			std::vector<std::string> result = split(*cmd_it, " ");
			check_cmd(result[0], result, tmp);
			cmd_it++;
		}
		tmp->getRecvBuffer().clear();
	}
	return 0;
}

void Server::relayEvent()
{
	char buf[512];
	for (int i = 1; i <= _maxClient; i++)
	{
		if (_pollClient[i].fd < 0)
			continue;
		if (_pollClient[i].revents & (POLLIN))
		{
			memset(buf, 0x00, 512);
			if (recv(_pollClient[i].fd, buf, 512, 0) <= 0)
			{
				std::vector<std::string> tmp_vec;
				tmp_vec.push_back("QUIT");
				tmp_vec.push_back(":lost connection");
				_command.quit(tmp_vec, findClient(_pollClient[i].fd));
				_pollClient[i].fd = -1;
			}
			else
			{
				if (rec_cmd(i, buf) == 1)
					continue;
			}	
		}
		else if (_pollClient[i].revents & (POLLERR))
		{
			std::cout << "--- ERROR ---" << std::endl;
			exit(1);
		}
	}
	send_msg();
}

int Server::execute()
{
	while (42)
	{
		_pollLet = poll(_pollClient, _maxClient + 1, -1);
		if (_pollLet == 0 || _pollLet == -1)
		{
			std::cerr << "pollet : " << _pollLet << std::endl;
			break ;
		}
		if (_pollClient[0].revents & POLLIN)
		{
			if ((pollingEvent()) == -1){
				continue;
			}
		}
		relayEvent();
	}
	close(_serverSocketFd);
	return (0);
}
