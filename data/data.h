#pragma once
#include "../commonLibrary.h"
#include <iostream>

using std::exception;

class FileException: public exception {
public:
	string s;
	FileException() {
	}
	FileException(string s) :
			s(s) {
	}
};
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
	void loadFile(const char *filename) throw (FileException) {
		pv(sem_id, 0);
		if (filefd != -1)
			close(filefd);
		name = filename;
		filefd = open(filename, O_RDWR);
		if (filefd == -1) {
			filefd = open(filename, O_RDWR | O_CREAT);
		}
		reload();
	}
	string get() {
		if (file == "") {
			reload();
		}
		return file;
	}
	void reload() {
		pv(sem_id, 1);
		string buf = "";
		char c[101];
		ssize_t readLen = 0;
		while ((readLen = read(filefd, c, 100)) != 0) {
			for (unsigned i = 0; i < readLen; i++)
				buf += c[i];
		}
		pv(sem_id, -1);
		json tmp = safe_parse(buf);
		file = tmp.dump();

	}

	void rewrite(json j) throw (FileException) {
		pv(sem_id, 0);
		string buf = j.dump();
		char s[buf.length() + 1];
		sprintf(s, "%s", buf.c_str());
		ssize_t writeLen = 0;
		writeLen = write(filefd, s, strlen(s));
		if (writeLen < buf.length() - 1)
			throw FileException("Write file exception");
	}

	void clear() throw (FileException) {
		pv(sem_id, 0);
		close(filefd);
		filefd = -1;
	}

	~File() {
		if (filefd != -1)
			close(filefd);
	}

};

