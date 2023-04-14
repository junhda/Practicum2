/*
 * client.c -- TCP Socket Client
 * 
 * adapted from: 
 *   https://www.educative.io/answers/how-to-implement-tcp-sockets-in-c
 */

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_CHARACTERS 8196

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

  // check that local file path was specified
  if(strings[1] == NULL) {
    printf("PUT command expects a local file path input");
    return;
  }

  // if remote file path argument missing, substitute with local file path
  if(strings[2] == NULL) {
    strcpy(strings[2], strings[1]);
  }

  // recreate the PUT request
  strcat(new_message, strings[0]);
  strcat(new_message, " ");
  strcat(new_message, strings[2]);
  strcat(new_message, " ");

  // read contents from local file and add to PUT request
  FILE* put_file = fopen(strings[1], "r");
  if(!put_file) {
    printf("Local file %s does not exist", strings[1]);
    strcpy(new_message, NULL);
  }
  char* line;
  while(fgets(line, sizeof(line), put_file)) {
    strcat(new_message, line);
    strcat(new_message, "\n");
  }

  fclose(put_file);

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
    printf("GET command expects a remote file path input\n");
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

int main(int argc, char *argv[])
{
  // read environment variables
  int port = 2000;//atoi(getenv("PORT"));
  char* ip_addr = "127.0.0.1"; //getenv("IP_ADDRESS");
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
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = inet_addr(ip_addr);
  
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
