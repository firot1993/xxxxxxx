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
//class FileOperate{
//public:
//	static bool isFolderexist(const char* zz){
//
//		return false;
//	}
//};
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
	void createFile(const char *filename) throw (FileException){
		filefd = open(filename, O_RDWR | O_CREAT);
		if (filefd == -1){
			throw FileException("can not create file");
		}
	}
	void loadFile(const char *filename) throw (FileException) {
		LogPrinter::outputD("start load %s ", filename);
		LogPrinter::outputD("waiting semphore to be zero in %s", __func__);
		pv(sem_id, 0);
		LogPrinter::outputD("finished waiting in %s", __func__);
		if (filefd != -1)
			close(filefd);
		name = filename;
		filefd = open(filename, O_RDWR);
		if (filefd == -1){
			throw FileException("no such file");
		}
		reload();
		LogPrinter::outputD("the load file is %s", file.c_str());
		LogPrinter::outputD("finished load %s", filename);
	}

	string get() {
		if (file == "") {
			reload();
		}
		return file;
	}

	void reload() {
		LogPrinter::outputD("waiting semphore in %s", __func__);
		pv(sem_id, 1);
		LogPrinter::outputD("get semphore in %s", __func__);
		string buf = "";
		char c[101];
		ssize_t readLen = 0;
		LogPrinter::outputD("the filefd is %d", filefd);
		while ((readLen = read(filefd, c, 100)) != 0) {
			for (unsigned i = 0; i < readLen; i++)
				buf += c[i];
		}
		LogPrinter::outputD("finish read data in %s", __func__);
		pv(sem_id, -1);
		LogPrinter::outputD("release semphore in %s", __func__);
		json tmp;
		bool ret = safe_parse(buf, tmp);
		if (ret)
			file = tmp.dump();
		else
			file = "";

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

