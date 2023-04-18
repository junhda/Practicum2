/*
 * server.c -- TCP Socket Server
 * 
 * adapted from: 
 *   https://www.educative.io/answers/how-to-implement-tcp-sockets-in-c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#define errExit(msg) do { perror(msg); exit(EXIT_FAILURE); \
                               } while (0)

#define CONFIG "./config.txt"
#define MAX_CHARACTERS 8196

static char drive1[1024], drive2[1024], server_port[20], server_ip_address[100];

static void sigintHandler(int sig)
{
    write(STDERR_FILENO, "Caught SIGINT!\n", 15);
}

void get(char* command, char* server_message) {
  // initialize response
  char response[MAX_CHARACTERS];
  memset(response, '\0', sizeof(response));

  // extract arguments
  // strings[1] specifies remote-file-path
  char* strings[2]; 
  int j = 0;
  for(int i = 0; i < sizeof(strings); i++) {
    strings[i] = NULL;
  }
  char* token = strtok(command, " ");
  while(token != NULL) {
    strings[j++] = token;
    token = strtok(NULL, " ");
  }

  // check if remote path argument provided
  if(strings[1] == NULL){
    printf("Client did not provide path.\n");
    char resp[] = "GET command expects remote file path input.";
    strcpy(server_message, resp);
    return;
  } else {
    // open remote path, read data into response
    char path[2000];
    
    // check for available drive
    // printf("Drive 1: %s\n", drive1);
    // printf("Drive 2: %s\n", drive2);
    if(access(drive1, F_OK) != -1) {
      strcpy(path, drive1);
    } else if(access(drive2, F_OK) != -1) {
      strcpy(path, drive2);
    } else {
      printf("Drives are unavailable.\n");
      char resp[] = "Drives 1 and 2 are down. Please try again later.";
      strcpy(server_message, resp);
      return;
    }
    
    // open file on server
    strcat(path, strings[1]);
    FILE* fp = fopen(path, "r");
    char buffer[1024];
    // printf("File opened\n");

    // read file into response if file exists
    if(!fp) {
      char resp[] = "Remote file not found";
      strcpy(server_message, resp);
      return;
    } else {
      while(fgets(buffer, sizeof(buffer), fp)) {
        strcat(response, buffer);
      }
    }

    // close the file
    fclose(fp);
  }

  // set server response
  strcpy(server_message, response);
}

void info(char* command, char* server_message) {
  // initialize response
  char response[MAX_CHARACTERS];
  memset(response, '\0', sizeof(response));

  // extract arguments
  // strings[1] specifies remote-file-path
  char* strings[2]; 
  int j = 0;
  for(int i = 0; i < sizeof(strings); i++) {
    strings[i] = NULL;
  }
  char* token = strtok(command, " ");
  while(token != NULL) {
    strings[j++] = token;
    token = strtok(NULL, " ");
  }

  // check if remote path argument provided
  if(strings[1] == NULL){
    printf("Client did not provide path.\n");
    char resp[] = "INFO command expects remote file path input.";
    strcpy(server_message, resp);
    return;
  } else {
    // create full remote path
    char path[2000];
    
    // check for available drive
    // printf("Drive 1: %s\n", drive1);
    // printf("Drive 2: %s\n", drive2);
    if(access(drive1, F_OK) != -1) {
      strcpy(path, drive1);
    } else if(access(drive2, F_OK) != -1) {
      strcpy(path, drive2);
    } else {
      printf("Drives are unavailable.\n");
      char resp[] = "Drives 1 and 2 are down. Please try again later.";
      strcpy(server_message, resp);
      return;
    }
    strcat(path, strings[1]);

    // get stats of the path
    struct stat fileStat;
    if (stat(path, &fileStat) < 0) {
        perror("stat");
        return;
    }

    // add stat values to the server_message;
    char size[20];
    sprintf(size, "%ld", (long)fileStat.st_size);
    char uid[20];
    sprintf(uid, "%d", fileStat.st_uid);
    char gid[20];
    sprintf(gid, "%d", fileStat.st_gid);
    char permission[20];
    sprintf(permission, "%o", fileStat.st_mode);

    strcpy(response, "File Information:\nFile name: ");
    strcat(response, path);
    strcat(response, "\nSize: ");
    strcat(response, size);
    strcat(response, " bytes\nOwner: ");
    strcat(response, uid);
    strcat(response, "\nGroup: ");
    strcat(response, gid);
    strcat(response, "\nPermissions: ");
    strcat(response, permission);
    strcat(response, "\nLast modified: ");
    strcat(response, ctime(&fileStat.st_mtime));
  }
  
  // set server response
  strcpy(server_message, response);
}

void md(char* command, char* server_message) {
  // initialize response
  char response[MAX_CHARACTERS];
  memset(response, '\0', sizeof(response));

  // extract arguments
  // strings[1] specifies remote-file-path
  char* strings[2]; 
  int j = 0;
  for(int i = 0; i < sizeof(strings); i++) {
    strings[i] = NULL;
  }
  char* token = strtok(command, " ");
  while(token != NULL) {
    strings[j++] = token;
    token = strtok(NULL, " ");
  }

  // check if remote path argument provided
  if(strings[1] == NULL){
    printf("Client did not provide path.\n");
    char resp[] = "INFO command expects remote file path input.";
    strcpy(server_message, resp);
  } else if(strrchr(strings[1], '.') != NULL){
    // checks if path specifies creating a new file
    printf("Client provided an invalid path.\n");
    char resp[] = "Invalid path specified.\n";
    strcpy(server_message, resp);
  } else {
    // create new directory in available drives
    int drive1Update = 0, drive2Update = 0;
    char path[2000];

    // create directory in drive 1
    if(access(drive1, F_OK) != -1) {
      strcpy(path, drive1);
      strcat(path, strings[1]);

      // check if directory already exists
      DIR* dir = opendir(path);
      if(dir) {
        closedir(dir);
        printf("Directory already exists: %s\n", path);
        drive1Update = -1;
      } else {
        // closedir(dir);
        printf("Creating new directory %s\n", path);
        if(mkdir(path, 0777) == -1) {
          printf("Failed to create directory %s\n", path);
          drive1Update = -2;
        } else {
          printf("Drive 1 updated successfully.\n");
          drive1Update = 1;
        }
      }
    }

    printf("Drive 1 check complete\n");
    
    // create directory in drive 2
    if(access(drive2, F_OK) != -1) {
      strcpy(path, drive2);
      strcat(path, strings[1]);

      // check if directory already exists
      DIR* dir = opendir(path);
      if(dir) {
        closedir(dir);
        printf("Directory already exists: %s\n", path);
        drive2Update = -1;
      } else {
        printf("Creating new directory %s\n", path);
        if(mkdir(path, 0777) == -1) {
          printf("Failed to create directory %s\n", path);
          drive2Update = -2;
        } else {
          printf("Drive 2 updated successfully.\n");
          drive2Update = 1;
        }
      }
    }

    printf("Drive 2 check complete\n");
    
    // create response for successful / unsuccessful updates
    if(drive1Update == 0 && drive2Update == 0){
      printf("Drives are unavailable.\n");
      strcpy(response, "Drives 1 and 2 are down. Please try again later.");
    } else {
      if(drive1Update == 1) {
        strcat(response, strings[1]);
        strcat(response, " created successfully.\n");
      } else if(drive1Update == -1) {
        strcat(response, "MD command attempted to create an existing directory.\n");
      } else if(drive1Update == -2) {
        strcat(response, "MD command failed.\n");
      } else if(drive1Update == 0) {
        strcat(response, "Drive 1 not found.\n");
      }

      if(drive2Update == 1) {
        strcat(response, strings[1]);
        strcat(response, " created successfully.\n");
      } else if(drive2Update == -1) {
        strcat(response, "MD command attempted to create an existing directory.\n");
      } else if(drive2Update == -2) {
        strcat(response, "MD command failed.\n");
      } else if(drive2Update == 0) {
        strcat(response, "Drive 2 not found.\n");
      }
    }

    // send server_message
    strcpy(server_message, response);
    // printf("Server Response: %s", server_message);
  }

  // printf("Server Response MD End: %s", server_message);
}

void put(char* command, char* server_message) {
  // initialize response
  char response[MAX_CHARACTERS];
  memset(response, '\0', sizeof(response));

  // extract arguments
  // strings[1] specifies remote-file-path
  char* strings[2]; 
  int j = 0;
  for(int i = 0; i < sizeof(strings); i++) {
    strings[i] = NULL;
  }
  char* token = strtok(command, " ");
  while(token != NULL) {
    strings[j++] = token;
    token = strtok(NULL, " ");
  }
}

void rm(char* command, char* server_message) {
  // initialize response
  char response[MAX_CHARACTERS];
  memset(response, '\0', sizeof(response));

  // extract arguments
  // strings[1] specifies remote-file-path
  char* strings[2]; 
  int j = 0;
  for(int i = 0; i < sizeof(strings); i++) {
    strings[i] = NULL;
  }
  char* token = strtok(command, " ");
  while(token != NULL) {
    strings[j++] = token;
    token = strtok(NULL, " ");
  }
}

void *drive_sync(void *arg) {
  int drive1_flag = 0;
  int drive2_flag = 0;
  while(1 == 1) {
    // check that Drive 1 is connected
    if(access(drive1, F_OK) != -1) {
      drive1_flag = 1;
      printf("Drive 1 is connected\n");
    } else {
      printf("Drive 1 is not connected\n");
      drive1_flag = 0;
    }

    // check that Drive 2 is connected
    if(access(drive2, F_OK) != -1) {
      drive2_flag = 1;
      printf("Drive 2 is connected\n");
    } else {
      printf("Drive 2 is not connected\n");
      drive2_flag = 0;
    }

    sleep(5);
  }
}

void initialize_config() {
  // read config variables
  FILE* f = fopen(CONFIG, "r");
  if(!f) {
    perror("Failed to open config file");
    exit(1);
  } else {
    char buffer[1024];
    while(fgets(buffer, sizeof(buffer), f)) {
      // printf("Line: %s\n", buffer);
      // get variable assignments from each line of the config.txt file
      buffer[strcspn(buffer, "\n")] = '\0';
      char *key, *value;
      key = strtok(buffer, "=");
      value = strtok(NULL, "=");

      // printf("Variable: %s\n", key);
      // printf("Value: %s\n", value);
      
      if(strcmp(key, "drive1") == 0) {
        strcpy(drive1, value);
      } else if(strcmp(key, "drive2") == 0) {
        strcpy(drive2, value);
      } else if(strcmp(key, "port") == 0) {
        strcpy(server_port, value);
      } else if(strcmp(key, "ip_address") == 0) {
        strcpy(server_ip_address, value);
      } 
    }
  }

  fclose(f);
}

int main(void)
{
  // initialize config variables
  initialize_config();
  // printf("\n");
  // printf("Drive 1: %s\n", drive1);
  // printf("Drive 2: %s\n", drive2);
  // printf("Port: %s\n", server_port);
  // printf("Server IP Address: %s\n", server_ip_address);

  int port = atoi(server_port);

  // initialize variables
  int socket_desc, client_sock;
  socklen_t client_size;
  struct sockaddr_in server_addr, client_addr;
  char server_message[MAX_CHARACTERS], client_message[MAX_CHARACTERS];
  // char* server_message = (char*)malloc(sizeof(char) * MAX_CHARACTERS);
  // char* client_message = (char*)malloc(sizeof(char) * MAX_CHARACTERS);

  // // sigint handler registration
  // // if (signal(SIGINT, sigintHandler) == SIG_ERR) {
  // //   errExit("signal SIGINT");
  // // }

  // // create background thread for syncing USB drives
  pthread_t bt;
  // // pthread_create(&bt, NULL, drive_sync, NULL);
  // // if(!bt) {
  // //   printf("Error: Unable to create thread\n");
  // //   return -1;
  // // }
  // // pthread_join(bt, NULL);
  
  // Clean buffers:
  memset(server_message, '\0', sizeof(server_message));
  memset(client_message, '\0', sizeof(client_message));

  // Create socket:
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  
  if(socket_desc < 0){
    printf("Error while creating socket\n");
    return -1;
  }
  printf("Socket created successfully\n");
  
  // Set port and IP:
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = inet_addr(server_ip_address);
  
  // Bind to the set port and IP:
  if(bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr))<0){
    printf("Couldn't bind to the port\n");
    return -1;
  }
  printf("Done with binding\n");
  
  // Listen for clients:
  if(listen(socket_desc, 1) < 0){
    printf("Error while listening\n");
    return -1;
  }
  printf("\nListening for incoming connections.....\n");
  
  // Accept an incoming connection:
  client_size = sizeof(client_addr);
  client_sock = accept(socket_desc, (struct sockaddr*)&client_addr, &client_size);
  
  if (client_sock < 0){
    printf("Can't accept\n");
    return -1;
  }
  printf("Client connected at IP: %s and port: %i\n", 
         inet_ntoa(client_addr.sin_addr), 
         ntohs(client_addr.sin_port));
  
  // Receive client's command:
  if (recv(client_sock, client_message, 
           sizeof(client_message), 0) < 0){
    printf("Couldn't receive\n");
    return -1;
  }
  printf("Msg from client: %s\n", client_message);

  // Parse client command
  char message_copy[MAX_CHARACTERS];
  strcpy(message_copy, client_message);
  char* token = strtok(message_copy, " ");  // first command word from the client. this should define the command type

  // call different function based on command type
  if(strcmp(token, "GET") == 0) { // case when GET command called
    printf("GET command received\n");
    get(client_message, server_message);
  } else if(strcmp(token, "INFO") == 0) {
    printf("INFO command received\n");
    info(client_message, server_message);
  } else if(strcmp(token, "MD") == 0) {
    printf("MD command received\n");
    md(client_message, server_message);
  } else if(strcmp(token, "PUT") == 0) {
    printf("PUT command received\n");
    put(client_message, server_message);
  } else if(strcmp(token, "RM") == 0) {
    printf("RM command received\n");
    rm(client_message, server_message);
  } else {
    strcpy(server_message, token);
    strcat(server_message, " is not a valid command.\n");
  }

  printf("Server message: %s\n", server_message);
  // Respond to client:
  if (send(client_sock, server_message, strlen(server_message), 0) < 0){
    printf("Can't send\n");
    return -1;
  }
  
  // Cleanup
  // pthread_exit((void*)&bt);
  close(client_sock);
  close(socket_desc);
  
  return 0;
}
