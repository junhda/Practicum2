# Practicum 2 - File Systen

This project is a file system where multiple clients can connect to the server and perform basic file operations such as GET, INFO, MD, PUT, and RM on files or directories located on the server.

## System Requirements
* Operating System: Linux or Mac OS X
* Libraries: pthread, sys/socket.h, netinet/in.h, sys/stat.h, sys/types.h, fcntl.h, arpa/inet.h, signal.h, dirent.h
* Hardware: two USB drives mounted on the server machine

## Installation
1. Clone the repository using the following command: git clone https://github.com/junhda/Practicum2.git
2. Navigate to the project directory
3. Compile code using command: 'make all'
4. Update config.txt file
    1. drive1: path to flash drive 1
    2. drive2: path to flash drive 2
    3. port: unique port for TCP/IP connection (try a port > 10000)
    4. ip_address: public ip address of server

## Usage 
1. Insert two USB drives into the server machine
2. Nagicate to the project directory on both server and client machines
3. Start the server on server machine using command './fserver'
4. Start the client program on a client machine using command './fget <client_command>'. Replace <client_command> with the appropriate command:
    1. GET <remote-file-path>               // Retrieve contents of file at remote-file-path
    2. INFO <remote-file-path>              // Get information about file at remote-file-path
    3. MD <remote-dir-path>                 // Create new directory at remote-dir-path
    4. PUT <remote-file-path> <string-data> // Create new file at remote-file-path with contents string-data
    5.RM <remote-file-path>                // Remove file or directory at remote-file-path
5. Server will stop in 2 cases: SIGINT signal detected (Ctrl + C) or the Max number of clients have connected to the server. The default max is 1000, and can be updated in server.c

## Project Architecture
* client.c: This file contains the implementation of the client that connects to the server to perform various file operations.
* server.c: This file contains the implementation of the server that listens for incoming client connections and serves their requests. Each client request will be processed in a separate thread.
* sync_folder.h: This file contains the function prototype of sync_folders().
* config.txt: This file contains the configuration parameters for the system, such as the mount point of the two USB drives, the IP address of the server, and the port number on which the server listens.

## Testing
1. Set Flash Drives 1 and 2 to be in sync with each other (ie empty both drives)
2. Add a file 'example1.txt' into both flash drives with the same data inside
3. On a client machine, run 'bash test.sh', which will run a series of tests for each client command type.
    1. try testing with both flash drives connected to server
    2. try testing with flash drive 1 connected flash drive 2 disconnected to server. Then reconnect flash drive 2
        1. After 1 - 5 seconds, flash drive 2 should be synced to the state of flash drive 1
    3. try testing with flash drive 2 connected flash drive 1 disconnected to server. Then reconnect flash drive 1.
        1. After 1 - 5 seconds, flash drive 1 should be synced to the state of flash drive 1

## Project Contributors
* Daniel Jun