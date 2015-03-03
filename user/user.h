#pragma once

#include "../commonLibrary.h"
#include "../memoryData.h"

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
	int getUserMessageList(void *p) {
		memoryData* ptr = static_cast<memoryData*>(p);
		string s = ptr->setting.get();
		json set = json::parse(s);
		string a = set["location"];
		string location = a + '/' + username +"/messageList.json";
		return ptr->getAFile(location.c_str());
	}
	json getUserMessage(const char* UMid, void *p) {

	}
};
