# WebServerInC

Project part of Principles of Operating Systems (IC6600) course. That has the aim to learn about client-server scheme, sockets, manage of process and threads, develop in C.

## Developers

Adrián Álvarez Calderón [@adalvarez](https://github.com/adalvarez)

Michael Choque Núñez [@Feymus](https://github.com/Feymus)

---

## What is it?

Its about a project of the course Principles of Operating Systems (IC6600) in Technological Institute of Costa Rica that wants to develop a web-server (just response static files).

#### Objectives of the project
* Learn to develop a client-server scheme.
* Know the communication through sockets.
* Develop a program that manages processes.
* Develop a program where threads are used.
* Develop a program in C to be run in Linux environment.

This first project aims to develop different server schemes. The servers to be developed will respond to file requests that the clients will make. The client can be any browser or the developed client. The difference between both clients is that the Browser when making the request to the server shows the received file, instead the client saves the file in a folder of the disk of the client machine. It is a very careful project in terms of closing processes, threads and sockets.

#### Detailed Schemas
* **FIFO**: Response the requests as they arrive. There is only one sequential process, no action is executed in parallel. So the server will attend the requests one by one.
* **FORK**: This server will receive the requests and for each new request it will create a new process to attend it.
* **THREAD**: This server will receive the requests and for each new request it will create a new thread that attends it.
* **PRE-THREAD**: This server will receive the requests and for each new request will distribute it among the N threads that it must already have created.

#### Requirements

* [Make](https://www.gnu.org/software/make/)
* [GCC](https://gcc.gnu.org/)

---

## Usage

In case that you do not have the requirements, you can run the install file. (**Debian/Ubuntu**)
`$ chmod +x install.sh && ./install.sh`

Make the binary files.
`$ make`

* `$ make main`: Clean the binary files and recompile.
* `$ make server`: Compile the server.
* `$ make client`: Compile the client.
* `$ make clear`: Remove the binary files.

#### Server

Run

> `$ ./server [options]`

Options:
* -fifo or -FIFO
* -fork or -FORK
* -thread or -THREAD
* -pre-thread or -PRE-THREAD: In this case the server ask you for the number of threads to create.

#### Client

Run

> `$ ./client`

It ask you for a file or files to search. Avoid white spaces, and in case that you need several files use commas, for example:
`Files to search: lion.jpg,txt`