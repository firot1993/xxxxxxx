/*
 * message.h
 *
 *  Created on: Feb 28, 2015
 *      Author: rrroen
 */

#ifndef SERVER_MESSAGE_MESSAGE_H_
#define SERVER_MESSAGE_MESSAGE_H_

#include "messagetype.h"
#include <cstdio>
#include <string>
#include <sstream>

using namespace std;


class Message{
public:
	string source;
	int messageType = -1;
	Message(string h):source(h){
	}
	// split the source and the headerType.
	bool extract(){
		printf("extract begin\n");
		stringstream buf;
		buf << source;
		char Type;
		buf >> Type;
		messageType = Type - 'a';
		buf >> source;
		printf("MessageType: %d  ",messageType);
		printf("MessageString: %s\n",source.c_str());
		printf("extract finished\n");
		if (messageType < 0) return false;
		return true;
	}

};




#endif /* SERVER_MESSAGE_MESSAGE_H_ */
