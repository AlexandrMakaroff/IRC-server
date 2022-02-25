#include "Server.hpp"

Server::Server(int ac, char **av) : _client_pass(""), _serv_port2("")
{
    int check_port;
    if (ac < 3 || ac > 4)
       throw Server::InvalidNumArgs();
    if (ac == 3)
    {
        _host = "127.0.0.1";
        check_port = atoi(av[1]);
        _serv_pass = av[2];
        if ((check_port < 1024 || check_port > 65000) || _serv_pass.empty())
            throw Server::InvalidArgs();
        _serv_port = av[1];
    }
    if (ac == 4)
    {
        std::vector<std::string> elems;
        std::stringstream ss(av[1]);
        std::string item;
        while (std::getline(ss, item, ':'))
        {
            if (!item.empty())
                elems.push_back(item);
        }
        if (elems.size() != 3)
            throw Server::InvalidNumArgs();
        _host = elems[0];
        check_port = stoi(elems[1]);
        _serv_pass = elems[2];
        int check_port2 = atoi(av[2]);
        _client_pass = av[3];
        if ((check_port < 1024 || check_port > 65000) || (check_port2 < 1024 || check_port2 > 65000) || _serv_pass.empty() || _client_pass.empty())
            throw Server::InvalidArgs();
        _serv_port = elems[1];
        _serv_port2 = av[2];
    }
    announce();
}

Server::Server()
{
}

Server::Server(Server const & copy)
{
    this->_fdmax = copy._fdmax;
    this->_client_pass = copy._client_pass;
    this->_host = copy._host;
    this->_listener = copy._listener;
    this->_serv_pass = copy._serv_pass;
    this->_serv_port2 = copy._serv_port2;
    this->_serv_port = copy._serv_port;
}

Server & Server::operator=(Server const & copy)
{
    if (this == &copy)
        return *this;
    this->_fdmax = copy._fdmax;
    this->_client_pass = copy._client_pass;
    this->_host = copy._host;
    this->_listener = copy._listener;
    this->_serv_pass = copy._serv_pass;
    this->_serv_port2 = copy._serv_port2;
    this->_serv_port = copy._serv_port;
    return *this;
}

Server::~Server()
{
    std::cout << "Server closed." << std::endl;
}

void Server::announce()
{
    std::cout << "Server config" << std::endl
        << "Host network : " << _host << std::endl
        << "Port network :" << _serv_port << std::endl
        << "Pass network : " << _serv_pass << std::endl;
        if (!_client_pass.empty() && !_serv_port2.empty())
        {
            std::cout << "Client pass : " <<  _client_pass << std::endl
                << "Port : " << _serv_port2 << std::endl;
        }
}

void Server::start()
{
    /* готовимся к запуску, заполняем структуру */

    struct addrinfo hints, *ai, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, _serv_port.c_str(), &hints, &ai)) != 0) 
        throw std::runtime_error("getaddrinfo() error.\n");
    for(p = ai; p != NULL; p = p->ai_next) {
        /* Создаём сокет для приёма входящих соединений */ 
        _listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (_listener < 0) {
            continue;
        }
        int yes = 1;

         /* Избегаем ошибки «address already in use». Разрешаем повторное использование дескриптора сокета.*/
        setsockopt(_listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        std::cout << "Server socked created with fd [" << _listener << "]" << std::endl;

        /* SUBJECT : "..use fcntl because MacOS X doesn’t implement write the same way as other Unix OSes.
            You must use non-blocking FD to have a result similar to other OSes."
            Устанавливаем для сокета неблокирующий режим.*/
        fcntl(_listener, F_SETFL, O_NONBLOCK);
        
        /* привязываем сокет к порту */
        if (bind(_listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(_listener);
        continue;
        }
        break;
    }

    /* если мы попали сюда, значит мы не смогли забиндить сокет */
    if (p == NULL)
        throw std::runtime_error("bind() error.\n");

    freeaddrinfo(ai); // с этим мы всё сделали

    /* ожидаем входящие соединения и слушаем. Входящие соединения встают в очередь, пока их не примут (accept()),
        в большинстве случаев хватит значения лимита от 5 до 10 */
    if (listen(_listener, 10) < 0) {
        close(_listener);
        throw std::runtime_error("listen() error.\n");
    }

    /* инициализируем pollfd структуру */
    memset(_pollfd, 0, sizeof(_pollfd));
    _pollfd[0].fd = _listener; // устанавливаем начальный прослушивающий сокет
    _pollfd[0].events = POLLIN; // POLLIN - входящие данные , POLLOUT - исходящие 
    _pollfd[0].revents = 0;
    _fdmax = 1;
}

void Server::exec()
{
    struct sockaddr_storage remoteaddr; // адрес клиента
    socklen_t addrlen = sizeof(remoteaddr);
    char buf[DATA_SIZE];  // буфер для данных клиента
    int rc;

    for(;;) {
        std::cout << "Listen " << _fdmax - 1 << " clients" << std::endl;
    
        /* можно установить таймаут (3 арг), и тогда программа завершится, 
        если в течение этого времени не будет никаких действий, но нам это не нужно  */
        if (poll(_pollfd, _fdmax, -1) == -1) 
            throw std::runtime_error("poll() error.\n");

        /* Один или несколько дескрипторов доступны для чтения. Нужно определить, какие именно. */
        for (int i = 0; i < _fdmax; ++i) {
            
            /* ищем дескрипторы, которые вернули POLLIN, определяем, прослушивающее соединение или активное */ 
            if (_pollfd[i].revents == 0)
                continue;
            
            if (_pollfd[i].revents != POLLIN)  // unexpected result
               throw std::runtime_error("poll() error.\n");
            
            /* обнаружили событие */
            if (_pollfd[i].revents & POLLIN) {

                /* дескриптор прослушивания доступен для чтения */
                if (i == 0) {
                
                /* примем входящее соединение, прежде чем вернуться в цикл и вызвать poll() снова */
                    int new_fd = accept(_pollfd[i].fd, (struct sockaddr *) &remoteaddr, &addrlen);
                    if (new_fd == -1) 
                        throw std::runtime_error("accept() error.\n");
                    std::cout << "New connection from fd " << new_fd << std::endl;
                    _userList.insert(std::pair<int, Client>(new_fd, Client()));

                    /* добавляем новое входящее соединение в pollfd */
                    _pollfd[_fdmax].fd = new_fd;
                    _pollfd[_fdmax].events = POLLIN;
                    _pollfd[_fdmax].revents = 0;
                    if (_fdmax < FD_MAX)
                        ++_fdmax;
                }

                /* это не прослушивающий сокет, поэтому существующее соединение должно быть читаемым */
                else {

                    /* получаем все входящие данные в этот сокет, прежде чем мы вернёмся и снова вызовем poll()*/
                    rc = recv(_pollfd[i].fd, buf, DATA_SIZE, 0);
                    if (rc == -1) {
                        std::cout << "recv() fail" << std::endl;
                    }
                    if (rc == 0) {
                        std::cout << "Connection closed." << std::endl;
                        std::string s("QUIT");
                        quitCommand(s, _pollfd[i].fd);
                    }

                    /* данные получены */
                    else {
                        std::string data(buf);
                        parseRequest(data, _pollfd[i].fd);
                        memset (buf, 0, DATA_SIZE);
                    }
                }
            }
        }
    }
}

void	Server::processRequest(std::string & request, int fd) {
	while(request.size() && isspace(request.front())) request.erase(request.begin()); // removes first spaces
	while(request.size() && isspace(request.back())) request.pop_back(); //remove last spaces
	if (whichCommand(request) > -1) {
		void		(Server::*ptr[])(std::string &, int) = {
		&Server::passCommand,
		&Server::nickCommand,
		&Server::userCommand,
		&Server::joinCommand,
		&Server::operCommand,
		&Server::quitCommand,
		&Server::msgCommand,
		&Server::helpCommand,
		&Server::killCommand,
        &Server::uploadCommand,
        &Server::downloadCommand
		};
		(this->*ptr[whichCommand(request)]) (request, fd);
	}
	else if (DEBUG)
	{
		std::string firstdest;
		firstdest = "";
		for (std::map<int, Client>::iterator it = _userList.begin(); it != _userList.end(); it++) {
			joinMsgChat(_userList[fd], firstdest, it->first, "MSG", request);
		}
	}
	else {
		std::istringstream iss(request);
		std::string command;
		iss >> command;
		send_to_fd("421", std::string(command) +" :Unknown command, use HELP", _userList[fd], fd, false);
	}
}


void	Server::parseRequest(std::string request, int fd){
	std::string parse;
	std::map<int, Client>::iterator it = _userList.find(fd);

	request.erase(std::remove(request.begin(), request.end(), '\r'), request.end()); //erase \r, to work with netcat or irc client
	while (!request.empty())
	{
		if (request.find('\n') == std::string::npos) {// no \n found, incomplete request, add to user
			_userList[fd].appendTmpRequest(request);
			break;
		}
		else { //\n found, but maybe more than 1, check User._tmpRequest to append with it
			parse = it->second.getTmpRequest().append(request.substr(0, request.find_first_of("\n")));
			_userList[fd].cleanTmpRequest(); //request is complete, we can clean tmpReq;
			processRequest(parse, fd);
		}
		request = request.substr(request.find_first_of("\n") + 1);
	}
}

std::string	Server::getNbUsers() const{
	std::stringstream ss;
	ss << _userList.size();
	return ss.str();
}

std::string	Server::getNbChannels() const{
	std::stringstream ss;
	ss << _channels.size();
	return ss.str();
}

void	Server::send_to_fd(std::string code, std::string message,
Client const & user, int fd, bool dispRealName) const {
	std::string rep = SERVER_NAME;
	rep.append(user.getNickname());
	rep.append( ": ");
	rep.append(message);
	if (dispRealName){
		rep.append(" ");
		rep.append(user.getNickname());
		rep.append("!~");
		rep.append(user.getClientname());
		rep.append("@localhost(sendto)");
	}
	send(fd, rep.c_str(), rep.length(), 0);
	if (code.compare("001") != 0 &&	code.compare("251") != 0 &&
		code.compare("254") != 0 && code.compare("255") != 0)
	std::cout << rep << std::endl;
	return;
}

void	Server::joinMsgChat(Client const & user, std::string channel, int fd, std::string command, std::string message) {
	std::string rep("user: ");
	rep.append(user.getNickname());
	rep.append(" ->");
	if (command.compare("MSG") == 0)
		rep.append((std::string("[") + channel + "]: " + message));
	else
		rep.append((" " + channel));
	send(fd, rep.c_str(), rep.length(), 0);
	std::cout << rep;
}

int	Server::checkPassword(Client & user){
    if (user.getTmpPwd() != _serv_pass)
		return false;
	return true;
}

int		Server::checkRegistration(int fd, int flag) {
	if (_userList[fd].getNickname().length() < 1 || _userList[fd].getClientname().length() < 1 || _userList[fd].getTmpPwd().length() < 1)
	{
		send_to_fd("481", ":Permission Denied - Need registration. Try USER <username> <realname> <password>", _userList[fd], fd, false);
		return 0;
	}
	if (!checkPassword(_userList[fd]))
	{
		send_to_fd("481", ":Permission Denied - Bad password. Try again PASS", _userList[fd], fd, false);
		return 0;
	}
	if (flag)
		return 1;
	_userList[fd].setRegistered(1);
	std::cout <<  "User " << _userList[fd].getNickname() << " registered !" << std::endl;
	send_to_fd("001",  ":Welcome to IRCServer!\nUsers : " + getNbUsers() + "\n" +  "Channels : " + getNbChannels() + "\n", _userList[fd], fd, false);
	return 1;
}

void	Server::close_fd(int fd){
	for (int i = 1; i < _fdmax; i++){
		if (_pollfd[i].fd == fd) { //find which pollfds correspond to client's fd
			_pollfd[i].fd *= -1;
			if (i == _fdmax - 1) // last poll, just close and decr fd number
				close(fd);
			else { //switch the one to delete with the last one
				_pollfd[i] = _pollfd[_fdmax - 1];
				close(fd);
			}
			_userList.erase(fd);
			_fdmax--;
			}
		}
}