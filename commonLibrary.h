#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <sys/wait.h>
#include "message/message.h"
#include "json/json.h"
#include "log/log.h"
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdexcept>
#include "setting.h"
#include "timeheap.h"



#include <algorithm>
#include <vector>
#include <map>

using namespace std;
using json = nlohmann::json;

#define safe_delete(p) if(p){delete p;p=NULL;}





int newServer(const char* ip, int port, int maxReach);

union semun {
	int val;
	semid_ds* buf;
	unsigned short int* array;
	seminfo* _buf;
};

void error_and_die(const char *msg) {
	perror(msg);
	exit(EXIT_FAILURE);
}

void pv(int sem_id, int op) {
	struct sembuf sem_b;
	sem_b.sem_op = op;
	sem_b.sem_num = 0;
	sem_b.sem_flg = SEM_UNDO;
	semop(sem_id, &sem_b, 1);
}




class Server {

private:
	const char *ip;
	int port;
	int maxReach;

public:
	int sockfd = 0;
	Server(int _maxReach = -1, int _port = -1, const char *_ip = NULL) {
		maxReach = (_maxReach != -1) ? _maxReach : 5;
		port = (_port != -1) ? _port : 8080;
		ip = (_ip != NULL) ? _ip : "127.0.0.1";

	}
	void start() {
		sockfd = newServer(ip, port, maxReach);
	}
	void stop() {
		close(sockfd);
	}
};

// create a new server; normal setting ;
int newServer(const char* ip, int port, int maxReach) {
	sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &address.sin_addr);
	address.sin_port = htons(port);

	int sock = socket(PF_INET, SOCK_STREAM, 0);
	assert(sock >= 0);
	int reuse = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	int ret = bind(sock, (sockaddr*) &address, sizeof(address));
	assert(ret != -1);

	ret = listen(sock, maxReach);
	assert(ret != -1);
	return sock;

}

//create a new connection, common setting;
int newClient(const char* ip, int port) {
	sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &address.sin_addr);
	address.sin_port = htons(port);
	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	assert(sockfd >= 0);
	return sockfd;
}

//add new signal handle function
void addsig(int sig, void (*sig_handler)(int)) {
	struct sigaction sa;
	memset(&sa, '\0', sizeof(sa));
	sa.sa_handler = sig_handler;
//	sa.sa_flags |= SA_RESTART;
	sigfillset(&sa.sa_mask);
	assert(sigaction(sig, &sa, NULL) != -1);
}

//set nonblock
int setnonblocking(int fd) {
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}

//new event epoll
void addfd(int epollfd, int fd, unsigned ets) {
	epoll_event event;
	event.data.fd = fd;
	event.events = ets;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
	setnonblocking(fd);
}

void editfd(int epollfd, int fd, unsigned ets) {
	epoll_event event;
	event.data.fd = fd;
	event.events = ets;
	epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

char* safeRead(int fd, int bufsize) {
	char *k = new char[bufsize];
	int ret = recv(fd, k, bufsize - 1, 0);
//	printf("get %d bytes from connection %d \n", ret, fd);
	if (ret < 0) {
		return NULL;
	}
	return k;
}

int safeSend(int fd, char* buf) {
	int ret = send(fd, buf, strlen(buf), 0);
	return ret;
}

int sendJson(int fd, json j) {
	string s = j.dump();
	char buf[s.length() + 1];
	sprintf(buf, "%s", s.c_str());
	int ret = send(fd, buf, strlen(buf), 0);
	return ret;
}

// unfinished
int check(string user, string password) {
	return 1;
}

string generateMid(vector<string> &lists){
	string mid = "123";
	return mid;
}

#define MAX_PATH  1000
//const std::string strCfgName = "logger_import_db.conf" ;

bool fGetCfgFileName(std::string& paraStr_CfgFileName)
{
    paraStr_CfgFileName.clear() ;
    char szWorkDir[MAX_PATH] = {0} ;
//    char szCfgFileNameTemp[MAX_PATH] = {0} ;
    if(!getcwd(szWorkDir, 260))
    {
        return false ;
    }

    paraStr_CfgFileName = szWorkDir ;
//    paraStr_CfgFileName.append("/") ;
//    paraStr_CfgFileName.append(strCfgName) ;

    return true ;
}

bool safe_parse(string e, json&t){
	try{
		t = json::parse(e);
		return true;
	}catch(const std::invalid_argument& xx){
//		printf("%s %d\n",xx.what(),time(NULL));
		LogPrinter::outputD("%s cannot parse ,error: %s in %s",e.c_str(),xx.what(),__func__);
		return false;
	}
}

