#ifndef UTILS_H
#define UTILS_H

const char *get_filename_ext(const char *filename) {
	const char *dot = strrchr(filename, '.');
	if(!dot || dot == filename) return "";
	return dot + 1;
}

typedef struct socket_params {
	int client_sock;
	char buffer[1024];
	int numchars;
	int scheme;
	/*
		fifo = 0
		fork = 1
		thread = 2
		pre_thread = 3
	*/
	pid_t pid;
	/*
		browser = 0
		desktop = 1
	*/
	int origin;
} s_p;

#endif