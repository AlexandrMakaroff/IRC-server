#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctime>

#define MAXDATASIZE 100 // максимальное число байт, принимаемых за один раз

// получение структуры sockaddr, IPv4 или IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 4) {
		printf("usage: client <hostname> <port> <pass>");
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // неважно, IPv4 или IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream-sockets

	if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
		std::cout << "getaddrinfo() error.\n";
		return 1;
	}

// Проходим через все результаты и соединяемся к первому возможному
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
							 p->ai_protocol)) == - 1) {
			perror("client: socket");
			continue;
		}
		/* соединяемся с хостом */
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == - 1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}
		break;
	}

	if (p == NULL) {
		std::cout <<"client: failed to connecting" << std::endl;
		return 2;
	}
	/* преобразуем IP в строку */
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			  s, sizeof s);
	std::cout << "bot: connecting to " << s << std::endl;

	freeaddrinfo(servinfo); // эта структура больше не нужна

	std::string stroka;
	
	std::string str = "USER bot bot ";
	str.append(argv[3]);
	str.append("\r\nJOIN #bot\r\n");
	
	send(sockfd, str.c_str(), str.size() + 1, 0);
	while(1)
	{
		if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1)
		{
			perror("recv");
			exit(1);
		}
		buf[numbytes] = '\0';
		std::cout << "client: received " << buf << std::endl;
		stroka = buf;

		if (stroka.find("[bot]: hi") != std::string::npos)
		{
			send(sockfd, "MSG #bot :Hello im a so small and cute bot! I can't do anything yet, but maybe someday I will learn interesting commands! BUT! Even now I can tell you how beautiful you are!\r\n", 179, 0);
		}
		else if (stroka.find("[bot]: time") != std::string::npos)
		{
			time_t rawtime;
  			struct tm * timeinfo;
  			char buffer[80];

  			time (&rawtime);
  			timeinfo = localtime(&rawtime);

  			strftime(buffer,sizeof(buffer),"%d-%m-%Y %H:%M:%S",timeinfo);
  			std::string times = "MSG #bot : current time: ";
			times.append(buffer);
			times.append("\r\n");
			std::cout << times << std::endl;
			send(sockfd, times.c_str(), times.size() + 1, 0); 
		}
		stroka.clear();
	}

	close(sockfd);

	return 0;
}