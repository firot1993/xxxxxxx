#pragma once

#include "setting.h"
#include "data/data.h"
#include "commonLibrary.h"

struct userData {
	string username;
	string password;
};

struct memoryData {
private:
	int sem_id;
public:
	//please set a time_heap to auto_release File;
	File userList;
	File setting;
	File opened[MAX_FILE_OPENED];
	bool flag[MAX_FILE_OPENED];
	userData registerUser[100];
	bool fRegisterUser[100];
	userData database[1000];
	int connectfd[2];
	int userNum = 0;
	int tmpNum = 0;

	~memoryData() {
		LogPrinter::outputD("%s", __func__);
		semun sem_un;
		semctl(sem_id, 0, IPC_RMID, sem_un);
		semctl(sem_id, 1, IPC_RMID, sem_un);
	}
	memoryData() {
		LogPrinter::output("-----------%s---------", __func__);
		sem_id = semget(IPC_PRIVATE, 2, 0666);
		if (sem_id == -1)
			LogPrinter::outputD("errno %d", errno);
		LogPrinter::outputD("asking for semphor , and sem_id :%d", sem_id);
		semun sem_um;
		sem_um.val = MAX_FILE_OPENED;
		semctl(sem_id, 0, SETVAL, sem_um);
		sem_um.val = 100;
		semctl(sem_id, 1, SETVAL, sem_um);
		memset(flag, 0, sizeof(flag));
		memset(fRegisterUser, 0, sizeof(fRegisterUser));
	}
	int getAFile(const char* filename, bool createnew = false) {
		LogPrinter::outputD("start load %s", filename);
		for (int i = 0; i < MAX_FILE_OPENED; i++)
			if (opened[i].name == filename && flag[i]) {
				LogPrinter::outputD("finish load %s, fid = %d", filename, i);
				return i;
			}
		LogPrinter::outputD("waiting for semphore..in %s", __func__);
		pv(sem_id, -1);
		LogPrinter::outputD("get the semphore in %s..", __func__);
		for (int i = 0; i < MAX_FILE_OPENED; i++)
			if (!flag[i]) {
				flag[i] = true;
				try {
					opened[i].loadFile(filename, false, createnew);
					LogPrinter::outputD("finish load %s, fid = %d", filename,
							i);
					return i;
				} catch (FileException xx) {
					LogPrinter::outputD(xx.s);
					return -1;
				}
			}

	}
	void releaseFile(int pos) {
		try {
			opened[pos].clear();
			flag[pos] = false;
			pv(sem_id, 1);
		} catch (FileException e) {
			LogPrinter::outputD(e.s);
		}
	}
	void release() {
		semun sem_un;
		semctl(sem_id, 0, IPC_RMID, sem_un);
		semctl(sem_id, 1, IPC_RMID, sem_un);
		LogPrinter::outputD("release semid %d", sem_id);
	}
	bool registerNewUser(string username, string password) {
		pv(sem_id, -1, 1);
		for (int i = 0; i < userNum; i++)
			if (database[i].username == username)
				return false;
		for (int i = 0; i < 100; i++)
			if (!fRegisterUser[i]) {
				registerUser[i].username = username;
				registerUser[i].password = password;
				database[userNum] = registerUser[i];
				tmpNum++;
				userNum++;
				break;
			}

		if (tmpNum == 100) {
			int key = 1;
			send(connectfd[1], (char*) &key, 1, 0);
		}
		return true;
	}
	bool writeback(sqlite3* db) {
		if (db == NULL) {
			return false;
		}
		for (int i = 0; i < 100; i++) {
			if (fRegisterUser[i]) {
				//write back to db;
				char b[100];
				int tid = 0;
				snprintf(b, 100,
						"insert into _time (_date,_time) values (date(),time())");
				char * errmsg = 0;
				sqlite3_exec(db, b, NULL, 0, &errmsg);
				snprintf(b, 100, "select max(id) from _time");
				char ** tb;
				int row = 0;
				int col = 0;
				sqlite3_get_table(db,b,&tb,&row,&col,&errmsg);
//				tid = tb[col];
				snprintf(b, 100,
						"insert into users (username,password,registertime,logtime_id) "
						"values (\"%s\",\"%s\",date(),%d)",
						registerUser[i].username.c_str(), registerUser[i].password.c_str(),
						tid);
				sqlite3_exec(db,b,NULL,0,&errmsg);
				fRegisterUser[i] = false;
				sqlite3_free(errmsg);
				sqlite3_free_table(tb);
				pv(sem_id, 1, 1);
			}
		}
		return true;
	}
};

struct connection_data {
	sockaddr_in address;
	int connfd;
	pid_t pid;
	int pipefd[2];
};
