#pragma once
#include "../commonLibrary.h"
#include <iostream>

using std::exception;

class File {
private:
	int filefd = -1;
	int sem_id;
//	json file;
	string file = "";
public:
	string name;
	File() {
		sem_id = semget(IPC_PRIVATE, 1, 0666);
		union semun sem_un;
		sem_un.val = 0;
		semctl(sem_id, 0, SETVAL, sem_un);
	}
	void loadFile(const char *filename) throw (exception) {
		pv(sem_id, 0);
		if (filefd != -1)
			close(filefd);
		name = filename;
		filefd = open(filename, O_RDWR);
		if (filefd == -1) {
			throw exception();
		} else {
			reload();
		}
	}
	string get(){
		if (file == ""){
			reload();
		}
		return file;
	}
	void reload() {
		pv(sem_id, 1);
		string buf = "";
		char *c;
		ssize_t readLen = 0;
		while ((readLen = read(filefd, c, 100)) != 0) {
			for (unsigned i = 0; i < strlen(c); i++)
				buf += c[i];
		}
		pv(sem_id, -1);
		json tmp = json::parse(buf);
		file = tmp.dump();
	}

	void rewrite(json j) throw (exception) {
		pv(sem_id, 0);
		string buf = j.dump();
		char s[buf.length() + 1];
		sprintf(s, "%s", buf.c_str());
		ssize_t writeLen = 0;
		writeLen = write(filefd, s, strlen(s));
		if (writeLen < buf.length() - 1)
			throw exception();
	}

	void clear() throw (exception) {
		pv(sem_id, 0);
		close(filefd);
		filefd = -1;
	}

	~File() {
		if (filefd != -1)
			close(filefd);
	}

};

