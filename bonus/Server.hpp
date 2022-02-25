#ifndef SERVER_HPP
#define SERVER_HPP

#include "include.hpp"
# include "Channel.hpp"
# include "Client.hpp"

class Client;
class Channel;

class Server
{
private:
    std::string _serv_port;
    std::string _serv_pass;
    std::string _host;
    std::string _client_pass;
    std::string _serv_port2;
    struct pollfd _pollfd[FD_MAX];
    int _fdmax;        // макс. число дескрипторов
    int _listener;     // дескриптор слушающего сокета
    std::map<int, Client> _userList;
    std::map<std::string, Channel> _channels;
    Server();

public:
    Server(int ac, char **av);
    Server(Server const & copy);
    ~Server();
    Server & operator= (Server const & copy);

    void start();
    void announce();
    void exec();

    void	send_to_fd(std::string code, std::string message, Client const & user, int fd, bool dispRealName) const;
    void	close_fd(int fd);

	int		whichCommand(std::string & request);
	int		checkPassword(Client & user);
	int		checkRegistration(int fd, int flag);

    void	processRequest(std::string & request, int fd);
	void	parseRequest(std::string request, int fd);
	void	joinMsgChat(Client const & user, std::string channel, int fd, std::string command, std::string message);
    
	std::string	getNbUsers() const;
	std::string	getNbChannels() const;

	void 	passCommand(std::string & request, int fd);
	void 	nickCommand(std::string & request, int fd);
	void 	userCommand(std::string & request, int fd);
	void 	joinCommand(std::string & request, int fd);
	void 	operCommand(std::string & request, int fd);
	void 	quitCommand(std::string & request, int fd);
	void 	msgCommand(std::string & request, int fd);
    void    uploadCommand(std::string & request, int fd);
    void    downloadCommand(std::string & request, int fd);
	void	helpCommand(std::string & request, int fd);
	void	killCommand(std::string & request, int fd);
    class  InvalidNumArgs : public std::exception {
        public:
                virtual const char * what() const throw (){
                    return("Invalid num of args. Example : \"./ircserv [host:port_network:password_network] <port> <password>\" or \"./ircserv <port> <password>\"");
                }
    };
    class  InvalidArgs : public std::exception {
        public:
                virtual const char * what() const throw (){
                    return("Invalid port or password");
                }
    };
    
};

#endif