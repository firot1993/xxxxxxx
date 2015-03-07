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
	bool check(string password,sqlite3* db = NULL) {
		if (!db){
			string order = "select * from users where users.username="+'"'+username+'"';
			char **result;
			char *error;
			int row = 0;
			int col = 0;
			sqlite3_get_table(db,order.c_str(),&result,&row,&col,&error);
			for (int i = 0; i < row; i++)
				for (int j = 0; j< col;j++)
					cout<<result[i*col+j];
		}

		return true; // unfinished;
	}
	void createFolder(memoryData *p) {
		LogPrinter::outputD("ready for creating the folder for %s", username.c_str());
		json k;
		bool f = safe_parse(p->setting.get(),k);
		string locationhead = k["location"];
		string location = locationhead + '/' + username;
		LogPrinter::outputD("the path name is %s", username.c_str());
		if (access(location.c_str(), 6)) {
			string order = "mkdir " + location;
			LogPrinter::outputD("the order name is %s", order.c_str());
			system(order.c_str());
		}
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
		return ptr->getAFile(location.c_str(),true);
	}
	int getUserMessage(const char* UMid, void *p) {

	}
};
