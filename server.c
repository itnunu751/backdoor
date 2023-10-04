#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

int portno = 0;
int sockfd; //socket file descriptor

void accept_command(struct sockaddr_in server_address);
void SIGINT_callback_handler(int signal_num);
void get_command_from_url(char * url);
int is_gzip_enabled(const char * str);

int main(int argc, char *argv[]){
  //check command line arguments
  if(argc < 2){
    printf("Command to Execute: ./normal_web_server <port_number>\nMissing port number.\n");
    exit(1);
  }
  portno = atoi(argv[1]); //converts portno from string of digits to int

  //Create a new socket descriptor
  sockfd = socket(AF_INET, SOCK_STREAM, 0); //AF_INET is used for internet domain for any two hosts on the internet. SOCK_STREAM is used for continuous stream as if from a file. 0 is used for choosing the most appropriate protocol which is TCP for stream sockets and UDP for datagram sockets

  //check if there was an error creating socket.
  if (sockfd == 0)
  {
      perror("failed to get socket");
      exit(1);
  }

  //register handler for Ctrl + C
  signal(SIGINT, SIGINT_callback_handler);

  //create server address
  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET; //1st field of struct sockaddr_in
  serv_addr.sin_port = htons(portno); //2nd field of struc sockaddr_in, port no but must convert to network byte order first
  serv_addr.sin_addr.s_addr = INADDR_ANY; //3rd field of struct sockaddr_in. ip address of the host which is given by INADDR_ANY

  //bind socket and port
  int bind_status = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)); //binds socket to an address. takes three arguments: socket file descriptor, pointer to a strcut of type sockaddr which is sockaddr_in cast to the corret type, and size of serv_addr
  if(bind_status < 0){
    perror("Failed to bind socket to port");
    exit(1);
  }

  //listen to socket
  int listen_status = listen(sockfd, 5); //allows process to listen on the socket for connections. first arg is sockfd and second is size of the backlog queue, 5 is the max size permitted by most systems
  if(listen_status < 0){
    perror("Failed to listen to port");
    exit(1);
  }

  accept_command(serv_addr); //call accept command

  return 0;
}

void accept_command(struct sockaddr_in serv_addr){
  char * ok_response_header = "HTTP/1.1 200 OK\n"
                              "Content-Type: text/html\n"
                              "Accept-Ranges: bytes\n"
                              "Connection: close\n"
                              "\n";
  char * not_found_response_header = "HTTP/1.1 404 NOTFOUND\n"
                                    "Content-Type: text/html\n"
                                    "Accept-Ranges: bytes\n"
                                    "Connection: close\n"
                                    "\n";
  char * get_request = "GET";

  while(1){
    //accept incoming requests
    int len_of_address = sizeof(serv_addr);
    int client_socket_descriptor = accept(sockfd, (struct sockaddr *)&serv_addr, (socklen_t *)&len_of_address);

    char buff[1000];
    char url[1000];

    read(client_socket_descriptor, buff, 1000);
    printf("%s\n", buff);

    char command_result[1000];
    char response_header[1000];

    if(strlen(buff)<= 10 || strncmp(get_request, buff, strlen(get_request)) != 0){
      strcpy(response_header, not_found_response_header);
      strcpy(command_result, "Invalid request only GET is allowed.");
    }
    else{
      int i, j=0;
      for(i = 4; i<strlen(buff); i++){
        if(buff[i] == ' '){
          break;
        }
        url[j++] = buff[i];
      }
      url[j] = '\0';
      get_command_from_url(url);
      printf("Command recieved: %s\n", url);

      if(strlen(url) == 0){
        strcpy(response_header, not_found_response_header);
        strcpy(command_result, "Invalid command!");
      }
      else{
        strcat(url, " 2>&1");
        char tmp_buff[1000];
        FILE* file = popen(url, "r");
        if(file == NULL){
          strcpy(response_header, not_found_response_header);
          strcpy(command_result, "Invalid command!");
        }
        else{
          int flag = 0;
          while (fgets(tmp_buff, sizeof(tmp_buff), file) != NULL)
          {
            if(flag == 0){
                strcpy(command_result, tmp_buff);
                flag++;
            }
            else{
                strcat(command_result, tmp_buff);
            }
          }
          strcpy(response_header, ok_response_header);
        }

        if (pclose (file) != 0)
        {
          strcpy(response_header, not_found_response_header);
          file = NULL;
        }

        memset(tmp_buff, 0, sizeof(tmp_buff));
      }
    }

    char * response = NULL;
    response = malloc(strlen(response_header) + strlen(command_result) + 10);
    strcpy(response, response_header);
    strcat(response, command_result);

    send(client_socket_descriptor, response, strlen(response), 0);
    close(client_socket_descriptor);
    printf("Message sent:\n%s\n", response);

    //clear all buffers
    memset(buff, 0, sizeof(buff));
    memset(url, 0, sizeof(url));
    memset(response, 0, sizeof(response));
    memset(command_result, 0, sizeof(command_result));
    memset(response_header, 0, sizeof(response_header));
  }
}

void SIGINT_callback_handler(int signal_num){
  //Release the port
  close(sockfd);
  shutdown(sockfd, 2);

  //exit program
  exit(0);
}

void get_command_from_url(char * url){
  char * tmp_ptr;
  tmp_ptr = malloc(strlen(url));
  int i, j=0;

  for(i = 0; i<strlen(url); i++){
    if(*(url+i) == '+'){
      tmp_ptr[j++] = ' ';
    }
    else if(*(url+i) != '%'){
      tmp_ptr[j++] = *(url+i);
    }
    else if(!isxdigit(*(url+i+1)) || !isxdigit(*(url+i+2))){
      tmp_ptr[j++] = *(url+i);
    }
    else{
      char first_char = tolower(*(url+i+1));
      char second_char = tolower(*(url+i+2));
      if(first_char <= '9'){
        first_char = first_char - '0';
      }
      else{
        first_char = first_char - 'a' + 10;
      }
      if(second_char <= '9'){
        second_char = second_char - '0';
      }
      else{
        second_char = second_char - 'a' + 10;
      }

      tmp_ptr[j++] = (16 * first_char + second_char);
      i += 2;
    }
  }
  tmp_ptr[j] = '\0';
  strcpy(url, tmp_ptr);

  char * exec = "/exec/";
  if(strlen(url) <= 6 || strncmp(exec, tmp_ptr, strlen(exec)) != 0){
    memset(tmp_ptr, 0, sizeof(tmp_ptr));
    strcpy(url, tmp_ptr);
    return;
  }

  char command[1000];
  strcpy(command, url+6);
  strcpy(url, command);
}

int is_gzip_enabled(const char * str){
  return strstr(str, "content-coding: gzip") != NULL? 1 : 0;
}
