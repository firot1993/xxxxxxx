#pragma once

#include "setting.h"
#include "data/data.h"
#include "commonLibrary.h"

struct memoryData {
private:
	int sem_id;
public:
	//please set a time_heap to auto_release File;
	File userList;
	File setting;
	File opened[MAX_FILE_OPENED];
	bool flag[MAX_FILE_OPENED];

	~memoryData() {
		LogPrinter::outputD("%s",__func__);
		semun sem_un;
		semctl(sem_id, 0, IPC_RMID, sem_un);
	}
	memoryData() {
		LogPrinter::output("-----------%s---------",__func__);
		sem_id = semget(IPC_PRIVATE, 1, 0666);
		if (sem_id == -1)
			LogPrinter::outputD("errno %d", errno);
		LogPrinter::outputD("asking for semphor , and sem_id :%d", sem_id);
		semun sem_um;
		sem_um.val = MAX_FILE_OPENED;
		semctl(sem_id, 0, SETVAL, sem_um);
		memset(flag, 0, sizeof(flag));
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
		LogPrinter::outputD("release semid %d",sem_id);
	}
};

struct connection_data{
	sockaddr_in address;
	int connfd;
	pid_t pid;
	int pipefd[2];
};
