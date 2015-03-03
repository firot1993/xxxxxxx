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
		opened[pos].clear();
		flag[pos] = false;
		pv(sem_id, 1);
	}
};
