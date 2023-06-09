/*
 * client.c -- TCP Socket Client
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

#define MAX_CHARACTERS 8196

static char port[20], ip_address[100];

/**
 * @brief takes in client PUT request and reformats it to PUT <remote_path> <local_file_contents>. this is sent to server and the server response is printed to console
 * 
 * @param command 
 * @param socket_desc 
 */
void put_request(char* command, int socket_desc) {
  char new_message[MAX_CHARACTERS];
  char server_message[MAX_CHARACTERS];
  // extract arguments from command
  char* strings[3]; 
  int j = 0;
  for(int i = 0; i < sizeof(strings); i++) {
    strings[i] = NULL;
  }
  char* token = strtok(command, " ");
  while(token != NULL) {
    strings[j++] = token;
    token = strtok(NULL, " ");
  }

  // if remote path is missing, set equal to local path
  if(strings[2] == NULL) {
    strings[2] = strings[1];
  }

  printf("Command: %s\n", strings[0]);
  printf("Local Path: %s\n", strings[1]);
  printf("Remote Path: %s\n", strings[2]);

  // check that local file path was specified
  if(strings[1] == NULL) {
    printf("PUT command expects a local file path input");
    strcpy(new_message, strings[0]);
  } else {
    // update local file path to remove starting ./ or /
    char* relpath = strings[1];
    char relpath_updated[1024];
    int len = strlen(strings[1]);

    // Check if the relative path starts with "./" or "/"
    if (strncmp(relpath, "./", 2) == 0) {
      strcpy(relpath_updated, relpath + 2);
    } else if (strncmp(relpath, "/", 1) == 0) {
      strcpy(relpath_updated, relpath + 1);
    } else {
      strcpy(relpath_updated, relpath);
    }

    printf("Relative path: %s\n", relpath_updated);

    // if remote file path argument missing, substitute with local file path
    if(strings[2] == NULL) {
      // strcpy(strings[2], relpath_updated);
      strings[2] = relpath_updated;
    }

    // recreate the PUT request
    strcat(new_message, strings[0]);
    strcat(new_message, " ");
    strcat(new_message, strings[2]);
    strcat(new_message, " ");

    // create local path
    char path[2048], pwd[1024];
    if (getcwd(pwd, sizeof(pwd)) != NULL) {
      printf("Current working directory: %s\n", pwd);
      strcpy(path, pwd);
      strcat(path, "/");
      strcat(path, relpath_updated);

      printf("Absolute path: %s\n", path);

      // read contents from local file and add to PUT request
      FILE* put_file = fopen(path, "r");
      if(!put_file) {
        printf("Local file %s does not exist", path);
        strcpy(new_message, NULL);
      } else {
        char buffer[MAX_CHARACTERS];
        size_t n = fread(buffer, sizeof(char), MAX_CHARACTERS, put_file);
        if(n == 0) {
          printf("Failed to read file %s\n", path);
          fclose(put_file);
          return;
        } else {
          strcat(new_message, buffer);
          fclose(put_file);
        }
      }
    } else {
      printf("Could not find current working directory\n");
      strcpy(new_message, NULL);
    }
  }

  

  // Send the message to server:
  printf("Sending request: %s\n", new_message);
  if(send(socket_desc, new_message, strlen(new_message), 0) < 0){
    printf("Unable to send message\n");
    return;
  }
  
  // Receive the server's response:
  if(recv(socket_desc, server_message, sizeof(server_message), 0) < 0){
    printf("Error while receiving server's msg\n");
    return;
  }
  
  printf("Server's response: %s\n",server_message);
}

/**
 * @brief updates client GET request to GET <remote_file_path>. This is sent to server, and it waits for the server response. 
 * The response is then saved in the third argument <local_file_path> which is or isn't specified by client
 * 
 * @param command 
 * @param socket_desc 
 */
void get_request(char* command, int socket_desc) {
  char new_message[MAX_CHARACTERS];
  char server_message[MAX_CHARACTERS];
  // extract arguments from command
  char* strings[3]; 
  int j = 0;
  for(int i = 0; i < sizeof(strings); i++) {
    strings[i] = NULL;
  }
  char* token = strtok(command, " ");
  while(token != NULL) {
    strings[j++] = token;
    token = strtok(NULL, " ");
  }

  // printf("%s\n", strings[0]);
  // printf("%s\n", strings[1]);
  // printf("%s\n", strings[2]);

  // check that remote file path was specified
  if(strings[1] == NULL) {
    // Send the message to server:
    if(send(socket_desc, command, strlen(command), 0) < 0){
      printf("Unable to send message\n");
      return;
    }
    
    // Receive the server's response:
    if(recv(socket_desc, server_message, sizeof(server_message), 0) < 0){
      printf("Error while receiving server's msg\n");
      return;
    }
    
    printf("Server's response: %s\n",server_message);
    return;
  }

  // if remote file path argument missing, substitute with local file path
  if(strings[2] == NULL) {
    strings[2] = strings[1];
  }

  // recreate the GET request
  strcat(new_message, strings[0]);
  strcat(new_message, " ");
  strcat(new_message, strings[1]);

  printf("Sending request: %s\n", new_message);

  // Send the message to server:
  if(send(socket_desc, new_message, strlen(new_message), 0) < 0){
    printf("Unable to send message\n");
    return;
  }

  // printf("Message sent\n");
  
  // Receive the server's response:
  if(recv(socket_desc, server_message, sizeof(server_message), 0) < 0){
    printf("Error while receiving server's msg\n");
    return;
  }
  
  // printf("Server's response: %s\n",server_message);

  // save server response to file
  FILE* fnew = fopen(strings[2], "w");
  if(fnew == NULL) {
    printf("Failes to create file %s\n", strings[2]);
    return;
  }

  fputs(server_message, fnew);

  fclose(fnew);
}

/**
 * @brief reads in parameters from the config.txt file
 * 
 */
void initialize_config() {
  // read config variables
  FILE* f = fopen("config.txt", "r");
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
      
      if(strcmp(key, "port") == 0) {
        strcpy(port, value);
      } else if(strcmp(key, "ip_address") == 0) {
        strcpy(ip_address, value);
      } 
    }
  }

  fclose(f);
}

/**
 * @brief main running method for client program. takes in command line arguments, or prompts for command from client if command line arguments missing
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char *argv[])
{
  // read environment variables
  initialize_config();
  int port_no = atoi(port);

  // initialize variables
  int socket_desc;
  struct sockaddr_in server_addr;
  char server_message[MAX_CHARACTERS], client_message[MAX_CHARACTERS];
  
  // Clean buffers:
  memset(server_message,'\0',sizeof(server_message));
  memset(client_message,'\0',sizeof(client_message));
  
  // Create socket:
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  
  if(socket_desc < 0){
    printf("Unable to create socket\n");
    return -1;
  }
  
  printf("Socket created successfully\n");
  
  // Set port and IP the same as server-side:
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port_no);
  server_addr.sin_addr.s_addr = inet_addr(ip_address);
  
  // Send connection request to server:
  if(connect(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
    printf("Unable to connect\n");
    return -1;
  }
  printf("Connected with server successfully\n");
  
  // check if args provided in command line
  // if not, request user submits command
  if(argc == 1) {
    // Get input from the user:
    printf("Enter message: ");
    fgets(client_message, MAX_CHARACTERS, stdin);
  } else {
    for(int i = 1; i < argc; i++) {
      strcat(client_message, argv[i]);
      strcat(client_message, " ");
    }
  }

  // extract first command from client_message
  char message_copy[MAX_CHARACTERS];
  strcpy(message_copy, client_message);
  char* token = strtok(message_copy, " ");  // first command word from the client. this should define the command type

  if(strcmp(token, "GET") == 0) { // check if command is a GET command
    printf("GET command called: %s\n", client_message);
    get_request(client_message, socket_desc);
  } else if(strcmp(token, "PUT") == 0) { // check if command is a PUT command
    printf("PUT command called: %s\n", client_message);
    put_request(client_message, socket_desc);
  } else {
    printf("Command called: %s\n", client_message);
    // Send the message to server:
    if(send(socket_desc, client_message, strlen(client_message), 0) < 0){
      printf("Unable to send message\n");
      return -1;
    }
    
    // Receive the server's response:
    if(recv(socket_desc, server_message, sizeof(server_message), 0) < 0){
      printf("Error while receiving server's msg\n");
      return -1;
    }
    
    printf("Server's response: %s\n",server_message);
  }
  
  // Close the socket:
  close(socket_desc); 
  
  return 0;
}
