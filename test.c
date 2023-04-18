#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>

int main() {
    char* path = "/Volumes/ThumbDrive1/apple.txt";
    char drive1[] = "/Volumes/ThumbDrive1/";
    char drive2[] = "/Volumes/ThumbDrive2/";

    if(access(drive1, F_OK) != -1) {
        printf("Drive 1 found\n");
    }

    if(access(drive2, F_OK) != -1) {
        printf("Drive 2 found\n");
    }

    return 0;
}