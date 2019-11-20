#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <queue>

#include <stdint.h>
#include <time.h>
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h> 
#include <linux/kdev_t.h>
#include <sys/poll.h>

#include "cpp-httplib/httplib.h"
#include "base64.h"

using namespace std;
using namespace httplib;

#define KEY_NAME "/dev/sdoor"

#define DOOR_AUTHFAIL	0x01
#define DOOR_DOORBELL	0x02
#define DOOR_OPEN		0x03
#define DOOR_CLOSE		0x04

#define DOOR_GETSERIAL	0x80

void upload_image(int key_fd) {
	/*
		get picture from driver
	*/
	uint32_t size;
	read(key_fd, &size, sizeof(size));

	uint8_t *data = (uint8_t *)malloc(size);
	if(!data) return;

	int len = read(key_fd, data, size);
	while(len < size) {
		if(size - len >= 1024)
			len += read(key_fd, data, 1024);
		else
			len += read(key_fd, data, size - len);
	}

	string b64data = base64_encode(data, size);

	/*
		upload image to server
	*/
	Client con("pic.nekop.kr", 80);

	int rnd = open("/dev/urandom", O_RDONLY);
	uint64_t urnd;
	read(rnd, &urnd, sizeof(urnd));
	close(rnd);

	stringstream ss;
	ss << hex << urnd;

	string filename = ss.str();
	MultipartFormDataItems items = {
		{ "device", "test-device", "", "text/plain" },
		{ "image", b64data, filename, "application/octet-stream" }
	};

	auto res = con.Post("/index.php", items);
}

void update_door_status(uint8_t type) {
	Client con("pic.nekop.kr", 80);

	MultipartFormDataItems items = {
		{ "device", "test-device", "", "text/plain" },
		{ "status", to_string(type), "", "text/plain" }
	};

	auto res = con.Post("/status.php", items);
}

static bool run = true;
static queue<uint8_t> msg;

void client() {
	int key_fd, retval;
	char key, led_onoff = 0;
	struct pollfd events[2];	// a variable to handle an event (switch on)

	key_fd = open(KEY_NAME, O_RDWR|O_NONBLOCK);	// KEY device open
	if (key_fd < 0)					// error handling
	{
		fprintf(stderr, "Device Open Error\n");
		return;
	}
	while (run)
	{
		events[0].fd = key_fd;	  // Event descriptor to check
		events[0].events = POLLIN;	  // Data read, Set the bit value (0x0001)
		retval = poll(events, 1, 1000);  // event waiting(Wait until switch on)
		if (retval < 0) {
			fprintf(stderr, "poll error\n");
			return;
		}

		if (retval == 0) { // no event
			if(msg.empty())
				continue;

			uint8_t cmd = msg.front();
			write(key_fd, &cmd, sizeof(uint8_t));
			
			msg.pop();
			sleep(1);
		}

		if (events[0].revents & POLLIN) { // event
			uint8_t type;
			read(key_fd, &type, sizeof(uint8_t));

			switch(type) {
			case DOOR_AUTHFAIL:
			case DOOR_DOORBELL:
				upload_image(key_fd);
				break;
			case DOOR_OPEN:
			case DOOR_CLOSE:
				update_door_status(type);
				break;
			}
		}
	}
	close(key_fd);
}

void heartbeat() {
	int s = socket(PF_INET, SOCK_STREAM, 0);
	
	struct sockaddr_in server_addr = {0, };
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("3.115.149.14");
	server_addr.sin_port = htons(4444);

	connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr));
	write(s, "\x00\x0btest-device", 13);

	while(run) {
		uint8_t cmd;
		read(s, &cmd, sizeof(cmd));
		msg.push(cmd);
	}
	close(s);
}

void sig_int(int signo) {
	run = false;
}

void init() {
	int i = 0;
    struct sigaction intsig;

    intsig.sa_handler = sig_int;
    sigemptyset(&intsig.sa_mask);
    intsig.sa_flags = 0;

    if(sigaction(SIGINT, &intsig, 0) == -1)
        exit(-1);
}

int main() {
	init();

	//thread t(client);
	thread t2(heartbeat);

	//t.join();
	t2.join();

	return 0;
}