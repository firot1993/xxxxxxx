#pragma once

#include "setting.h"
#include "data/data.h"
#include "commonLibrary.h"

struct memoryData {
	//please set a time_heap to auto_release File;
	File userList;
	File setting;
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
					opened[i].loadFile(filename);
					LogPrinter::outputD("finish load %s, fid = %d", filename,
							i);
					return i;
				} catch (FileException xx) {
					if (createnew){
						opened[i].createFile(filename);
						return i;
					}
					LogPrinter::outputD(xx.s);
					return -1;
				}
			}

	}
	void releaseFile(int pos) {
		opened[pos].clear();
		flag[pos] = false;
		pv(sem_id, 1);
	}
};
