/*
 * main.cpp
 *
 *  Created on: Feb 26, 2015
 *      Author: rrroen
 */

#include "commonLibrary.h"
#include "data/data.h"
#include "user/user.h"
#include <string>

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

void work(int connfd, memoryData* ptr) {
	User *con = NULL;
	string user;
	string password;
	time_heap timeHeap(10);
	int epollfd = epoll_create(MAX_NUM_EPOLL_EVENTS);
	epoll_event events[MAX_NUM_EPOLL_NUM];
	addfd(epollfd, connfd, EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET);
	char buf[100] = "welcome to connect , Input Username first:";
	json js, list;
	bool listFlag = false;
	bool sendbuf = true;
	bool sendJ = false;
	bool flag = false;
//	int memoryfd = shm_open(memname, O_RDWR, 0666);
//	memoryData* ptr = static_cast<memoryData*>(mmap(0, size,
//	PROT_READ | PROT_WRITE, MAP_SHARED, memoryfd, 0));
	int fid = -1;
//	int *k = new int(3);
//	safe_delete(k);
	while (1) {
		timeHeap.tick();
		int ret = epoll_wait(epollfd, events, MAX_NUM_EPOLL_NUM, -1);
		if (ret < 0) {
			//more , remeber the hostname and print the connection .
			LogPrinter::output("epoll failure in connfd %d", connfd);
			break;
		}
		for (int i = 0; i < ret; i++) {
			if (events[i].events & EPOLLIN) {
				char *tmp = safeRead(events[i].data.fd, MAX_READ_BUF_SIZE);
				Message newMessage(tmp);
				int ret = newMessage.extract();
				LogPrinter::output("messageType : %d", newMessage.messageType);
				if (!ret
						|| ((newMessage.messageType == 0
								|| newMessage.messageType == 1)
								&& con && con->islogin())) {
					sprintf(buf,
							"an error had occured, the connection will close in 2 sec ");
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
					if (con != NULL) {
						delete con;
						con = NULL;
					}
					con = new User(user);
					sprintf(buf, "Please enter your password: User %s ",
							user.c_str());
					sendbuf = true;
					break;
				case LOG_IN_PASSWORD:
					password = newMessage.source;
					flag = con->check(password);
					if (!flag) {
						sprintf(buf, "password or username wrong  ");
						sendbuf = true;
					} else {
						sprintf(buf, "welcome back , %s ", user.c_str());
						sendbuf = true;
					}
					break;
				case GET_LIST:

					if (con && con->islogin()) {
						fid = con->getUserMessageList(ptr);
						js = ptr->opened[fid].get();
						ptr->releaseFile(fid);
						sendJ = true;

					} else {
						sprintf(buf, "you have not login ");
						sendbuf = true;
					}
					break;
				case GET_FILE: {
					if (con->islogin()) {
						const char* id = newMessage.source.c_str();
						fid = con->getUserMessageList(ptr);
						js = json::parse(ptr->opened[fid].get());
						ptr->releaseFile(fid);
						auto messageid = js["lists"];
						bool findId = false;
						for (int i = 0; i < messageid.size(); i++) {
							if (messageid[i] == id) {
								findId = true;
								break;
							}
						}
						if (!findId) {
							sprintf(buf, "wrong id messageid ");
							sendbuf = true;
						} else {
							fid = con->getUserMessage(id, ptr);
							js = json::parse(ptr->opened[fid].get());
							ptr->releaseFile(fid);
							sendJ = true;
						}
					}
					break;
				}
				case SEND_FILE: {
					fid = con->getUserMessageList(ptr);
					json js = json::parse(ptr->opened[fid].get());
					auto lists = js["lists"];
					vector<string> a;
					for (int i = 0; i < lists.size(); i++)
						a.push_back(lists[i]);
					json mes = json::parse(newMessage.source);
					string mid = generateMid(a);
					lists.push_back(mid);
					json now(lists);
					ptr->opened[fid].rewrite(now);
					ptr->releaseFile(fid);

					string s = json::parse(ptr->setting.get())["location"];
					string location = s + '/' + user + '/' + mid + ".json";
					fid = ptr->getAFile(location.c_str());
					ptr->opened[fid].rewrite(mes);
					ptr->releaseFile(fid);
					break;
				}
				default:
					break;
				}
				LogPrinter::output("the buf is :%s", buf);
			} else if (events[i].events & EPOLLRDHUP) {

			}
			//seperate the in and out.
			if (events[i].events & EPOLLOUT) {
				if (sendbuf) {
					int ret = safeSend(events[i].data.fd, buf);
					if (ret > 0)
						sendbuf = false;
				} else if (sendJ) {
					int ret = sendJson(events[i].data.fd, js);
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
	string path;
//	int xx=3;
//	char xxx='k';
//	char *xxxx ="1234";
//	LogPrinter::output("%d %c %s",xx,xxx,xxxx);

	bool fg = fGetCfgFileName(path);
	LogPrinter::output(path);
	addsig(SIGCHLD, sig_child);

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
	memoryData *pt = static_cast<memoryData*>(ptr);
	try {
		pt->setting.loadFile("./setting.json");
	} catch (FileException e) {
		LogPrinter::output(e.s);
	}
	static_cast<memoryData*>(ptr)->userList.loadFile(
			"./Database/userList.json");
	int epollfd = epoll_create(MAX_NUM_EPOLL_EVENTS);
	epoll_event events[MAX_NUM_EPOLL_NUM];
	assert(epollfd != -1);
	addfd(epollfd, newServer.sockfd, EPOLLIN | EPOLLET);
	LogPrinter::output("Server start succeed!Waiting for Connection");
	while (1) {
		int ret = epoll_wait(epollfd, events, MAX_NUM_EPOLL_NUM, -1);
		if (ret < 0) {
			LogPrinter::output("epoll failure");
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
					LogPrinter::output(
							"cannot create new process to handle new connection");
				} else if (pid != 0) {
					bindProandConn[connfd] = pid;
					close(connfd);
					// debug the child process.
					return 0;
				} else {
					work(connfd, pt);
					close(connfd);
					return 0;
				}
			}
		}

	}
	newServer.stop();
	return 0;
}

