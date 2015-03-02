/*
 * main.cpp
 *
 *  Created on: Feb 26, 2015
 *      Author: rrroen
 */

#include "commonLibrary.h"
#include "setting.h"
#include "timeheap.h"
#include "data/data.h"

#include <vector>
#include <map>

using namespace std;

//vector<int> process;

//share memory of fileMan.

struct memoryData {
	//please set a time_heap to auto_release File;
	File opened[MAX_FILE_OPENED];
	bool flag[MAX_FILE_OPENED];
	int sem_id;
	memoryData() {
		sem_id = semget(IPC_PRIVATE, 1, 0666);
		semun sem_um;
		sem_um.val = MAX_FILE_OPENED;
		semctl(sem_id, 0, SETVAL, sem_um);
		memset(flag, 0, sizeof(flag));
	}
	int getAFile(const char* filename) {
		for (int i = 0; i < MAX_FILE_OPENED; i++)
			if (opened[i].name == filename)
				return i;
		pv(sem_id, -1);
		for (int i = 0; i < MAX_FILE_OPENED; i++)
			if (!flag[i]) {
				flag[i] = true;
				opened[i].loadFile(filename);
				return i;
			}
	}
	void releaseFile(int pos) {
		pv(sem_id, 1);
		flag[pos] = false;
		opened[pos].clear();
	}
};

map<int, int> bindProandConn;

const char *memname = "sharedmemroy";
const size_t size = sizeof(memoryData);

void sig_child(int signo) {
	pid_t pid;
	int stat;
	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
		;
	return;
}

void terminateConnection(client_data* user_data) {
	close(user_data->sockfd);

}

void work(int connfd) {
	string user;
	string password;
	time_heap timeHeap(10);
	int epollfd = epoll_create(MAX_NUM_EPOLL_EVENTS);
	epoll_event events[MAX_NUM_EPOLL_NUM];
	addfd(epollfd, connfd, EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET);
	char buf[100] = "welcome to connect , Input Username first:\n";
	json *_json;
	bool sendbuf = true;
	bool sendJ = false;
	bool admin = false;
	int login = 0;
	while (1) {
		timeHeap.tick();
		int ret = epoll_wait(epollfd, events, MAX_NUM_EPOLL_NUM, -1);
		if (ret < 0) {
			//more , remeber the hostname and print the connection .
			printf("epoll failure in connfd %d \n", connfd);
			break;
		}
		for (int i = 0; i < ret; i++) {
			if (events[i].events & EPOLLIN) {
				char *tmp = safeRead(events[i].data.fd, MAX_READ_BUF_SIZE);
				Message newMessage(tmp);
				int ret = newMessage.extract();
				printf("messageType : %d\n", newMessage.messageType);
				if (!ret
						|| ((newMessage.messageType == 0
								|| newMessage.messageType == 1) && login)) {
					sprintf(buf,
							"an error had occured, the connection will close in 2 sec \n");
					sendbuf = true;
					heap_timer *timer = new heap_timer(2);
					timer->cb_func = terminateConnection;
					timer->user_data = new client_data;
					timer->user_data->sockfd = connfd;
					timeHeap.add_timer(timer);
				}
				switch (newMessage.messageType) {
				case LOG_IN_USER:
					user = newMessage.source;
					sprintf(buf, "Please enter your password: User %s \n",
							user.c_str());
					sendbuf = true;
					break;
				case LOG_IN_PASSWORD:
					password = newMessage.source;
					login = check(user, password);
					admin = (login == 2);
					if (!login) {
						sprintf(buf, "password or username wrong \n ");
						sendbuf = true;
					} else {
						sprintf(buf, "welcome back , %s \n", user.c_str());
						sendbuf = true;
					}
					break;
				case GET_LIST:
					if (login) {
						sendJ = true;
						_json = new json();
						(*_json)["type"] = "MessageType";
						(*_json)["num"] = 3;
					} else {
						sprintf(buf, "you have not login \n");
						sendbuf = true;
					}
					break;
				default:
					break;
				}
				printf("the buf is :%s\n", buf);
			} else if (events[i].events & EPOLLRDHUP) {

			}
			//seperate the in and out.
			if (events[i].events & EPOLLOUT) {
				if (sendbuf) {
					int ret = safeSend(events[i].data.fd, buf);
					if (ret > 0)
						sendbuf = false;
				} else if (sendJ) {
					int ret = sendJson(events[i].data.fd, (*_json));
//					delete [] _json;
//					_json = NULL;
					sendJ = false;
				}
			}
		}

	}

}

int main(int argc, char **argv) {
	// parse parameter
	addsig(SIGCHLD, sig_child);
	printf("good\n");
	// using default
	Server newServer;
	if (argc > 1) {

	} else {

	}
	newServer.start();
	int memoryfd = shm_open(memname, O_CREAT | O_TRUNC | O_RDWR, 0666);
	if (memoryfd == -1) {
		error_and_die("Can not open the shared memory");
	}
	int r = ftruncate(memoryfd, size);
	if (r == -1) {
		error_and_die("Can not change the size");
	}
	void * ptr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, memoryfd, 0);
	new (ptr) memoryData;

	int epollfd = epoll_create(MAX_NUM_EPOLL_EVENTS);
	epoll_event events[MAX_NUM_EPOLL_NUM];
	assert(epollfd != -1);
	addfd(epollfd, newServer.sockfd, EPOLLIN | EPOLLET);
	while (1) {
		int ret = epoll_wait(epollfd, events, MAX_NUM_EPOLL_NUM, -1);
		if (ret < 0) {
			printf("epoll failure\n");
			break;
		}
		for (int i = 0; i < ret; i++) {
			int sockfd = events[i].data.fd;
			if (sockfd == newServer.sockfd) {
				sockaddr_in client_address;
				socklen_t client_addrlength = sizeof(client_address);
				int connfd = accept(newServer.sockfd,
						(sockaddr*) &client_address, &client_addrlength);
				int pid = fork();
				if (pid == -1) {
					//	printf("cannot create new process to handle new connection");
				} else if (pid != 0) {

					bindProandConn[connfd] = pid;
					close(connfd);
					// debug the child process.
					//return 0;
					// printf("father process stop handle the connection");
				} else {
					//	printf("one new connection");
					work(connfd);
					close(connfd);
					return 0;
				}
			}
		}

	}
	newServer.stop();
	return 0;
}

