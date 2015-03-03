#pragma once

#include "../commonLibrary.h"
#include "../memoryData.h"

#define DEBUG

class User {
private:
	string username = "";
	int login = 0;
	bool admin = false;
//	time_t lastlogin;
//	time_t userlast;
//	time_t lasttime;
public:
	User() {
	}
	User(string username) :
			username(username) {
	}
	void setlogin(int k) {
		login = k, admin = (k == 2);
	}
	void reset() {
		login = false, admin = false;
	}
	bool islogin() {
		return login;
	}
	bool isadmin() {
		return admin;
	}
	bool check(string password) {
		setlogin(1);
		return true; // unfinished;
	}
	string getUsername() {
		return username;
	}
	int getUserMessageList(memoryData *p) {
		LogPrinter::outputD("inside");
		memoryData* ptr = p;
		string s = ptr->setting.get();
		LogPrinter::outputD("the json is %s", s.c_str());
		json set = json::parse(s);
		string a = set["location"];
		LogPrinter::outputD("the location is %s", a.c_str());
		string location = a + '/' + username + "/messageList.json";
		LogPrinter::outputD("Trying open %s", location.c_str());
		return ptr->getAFile(location.c_str());
	}
	json getUserMessage(const char* UMid, void *p) {

	}
};
