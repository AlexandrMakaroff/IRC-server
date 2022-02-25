
#include "Channel.hpp"

Channel::Channel(std::string name, std::string key) { 
    _name = name;
    _key = key;
}

Channel::Channel(int user, std::string name, std::string key){
    _name = name;
    _key = key;    
	_users.push_back(user);
}

Channel & Channel::operator=(Channel const & other){
    _users = other.getUsers();
    _name = other.getName();
    _key = other.getKey();
	return *this;
}

Channel::Channel(Channel const & other){
	*this = other;
}

Channel::~Channel(){ }

void	Channel::eraseUser(int fd){
	_users.erase(find(_users.begin(), _users.end(), fd));
}

std::string	Channel::getName() const {
    return _name;
}
std::string Channel::getKey() const {
    return _key;
}
std::vector<int> Channel::getUsers() const {
    return _users;
}

void	Channel::addUser(int fd) {_users.push_back(fd);}
void	 Channel::setKey(std::string key) {_key=key;}
