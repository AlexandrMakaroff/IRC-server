#include "Server.hpp"

int	Server::whichCommand(std::string & request) {
	const char* arr[] = {"PASS","NICK","USER","JOIN","OPER","QUIT","MSG", "HELP", "KILL", "UPLOAD", "DOWNLOAD"};
	std::istringstream iss(request);
	std::string firstWord;
	std::vector<std::string>::iterator it;

	iss >> firstWord;
	std::transform(firstWord.begin(), firstWord.end(),firstWord.begin(), ::toupper);
	std::vector<std::string> commandList(arr, arr + sizeof(arr)/sizeof(arr[0]));
	if (find(commandList.begin(), commandList.end(), firstWord) != commandList.end())
		for (size_t i = 0; i < commandList.size(); i++)
			if (firstWord == commandList[i])
				return i;
	return -1;	
}

void Server::passCommand(std::string & request, int fd) {
	std::string req =	request.substr(strlen("PASS"));
	std::stringstream 	info(req);
	std::string			password;

	info >> password;
	if (password.empty())
	{
		send_to_fd("461", "PASS : Syntax error - use PASS <password>", _userList[fd], fd, false);
		return;
	}
	else
	{
		_userList[fd].setTmpPwd(password);

		if (!checkRegistration(fd, 1))
			return;
		else {
			send_to_fd("381", ":Password saved", _userList[fd], fd, false);
		}
	}
}

void Server::nickCommand(std::string & request, int fd) {
	std::string req =	request.substr(strlen("NICK"));
	std::stringstream 	info(req);
	std::string			username, oldNick;
	unsigned int		countParams = 0;

    if(info >> username) { ++countParams;}

	if (!checkRegistration(fd, 1))
		return;

	if (username.empty() || countParams == 2 || (username.length() > 9) || username.find_first_not_of(ALLOWED_CHAR) != std::string::npos){
		send_to_fd("461", "NICK :Syntax error - NICK <nickname>", _userList[fd], fd, false);
		return;
	}
	for (std::map<int, Client>::iterator it = _userList.begin(); it != _userList.end(); it++)
		if (it->second.getNickname().compare(username) == 0) { // already same nickname
			send_to_fd("462",  ":Nickname is already in use", it->second, fd, false);
			return;
		}
	if (_userList[fd].getNickname().compare("*") != 0)
		oldNick = _userList[fd].getNickname();
	_userList[fd].setNickname(username);
	if (_userList[fd].isRegistered())
	{
		std::string rep(":"); rep += oldNick; rep += "!~"; rep += _userList[fd].getClientname(); rep += "@localhost NICK :"; rep += username; rep += "\n";
		send(fd, rep.c_str(), rep.length(), 0);
		return ;
	}
    send_to_fd("381", ":Nickname saved", _userList[fd], fd, false);
}


void Server::userCommand(std::string & request, int fd) {
	std::string req =	request.substr(strlen("USER"));
	std::stringstream 	info(req);
	std::string			nickname, username, password;

	info >> nickname;
	info >> username;
	info >> password;
	if (nickname.empty() || username.empty() || password.empty())
	{
		send_to_fd("461","USER : Syntax error - use USER <nickname> <username> <password>", _userList[fd], fd, false);
		return;
	}
	else if (_userList[fd].getRegistered())
	{
		send_to_fd("462", ":You are already registered", _userList[fd], fd, false);
		return;
	}
	else
	{
        std::cout << "nickname: " << nickname << std::endl
        		<< "username: " << username << std::endl
        		<< "password: " << password << std::endl;
		_userList[fd].setNickname(nickname);
		_userList[fd].setClientname(username);
		_userList[fd].setTmpPwd(password);
		if (!checkRegistration(fd, 0))
			return;
		else
		{
			send_to_fd("381", ":User saved", _userList[fd], fd, false);
		}
	}
}

void Server::joinCommand(std::string & request, int fd) {
	std::string str = request.substr(strlen("JOIN"));
	std::stringstream 	stream(str);
	std::string		chans;
	std::string		keys;
	unsigned int	countParams = 0;
	if(stream >> chans) { ++countParams;}
	if(stream >> keys) { ++countParams;}
	if(stream >> chans) { ++countParams;}

	std::string firstchan, firstkey;


	if (!checkRegistration(fd, 1))
		return;
    

	if (countParams < 1 || countParams > 2) { //BAD SYNTAX
		send_to_fd("461", "JOIN :Syntax error, JOIN #<channel>",_userList[fd], fd, false);
	}
	while (!chans.empty()) { //can have more chan than keys, so we dont care
		if (chans.find(',')) //more than 1 chan
			firstchan = chans.substr(0, chans.find(','));
		else
			firstchan = chans;

		if (keys.find(',')) //more than 1 keys
			firstkey = keys.substr(0, chans.find(',') - 1);
		else
			firstkey = keys;
		std::map<std::string, Channel >::iterator itchan = _channels.find(firstchan);
		if (itchan != _channels.end()) { //add to existing chan
			std::vector<int> users = itchan->second.getUsers(); //check if user isnt already in
			if (find(users.begin(), users.end(), fd) ==  users.end()){
				itchan->second.addUser(fd);
				joinMsgChat(_userList[fd], firstchan, fd, "JOIN", std::string(""));
				std::cout << "Add to existing chan " <<  firstchan <<  std::endl;
				for (std::vector<int>::iterator it = users.begin(); it != users.end(); it++)
					if ((*it) != fd)
						joinMsgChat(_userList[fd], firstchan, (*it), "JOIN", std::string(""));
			}
		}
		else { //chan must begin with #, cant contain spaces/ctrl G/comma
			if ((firstchan.find_first_of("#") == 0) && (firstchan.find_first_of(" ,\x07") == std::string::npos)) {
				_channels.insert(std::pair<std::string, Channel>(firstchan, Channel(fd, firstchan, firstkey)));
				joinMsgChat(_userList[fd], firstchan, fd, "JOIN", std::string(""));
				std::cout << "Creating new channel : " <<  firstchan << std::endl;
			}
			else //bad chan name
				send_to_fd("403", std::string(firstchan)+" :No such channel", _userList[fd], fd, false);
		}
		if (chans.find(',') != std::string::npos) //more than 1 chan
			chans = chans.substr(chans.find(',')+1);
		else
			chans.erase();
	}
}

void Server::operCommand(std::string & request, int fd) {
	std::string str = request.substr(strlen("OPER"));
	std::stringstream 	stream(str);
	std::string		user;
	std::string		password;
	unsigned int	countParams = 0;
	if (stream >> user) { ++countParams;}
	if (stream >> password) { ++countParams;}

	if (!checkRegistration(fd, 1))
		return;

	if (countParams != 2) {
		send_to_fd("461",  "OPER :Syntax error, OPER <nickname> <password>", _userList[fd], fd, false);
		return;
	}
	if (password.compare(PWD_OPER) == 0) {
		_userList[fd].setOperName(user);
		send_to_fd("381",  ":You are now an IRC operator", _userList[fd], fd, false);
	}
	else {
		send_to_fd("464",  ":Password incorrect", _userList[fd], fd, false);
	}
}

void Server::quitCommand(std::string & request, int fd) {
	std::vector<int> users_to_contact;

	std::string str = request.substr(strlen("QUIT"));
	std::string message;

	if (str.empty()) //no message, init with nickname
		message = ":Client closed connection";
	else
		message = str.substr(str.find_first_not_of(" "));
	if (std::count(message.begin(), message.end(), ' ') > 0 && message[0] != ':') {//there is more than one word, : needed
		send_to_fd("461",  "QUIT :Syntax error, QUIT <message>", _userList[fd], fd, false);
		return;
	}
	std::map<std::string, Channel >::iterator ite = _channels.end();

	for (std::map<std::string, Channel >::iterator it = _channels.begin(); it != ite; it++) //fill users to contact with users who shares channels with the leaver
	{
		std::vector<int> tmp_users = it->second.getUsers();
		if (find(tmp_users.begin(), tmp_users.end(), fd) != tmp_users.end())
		{
			it->second.eraseUser(fd);
			for (std::vector<int>::iterator itusers = tmp_users.begin(); itusers != tmp_users.end(); itusers++)
			{
				if (find(users_to_contact.begin(), users_to_contact.end(),(*itusers)) == users_to_contact.end())
				users_to_contact.push_back((*itusers));
			}
		}
	}
	for (std::vector<int>::iterator contact = users_to_contact.begin(); contact != users_to_contact.end(); contact++)
	{
		if (*contact != fd) {
			std::string rep(":");
			rep.append( _userList[fd].getNickname());
			rep.append("!~");
			rep.append(_userList[fd].getClientname());
			rep.append("@localhost QUIT ");
			rep.append(message);
			rep.append("\n");
			send((*contact), rep.c_str(), rep.length(), 0);
			std::cout << rep;
		}
	}
	close_fd(fd);
}

void Server::msgCommand(std::string & request, int fd) {
	std::string str = request.substr(strlen("MSG"));
	std::stringstream 	stream(str);
	std::string		dests, message, tmp, firstdest;
	unsigned int	countParams = 0;
	if(stream >> dests) { ++countParams;}
	if(stream >> message) { ++countParams;}
	while (stream >> tmp) { ++countParams;}

	if (!checkRegistration(fd, 1))
		return;

	if (countParams == 0) {
		send_to_fd("411", ":No recipient given, MSG <nickname> <message>", _userList[fd], fd, false);
		return;
	}

	if (countParams == 1) {
		send_to_fd("412",  "No text to send, MSG <nickname> <message>", _userList[fd], fd, false); //only dest, no params
		return;
	}

	if (countParams > 2 && message[0] != ':') {//there is more than one word, : needed
		send_to_fd("461",  "MSG :Syntnax error, too few arguments to <message>", _userList[fd], fd, false);
		return;
	}
	str = str.substr(str.find(dests) + dests.length());
	str = str.substr(str.find_first_not_of(" "));
	if (str[0] == ':') str = str.substr(1); //remove first char if ":"

	while(!dests.empty()) {
		bool disp = true;
		if (dests.find(',')) //more than 1 chan
			firstdest = dests.substr(0, dests.find(','));
		else
			firstdest = dests;
		std::map<std::string, Channel >::iterator itchan = _channels.find(firstdest);
		if (itchan != _channels.end()) {
			std::vector<int> users = itchan->second.getUsers();
			for (std::vector<int>::iterator it = users.begin(); it != users.end(); it++) {
				if ((*it) != fd) {
					joinMsgChat(_userList[fd], firstdest, (*it), "MSG", str);
				}
				disp = false;
			}
		}
		for (std::map<int, Client>::iterator it = _userList.begin(); it != _userList.end(); it++)
		{
			if (it->second.getNickname() == firstdest)
			{
				joinMsgChat(_userList[fd], firstdest, it->first, "MSG", str);
				disp = false;
			}
		}
		if (disp)
			send_to_fd("401", ":No such nick or channel name",_userList[fd],fd,false);
		if (dests.find(',') != std::string::npos) //more than 1 chan
			dests = dests.substr(dests.find(',')+1);
		else
			dests.erase();
	}
}

void Server::helpCommand(std::string & request, int fd) {
	(void)request;
	std::string rep = "Allowed commands:\n"; 
	rep.append("\"PASS <password>\" to set the 'password'\n");
	rep.append("NICK <nickname>\n");
	rep.append("The NICK message is used to give a user a nickname\n\n");
	rep.append("USER <username> . . <real name>\n");
	rep.append("The USER message is used at the start of a connection to specify the user name, host name, server name, and real name of a new user)\n\n");
	rep.append("JOIN <channel1,channel2>\n");
	rep.append("The JOIN command is used by a client to start listening to a specific channel\n\n");
	rep.append("OPER <user> <password>\n");
	rep.append("The OPER message is used by a normal user to obtain the operator privilege\n\n");
	rep.append("QUIT <message>\n");
	rep.append("A client session ends with a QUIT message can add a leave message\n\n");
	rep.append("MSG <recipient>(1 or more) <:text to send>\n");
	rep.append("MSG is used to send a private message between users\n\n");
	rep.append("OPER <username> <password>\n");
	rep.append("OPER is used to have operator privileges\n\n");
	rep.append("KILL <user> <message>\n");
	rep.append("The KILL command is used to remove a user from the server\n\n");
	send(fd, rep.c_str(), rep.length(), 0);
}

void	Server::killCommand(std::string & request, int fd) {
	std::string str = request.substr(strlen("KILL"));
	std::stringstream 	stream(str);
	std::string		target, message, tmp;
	unsigned int	countParams = 0;
	if (stream >> target) { ++countParams;}
	if (stream >> message) { ++countParams;}
	while (stream >> tmp) {++countParams;}

	if (!checkRegistration(fd, 1))
		return;

	if (countParams < 2 || (countParams > 2 && message[0] != ':')) {
		send_to_fd("461", "KILL :Syntax error, KILL <username> <message>", _userList[fd], fd, false);
		return;
	}
	if (_userList[fd].getOperName().empty()) {
		send_to_fd("481", ":Permission Denied- You're not an IRC operator",_userList[fd],fd,false);
		return;
	}
	for (std::map<int, Client>::iterator it = _userList.begin(); it != _userList.end(); it++){
		if (target == it->second.getNickname()) {
			std::string rep("ERROR : KILLed by ");
			rep.append(_userList[fd].getNickname());
			rep.append(": ");
			rep.append(str);
			rep.append("\n");
			send(it->first, rep.c_str(), rep.length(), 0);
			close_fd(it->first);
			return;
		}
	}
}


void	Server::uploadCommand(std::string & request, int fd)  {
	std::string str = request.substr(strlen("UPLOAD"));
	std::stringstream 	stream(str);
	std::string		filename, tmp;
	unsigned int	countParams = 0;

	if (stream >> filename) {++countParams;}
	while (stream >> tmp) {++countParams;}
	if (filename.empty() || countParams != 1)
	{
		send_to_fd("461", "UPLOAD :Syntax error, UPLOAD <path_to_file>", _userList[fd], fd, false);
		return;		
	}

	std::ifstream ifs;
    std::ofstream ofs("upload_file");
	std::string buf;

    ifs.open(filename);

    if (!ifs.is_open())
		send_to_fd("461", "UPLOAD :can't open file", _userList[fd], fd, false);
	while(!ifs.eof())
    {
        buf = "";
        getline(ifs, buf);
        ofs << buf << std::endl;
    }
}

void	Server::downloadCommand(std::string & request, int fd) {
	std::string str = request.substr(strlen("DOWNLOAD"));
	std::stringstream 	stream(str);
	std::string		filename, tmp;
	unsigned int	countParams = 0;

	if (stream >> filename) {++countParams;}
	while (stream >> tmp) {++countParams;}
	if (filename.empty() || countParams != 1)
	{
		send_to_fd("461", "DOWNLOAD :Syntax error, DOWNLOAD <filename>", _userList[fd], fd, false);
		return;		
	}
	const int _size=4096;
    char _buf[_size];

    FILE* file;
    file = fopen(filename.c_str(), "rb");
    int BytesRead = 0;
    while (!feof(file)) {
        memset(_buf, 0, _size);
        BytesRead = fread(_buf, 1, _size, file);
        if (BytesRead > 0) {
            send(fd, _buf, BytesRead, 0);
            std::cout << BytesRead << std::endl;
            memset(_buf, 0, _size);
            recv(fd, _buf, _size, 0);
        }
    }
    send(fd, "eof", 3, 0);
    fclose(file);
    std::cout << "transmitting successful" << std::endl;
}