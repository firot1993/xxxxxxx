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

connection_data *connection;
int cplink[65567];
int con_count = 0;
const char *memname = "sharedmemroy";
const size_t size = sizeof(memoryData);
int sig_pipe[2];

void sig_child(int signo) {
	pid_t pid;
	int stat;
	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
		;
	return;
}

void sig_handler(int sig) {
	int save_errno = errno;
	int msg = sig;
	send(sig_pipe[1], (char*) &msg, 1, 0);
	errno = save_errno;
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
								|| newMessage.messageType == 1) && con
								&& con->islogin())) {
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
					LogPrinter::outputD("------------LOG_IN_U--------------");
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
					LogPrinter::outputD("------------LOG_IN_P--------------");
					password = newMessage.source;
					flag = con->check(password);
					if (!flag) {
						sprintf(buf, "password or username wrong  ");
						sendbuf = true;
					} else {
						sprintf(buf, "welcome back , %s ", user.c_str());
						sendbuf = true;
						con->createFolder(ptr);
					}
					break;
				case GET_LIST: {
					LogPrinter::outputD("------------GET_LIST--------------");
					if (con && con->islogin()) {
						fid = con->getUserMessageList(ptr);
						bool ret = safe_parse(ptr->opened[fid].get(), js);
						ptr->releaseFile(fid);
						sendJ = ret;
						if (!ret) {
							sendbuf = true;
							sprintf(buf, "no new message ");
						}
					} else {
						sprintf(buf, "you have not login ");
						sendbuf = true;
					}
					break;
				}
				case GET_FILE: {
					LogPrinter::outputD("------------GET_FILE--------------");
					if (con->islogin()) {
						const char* id = newMessage.source.c_str();
						fid = con->getUserMessageList(ptr);
						if (fid == -1) {
							sendbuf = true;
							sprintf(buf, "can not get the list");
							break;
						}
						bool ret = safe_parse(ptr->opened[fid].get(), js);
						if (!ret) {
							sendbuf = true;
							sprintf(buf, "no new message");
							ptr->releaseFile(fid);
							break;
						}
						auto messageid = js["lists"];
						bool findId = false;
						for (unsigned i = 0; i < messageid.size(); i++) {
							if (messageid[i] == id) {
								findId = true;
								break;
							}
						}
						if (!findId) {
							sprintf(buf, "wrong id messageid ");
							sendbuf = true;
							ptr->releaseFile(fid);
							break;
						} else {
							int fid2 = con->getUserMessage(id, ptr);
							if (fid2 == -1) {
								sprintf(buf, "can not load message");
								sendbuf = true;
								json tmp;
								ret = safe_parse(ptr->opened[fid].get(), tmp);
								json g = tmp;
								if (ret) {
									g["lists"].clear();
									for (unsigned i = 0;
											i < tmp["lists"].size(); i++) {
										if (tmp["lists"][i] != id) {
											g["lists"].push_back(
													tmp["lists"][i]);
										}
									}
									ptr->opened[fid].rewrite(g);
								}
								ptr->releaseFile(fid);
								break;
							}
							ret = safe_parse(ptr->opened[fid].get(), js);
							if (ret) {
								ptr->releaseFile(fid);
								sendJ = true;
							} else {
								sendbuf = true;
								sprintf(buf, "can not load it");
							}
						}
					} else {
						sendbuf = true;
						sprintf(buf, "no new message");
					}

					break;
				}
				case SEND_FILE: {
					LogPrinter::outputD("------------SEND_FILE--------------");
					fid = con->getUserMessageList(ptr);
					json js, mes;
					bool ret;
					ret = safe_parse(ptr->opened[fid].get(), js);
//					auto lists = js["lists"];
					vector<string> a;
					for (unsigned i = 0; i < js["lists"].size(); i++)
						a.push_back(js["lists"][i]);

					ret = safe_parse(newMessage.source, mes);
					string mid = generateMid(a);
					js["lists"].push_back(mid);
//					json now(lists);
					ptr->opened[fid].rewrite(js);
					ptr->releaseFile(fid);

					json tmp;
					ret = safe_parse(ptr->setting.get(), tmp);
					string s = tmp["location"];
					string location = s + '/' + user + '/' + mid + ".json";
					fid = ptr->getAFile(location.c_str(), true);
					ptr->opened[fid].rewrite(mes);
					ptr->releaseFile(fid);
					break;
				}
				default:
					break;
				}
				if (sendbuf)
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

	int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, sig_pipe);
	bool running = true;
//	bool fg = fGetCfgFileName(path);
	LogPrinter::output(path);
	addsig(SIGCHLD, sig_handler);
	addsig(SIGTERM, sig_handler);
	addsig(SIGINT, sig_handler);
	addsig(SIGPIPE, SIG_IGN);

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
		pt->setting.loadFile("./setting.json", true, true);
		json tmp;
		ret = safe_parse(pt->setting.get(), tmp);
		if (ret) {
			int tt = tmp["maxconnection"];
			connection = new connection_data[tt];
		}
	} catch (FileException e) {
		LogPrinter::outputD(e.s);
		connection = new connection_data[100];
	}
	try {
		pt->userList.loadFile("./Database/userList.json", false, true);
	} catch (FileException e) {
		LogPrinter::outputD(e.s);
	}
	int epollfd = epoll_create(MAX_NUM_EPOLL_EVENTS);
	epoll_event events[MAX_NUM_EPOLL_NUM];
	assert(epollfd != -1);
	addfd(epollfd, newServer.sockfd, EPOLLIN | EPOLLET);
	setnonblocking(sig_pipe[1]);
	addfd(epollfd, sig_pipe[0], EPOLLIN | EPOLLET);

	LogPrinter::output("Server start succeed!Waiting for Connection");
	while (running) {
		int ret = epoll_wait(epollfd, events, MAX_NUM_EPOLL_NUM, -1);
		if (ret < 0 && errno != EINTR) {
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
					connection[con_count].address = client_address;
					connection[con_count].pid = pid;
					connection[con_count].connfd = connfd;
					cplink[pid] = con_count;
					con_count++;
					close(connfd);
					// debug the child process.
				} else {
					work(connfd, pt);
					close(connfd);
					return 0;
				}
			} else if ((sockfd == sig_pipe[0])
					&& (events[i].events & EPOLLIN)) {
				LogPrinter::outputD("signal coming");
				int sig;
				char signal[1024];
				ret = recv(sig_pipe[0], signal, sizeof(signal), 0);

				if (ret == -1) {
					continue;
				} else if (ret == 0) {
					continue;
				} else {
					for (int i = 0; i < ret; i++) {
						switch (signal[i]) {
						case SIGCHLD: {
							pid_t pid;
							int stat;
							while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
								int conid = cplink[pid];
								cplink[pid] = -1;
								connection[conid] = connection[con_count];
								cplink[conid] = connection[conid].pid;
								con_count--;
							}
							break;
						}
						case SIGINT:
						case SIGTERM:
							LogPrinter::output("Begin to clear connection");
							if (con_count == 0) {
								running = false;
								break;
							}
							for (int i = 0; i < con_count; i++) {
								int pid = connection[i].pid;
								kill(pid, SIGTERM);
							}
							running = false;
							break;
						default:
							break;
						}
					}
				}
			}
		}

	}
	LogPrinter::outputD("normal exists");
	newServer.stop();
	for (int i = 0; i < MAX_FILE_OPENED; i++) {
		pt->opened[i].release();
	}
	pt->setting.release();
	pt->userList.release();
	pt->release();
	munmap(ptr, size);
	shm_unlink(memname);
	return 0;
}

