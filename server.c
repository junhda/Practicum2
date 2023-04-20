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
#include "sync_folder.h"

#define errExit(msg) do { perror(msg); exit(EXIT_FAILURE); \
                               } while (0)

#define CONFIG "./config.txt"
#define MAX_CHARACTERS 8196

static char drive1[1024], drive2[1024], server_port[20], server_ip_address[100];
static int socket_desc, client_sock, sync_thread_exit = 0; // global variable for socket descriptor
static int sync_drives[] = {1, 1};

/**
 * @brief sigint handler method that closes client and server socket and turns off background syncing thread
 * 
 * @param sig_num 
 */
static void sigintHandler(int sig_num) {
  printf("\nReceived SIGINT. Closing server.\n");

  // close client socket
  if (client_sock) {
    close(client_sock);
  }

  // close server socket
  if (socket_desc) {
    close(socket_desc);
  }

  // close background thread
  sync_thread_exit = 1;

  exit(0);
}

/**
 * @brief get method accepts a client GET command, reads the remote file, and returns the contens of the file as a server message string
 * 
 * @param command original command from client
 * @param server_message string pointer to paste response into
 */
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

/**
 * @brief info method accepts a client INFO command, and finds information about a file or directory in the input remote file path argument
 * 
 * @param command original command from client
 * @param server_message string pointer to paste response into
 */
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

/**
 * @brief md method accepts a client MD command; creates a new directory
 * 
 * @param command original command from client
 * @param server_message string pointer to paste response into
 */
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
    char resp[] = "MD command expects remote file path input.";
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
        strcat(response, " created successfully in Drive 1.\n");
      } else if(drive1Update == -1) {
        strcat(response, "MD command attempted to create an existing directory in Drive 1.\n");
      } else if(drive1Update == -2) {
        strcat(response, "MD command failed in Drive 1.\n");
      } else if(drive1Update == 0) {
        strcat(response, "Drive 1 not found.\n");
        sync_drives[0] = 0;
      }

      if(drive2Update == 1) {
        strcat(response, strings[1]);
        strcat(response, " created successfully in Drive 2.\n");
      } else if(drive2Update == -1) {
        strcat(response, "MD command attempted to create an existing directory in Drive 2.\n");
      } else if(drive2Update == -2) {
        strcat(response, "MD command failed in Drive 2.\n");
      } else if(drive2Update == 0) {
        strcat(response, "Drive 2 not found.\n");
        sync_drives[1] = 0;
      }
    }

    // send server_message
    strcpy(server_message, response);
    // printf("Server Response: %s", server_message);
  }

  // printf("Server Response MD End: %s", server_message);
}

/**
 * @brief put method accepts a client PUT command; takes in input remote path and string arguments and creates a new file at the path location with the string stored inside the file
 * 
 * @param command original command from client
 * @param server_message string pointer to paste response into
 */
void put(char* command, char* server_message) {
  // initialize response
  char response[MAX_CHARACTERS];
  memset(response, '\0', sizeof(response));

  // extract arguments
  // strings[1] specifies remote-file-path
  char* strings[3]; 
  strings[0] = strtok(command, " ");
  strings[1] = strtok(NULL, " ");
  strings[2] = strtok(NULL, "");

  // check if remote path argument provided
  if(strings[1] == NULL){
    printf("Client did not provide path.\n");
    char resp[] = "MD command expects remote file path input.";
    strcpy(server_message, resp);
  } else if(strrchr(strings[1], '.') == NULL) {
    printf("The path specified does not indicate a file location.\n");
    char resp[] = "The path specified does not indicate a file location.\n";
    strcpy(server_message, resp);
  } else {
    // create new directory in available drives
    int drive1Update = 0, drive2Update = 0;
    char path[2000];

    // create file in drive 1
    if(access(drive1, F_OK) != -1) {
      strcpy(path, drive1);
      strcat(path, strings[1]);

      // create / open file
      FILE* fp = fopen(path, "w");
      if(fp == NULL) {
        printf("Failed to open file %s in Drive 1\n", path);
        drive1Update = -2;
      } else {
        // write to file contents from PUT command
        fprintf(fp, "%s", strings[2]);

        // close the file and mark for success
        fclose(fp);
        printf("Successfully created file %s in Drive 1\n", path);
        drive1Update = 1;
      }
    }

    printf("Drive 1 check complete\n");
    
    // create file in drive 1
    if(access(drive2, F_OK) != -1) {
      strcpy(path, drive2);
      strcat(path, strings[1]);

      // create / open file
      FILE* fp = fopen(path, "w");
      if(fp == NULL) {
        printf("Failed to open file %s in Drive 2\n", path);
        drive2Update = -2;
      } else {
        // write to file contents from PUT command
        fprintf(fp, "%s", strings[2]);

        // close the file and mark for success
        fclose(fp);
        printf("Successfully created file %s in Drive 2\n", path);
        drive2Update = 1;
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
        strcat(response, " created successfully in Drive 1.\n");
      } else if(drive1Update == -2) {
        strcat(response, "PUT command failed in Drive 1.\n");
      } else if(drive1Update == 0) {
        strcat(response, "Drive 1 not found.\n");
        sync_drives[0] = 0;
      }

      if(drive2Update == 1) {
        strcat(response, strings[1]);
        strcat(response, " created successfully in Drive 2.\n");
      } else if(drive2Update == -2) {
        strcat(response, "PUT command failed in Drive 2.\n");
      } else if(drive2Update == 0) {
        strcat(response, "Drive 2 not found.\n");
        sync_drives[1] = 0;
      }
    }

    // send server_message
    strcpy(server_message, response);
    // printf("Server Response: %s", server_message);
  }
}

/**
 * @brief rm method accepts a client RM command; removes a file or directory at the input remote path
 * 
 * @param command original command from client
 * @param server_message string pointer to paste response into
 */
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

  if(strings[1] == NULL){
    printf("Client did not provide path.\n");
    char resp[] = "RM command expects remote file path input.";
    strcpy(server_message, resp);
  } else if(strrchr(strings[1], '.') == NULL){
    // checks if path specifies creating a new file
    printf("Client provided an invalid path.\n");
    char resp[] = "Invalid path specified.\n";
    strcpy(server_message, resp);
  } else {
    // remove file or directory in available drives
    int drive1Update = 0, drive2Update = 0;
    char path[2000];
    struct stat path_stat;

    // remove file or directory in drive 1
    if(access(drive1, F_OK) != -1) {
      strcpy(path, drive1);
      strcat(path, strings[1]);

      
      if(stat(path, &path_stat) == 0) {
        if(S_ISREG(path_stat.st_mode)) { // check if file
          // attempt to remove file
          if(remove(path) == 0) {
            printf("File %s removed successfully from Drive 1\n", path);
            drive1Update = 1;
          } else {
            printf("Failed to delete file %s from Drive 1\n", path);
            drive1Update = -2;
          }
        } else if(S_ISDIR(path_stat.st_mode)) { // check if directory
          if(rmdir(path) == 0) {
            printf("Directory %s removed successfully from Drive 1\n", path);
            drive1Update = 1;
          } else {
            printf("Failed to delete directory %s from Drive 1\n", path);
            drive1Update = -2;
          }
        } else {
          printf("%s is not a file or directory\n", path);
          drive1Update = -3;
        }
        
      } else {
        printf("File %s does not exist exists in Drive 1\n", path);
        drive1Update = -1;
      }
    }

    printf("Drive 1 check complete\n");
    
    // remove file or directory in drive 2
    if(access(drive2, F_OK) != -1) {
      strcpy(path, drive2);
      strcat(path, strings[1]);

      if(stat(path, &path_stat) == 0) {
        if(S_ISREG(path_stat.st_mode)) { // check if file
          // attempt to remove file
          if(remove(path) == 0) {
            printf("File %s removed successfully from Drive 2\n", path);
            drive2Update = 1;
          } else {
            printf("Failed to delete file %s from Drive 2\n", path);
            drive2Update = -2;
          }
        } else if(S_ISDIR(path_stat.st_mode)) { // check if directory
          if(rmdir(path) == 0) {
            printf("Directory %s removed successfully from Drive 2\n", path);
            drive2Update = 1;
          } else {
            printf("Failed to delete directory %s from Drive 2\n", path);
            drive2Update = -2;
          }
        } else {
          printf("%s is not a file or directory\n", path);
          drive2Update = -3;
        }
        
      } else {
        printf("File %s does not exist exists in Drive 2\n", path);
        drive2Update = -1;
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
        strcat(response, " removed successfully from Drive 1.\n");
      } else if(drive2Update == -1) {
        strcat(response, "File does not exists in Drive 1\n");
      } else if(drive1Update == -2) {
        strcat(response, "RM command failed in Drive 1.\n");
      } else if(drive1Update == -3) {
        strcat(response, "Provided remote path did not point to a file or directory\n");
      } else if(drive1Update == 0) {
        strcat(response, "Drive 1 not found.\n");
        sync_drives[0] = 0;
      }

      if(drive2Update == 1) {
        strcat(response, strings[1]);
        strcat(response, " removed successfully from Drive 2.\n");
      } else if(drive2Update == -1) {
        strcat(response, "File does not exists in Drive 2\n");
      } else if(drive2Update == -2) {
        strcat(response, "RM command failed in Drive 2.\n");
      } else if(drive1Update == -3) {
        strcat(response, "Provided remote path did not point to a file or directory\n");
      } else if(drive2Update == 0) {
        strcat(response, "Drive 2 not found.\n");
        sync_drives[1] = 0;
      }
    }

    // send server_message
    strcpy(server_message, response);
    printf("Server Response: %s", server_message);
  }
}

/**
 * @brief background thread method that will continuously check if both USB drives are connected. If a drive is reconnected and has become out of sync, 
 * this thread will resync that drive based on the state of the other drive
 * 
 * @param arg null
 * @return void* 
 */
void *drive_sync(void *arg) {
  int drive1_flag = 0;
  int drive2_flag = 0;
  while(sync_thread_exit != 1) {
    // check that Drive 1 is connected
    if(access(drive1, F_OK) != -1) {
      drive1_flag = 1;
      if(access(drive2, F_OK) != -1 && sync_drives[0] == 0 && sync_drives[1] == 1) {
        printf("Syncing Drive 1\n");
        sync_folders(drive2, drive1);
        // mark Drive 1 as up-to-date
        sync_drives[0] = 1;
      }
      
    } else {
      printf("Drive 1 is not connected\n");
      drive1_flag = 0;
    }

    // check that Drive 2 is connected
    if(access(drive2, F_OK) != -1) {
      drive2_flag = 1;
      if(access(drive1, F_OK) != -1 && sync_drives[1] == 0 && sync_drives[0] == 1) {
        printf("Syncing Drive 2\n");
        sync_folders(drive1, drive2);
        // mark Drive 2 as up-to-date
        sync_drives[1] = 1;
      }
    } else {
      printf("Drive 2 is not connected\n");
      drive2_flag = 0;
    }

    // wait before next checkx
    sleep(5);
  }

  return NULL;
}

/**
 * @brief reads in parameters from the config.txt file
 * 
 */
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

/**
 * @brief main running method for executable file
 * 
 * @return int 
 */
int main(void)
{
  // initialize config variables
  initialize_config();
  int port = atoi(server_port);

  // sigint handler registration
  if (signal(SIGINT, sigintHandler) == SIG_ERR) {
    printf("Error: Unable to register signal handler for SIGINT.\n");
    return -1;
  }

  // create background thread for syncing USB drives
  pthread_t bt;
  pthread_create(&bt, NULL, drive_sync, NULL);
  if(!bt) {
    printf("Error: Unable to create thread\n");
    return -1;
  }
  // pthread_join(bt, NULL);

  // initialize variables
  socklen_t client_size;
  struct sockaddr_in server_addr, client_addr;
  char server_message[MAX_CHARACTERS], client_message[MAX_CHARACTERS];

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

  while(1) {
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

    // Respond to client:
    printf("Server message: %s\n", server_message);
    if (send(client_sock, server_message, strlen(server_message), 0) < 0){
      printf("Can't send\n");
      return -1;
    }
    
    // cleanup client connection
    close(client_sock);
    memset(server_message, '\0', sizeof(server_message));
    memset(client_message, '\0', sizeof(client_message));
  }
  
  return 0;
}
