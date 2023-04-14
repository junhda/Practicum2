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

#define errExit(msg) do { perror(msg); exit(EXIT_FAILURE); \
                               } while (0)

#define DRIVE1 "./drive1/"
#define DRIVE2 "./drive2/"
#define LOG_FILE "./log.txt"
#define MAX_CHARACTERS 8196

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
    if(access(DRIVE1, F_OK) != -1) {
      strcpy(path, DRIVE1);
    } else if(access(DRIVE2, F_OK) != -1) {
      strcpy(path, DRIVE2);
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

  // return response
  strcpy(server_message, response);
}

char* info(char* remote_path) {
  char* ans = remote_path;
  return ans;
}

char* md(char* remote_path) {
  char* ans = remote_path;
  return ans;
}

char* put(char* remote_path, char* local_path) {
  char* ans = remote_path;
  strcat(ans, "\n");
  strcat(ans, local_path);
  return ans;
}

char* rm(char* remote_path) {
  char* ans = remote_path;
  return ans;
}

void *drive_sync(void *arg) {
  int drive1_flag = 0;
  int drive2_flag = 0;
  FILE* log = (FILE*)arg;
  while(1 == 1) {
    // check that Drive 1 is connected
    if(access(DRIVE1, F_OK) != -1) {
      if(drive1_flag == 0) {
        drive1_flag = 1;
        // sync drive1 with log
      }
      printf("Drive 1 is connected\n");
    } else {
      printf("Drive 1 is not connected\n");
      drive1_flag = 0;
    }

    // check that Drive 2 is connected
    if(access(DRIVE2, F_OK) != -1) {
      if(drive2_flag == 0) {
        drive2_flag = 1;
        // sync drive2 with log
      }
      printf("Drive 2 is connected\n");
    } else {
      printf("Drive 2 is not connected\n");
      drive2_flag = 0;
    }

    sleep(5);
  }
}

int main(void)
{
  // read environment variables
  int port = 2000;//atoi(getenv("PORT"));
  char* ip_addr = "127.0.0.1"; //getenv("IP_ADDRESS");

  // initialize variables
  int socket_desc, client_sock;
  socklen_t client_size;
  struct sockaddr_in server_addr, client_addr;
  // char* client_message = (char*)malloc(sizeof(char) * MAX_CHARACTERS);
  char server_message[MAX_CHARACTERS], client_message[MAX_CHARACTERS];

  // Initialize journaling log file
  FILE* log = fopen(LOG_FILE, "wb+");
  if(!log) {
      perror("Failed to open disk file");
      exit(1);
  }

  // sigint handler registration
  // if (signal(SIGINT, sigintHandler) == SIG_ERR) {
  //   errExit("signal SIGINT");
  // }

  // create background thread for syncing USB drives
  pthread_t bt;
  // pthread_create(&bt, NULL, drive_sync, (void*)log);
  // if(!bt) {
  //   printf("Error: Unable to create thread\n");
  //   return -1;
  // }
  // pthread_join(bt, NULL);
  
  // Clean buffers:
  memset(server_message, '\0', sizeof(server_message));
  memset(client_message, '\0', sizeof(server_message));
  
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
  server_addr.sin_addr.s_addr = inet_addr(ip_addr);
  
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
    get(client_message, server_message);
  }
  // } else if(strcmp(token, "INFO") == 0) {
  //   printf("INFO command received:\n");
  //   // extract arguments
  //   char* arg1 = strtok(client_message, " "); // first argument from the client. Specifies remote-file-path

  //   if(arg1 == NULL) {
  //     // check if valid arguments provided
  //     strcpy(server_message, "INFO command expects remote file path input.");
  //   } else {
  //     // run INFO function
  //     strcpy(server_message, info(arg1));
  //   }
  // } else if(strcmp(token, "MD") == 0) {
  //   printf("MD command received:\n");
  //   // extract arguments
  //   char* arg1 = strtok(client_message, " "); // first argument from the client. Specifies remote-file-path

  //   if(arg1 == NULL) {
  //     // check if valid arguments provided
  //     strcpy(server_message, "MD command expects remote file path input.");
  //   } else {
  //     // run MD function
  //     strcpy(server_message, md(arg1));
  //   }
  // } else if(strcmp(token, "PUT") == 0) {
  //   printf("PUT command received:\n");
  //   // extract arguments
  //   char* arg1 = strtok(client_message, " "); // first argument from the client. Specifies remote-file-path
  //   char arg2[MAX_CHARACTERS]; // second argument from the client. should include all string data to PUT into new file
  //   strncpy(arg2, client_message + strlen(token) + 1 + strlen(arg1) + 1, strlen(client_message) - (strlen(token) + 1 + strlen(arg1) + 1));

  //   if(arg1 == NULL) {
  //     // check if valid arguments provided
  //     strcpy(server_message, "PUT command expects remote file path input.");
  //   } else {
  //     // run PUT function
  //     strcpy(server_message, put(arg1, arg2));
  //   }
  // } else if(strcmp(token, "RM") == 0) {
  //   printf("RM command received:\n");
  //   // extract arguments
  //   char* arg1 = strtok(client_message, " "); // first argument from the client. Specifies remote-file-path

  //   if(arg1 == NULL) {
  //     // check if valid arguments provided
  //     strcpy(server_message, "RM command expects remote file path input.");
  //   } else {
  //     // run RM function
  //     strcpy(server_message, rm(arg1));
  //   }
  // } else {  // invalid command provided
  //   strcat(token, " is not a valid command.");
  //   strcpy(server_message, token);
  // }

  // Respond to client:
  if (send(client_sock, server_message, strlen(server_message), 0) < 0){
    printf("Can't send\n");
    return -1;
  }
  
  // Cleanup
  // pthread_exit((void*)&bt);
  fclose(log);
  close(client_sock);
  close(socket_desc);
  
  return 0;
}
