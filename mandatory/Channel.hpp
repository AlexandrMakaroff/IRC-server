

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "include.hpp"

class Channel
{
private:
	Channel();
	std::string			_key;
	std::string         _name;
	std::vector<int>	_users;
public:
	Channel(std::string name, std::string key);
	Channel(int user, std::string name, std::string key);
	Channel & operator=(Channel const & other);
	Channel(Channel const & other);
	~Channel();

	std::string	getName() const;
	std::string getKey() const;
	std::vector<int> getUsers() const;

	void	 setKey(std::string key);
	void	addUser(int fd);
	void	eraseUser(int fd);
};

#endif