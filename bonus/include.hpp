#ifndef INCLUDE_HPP
#define INCLUDE_HPP

#include <exception>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <sstream>
#include <vector>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <vector>
#include <sstream>
#include <map>
#include <cstring>
#include <algorithm>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define FD_MAX 1000
#define DEBUG 1
#define DATA_SIZE 10000
#define PWD_OPER "root"
#define SERVER_NAME "ft_irc.com"
#define ALLOWED_CHAR "`|^_-{}[]\\abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"

#endif