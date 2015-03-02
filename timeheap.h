/*
 * timeheap.h
 *
 *  Created on: Mar 1, 2015
 *      Author: rrroen
 */

#include <iostream>
#include <netinet/in.h>
#include <time.h>

using std::exception;

#define BUFFER_SIZE 64

class heap_timer;

struct client_data {
	sockaddr_in address;
	int sockfd;
	char buf[BUFFER_SIZE];
	heap_timer* timer;
};

class heap_timer {
public:
	heap_timer(int delay) {
		expire = time( NULL) + delay;
	}
	time_t expire;
	void (*cb_func)(client_data*);
	client_data* user_data;
};

class time_heap {
public:
	time_heap(int cap) throw (std::exception) :
			capacity(cap), cur_size(0) {
		array = new heap_timer *[capacity];
		if (!array) {
			throw std::exception();
		}
		for (int i = 0; i < capacity; ++i) {
			array[i] = NULL;
		}
	}
	time_heap(heap_timer ** init_array, int size, int capacity)
			throw (std::exception) :
			cur_size(size), capacity(capacity) {
		if (capacity < size)
			throw std::exception();
		array = new heap_timer *[capacity];
		if (!array)
			throw std::exception();
		for (int i = 0; i < capacity; i++)
			array[i] = NULL;
		if (size != 0) {
			for (int i = 0; i < capacity; i++)
				array[i] = init_array[i];
			//heapify
		}
	}
	~time_heap() {
		for (int i = 0; i < cur_size; i++)
			delete array[i];
		delete[] array;
	}
	void add_timer(heap_timer * timer) throw (std::exception) {
		if (!timer)
			return;
		if (cur_size >= capacity)
			resize();
		int hole = cur_size++;
		int parent = 0;
		for (; hole > 0; hole = parent) {
			parent = (hole - 1) / 2;
			if (array[parent]->expire <= timer->expire)
				break;
			array[hole] = array[parent];
		}
		array[hole] = timer;
	}
	void del_timer(heap_timer* timer) {
		if (!timer) {
			return;
		}
		timer->cb_func = NULL;
	}
	heap_timer* top() const {
		if (empty())
			return NULL;
		return array[0];
	}
	void pop_timer() {
		if (empty())
			return;
		if (array[0]) {
			delete array[0];
			array[0] = array[--cur_size];
			heapify(0);
		}
	}
	void tick() {
		heap_timer* tmp = array[0];
		time_t cur = time(NULL);
		while (!empty()) {
			if (!tmp)
				break;
			if (tmp->expire > cur)
				break;
			if (array[0]->cb_func)
				array[0]->cb_func(array[0]->user_data);
			pop_timer();
			tmp = array[0];
		}
	}
	void resize() throw (std::exception) {
		heap_timer ** tmp = new heap_timer*[2 * capacity];
		for (int i = 0; i < 2 * capacity; i++)
			tmp[i] = NULL;
		if (!tmp)
			throw std::exception();
		capacity = 2 * capacity;
		for (int i = 0; i < cur_size; i++) {
			tmp[i] = array[i];
		}
		delete[] array;
		array = tmp;
	}
	bool empty() const {
		return cur_size == 0;
	}
	void heapify(int pos) {
		heap_timer * tmp = array[pos];
		int child = 0;
		for (; ((pos * 2 + 1) <= (cur_size - 1)); pos = child) {
			child = pos * 2 + 1;
			if ((child < (cur_size - 1)
					&& (array[child + 1]->expire < array[child]->expire))) {
				child++;
			}
			if (array[child]->expire < tmp->expire) {
				array[pos] = array[child];
			} else
				break;

		}
		array[pos] = tmp;
	}

private:
	heap_timer** array;
	int capacity ;
	int cur_size ;
};
