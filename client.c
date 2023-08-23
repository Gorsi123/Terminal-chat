#include <stdio.h> 
#include <stdlib.h> 
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h> 
#include <sys/types.h>
#include <string.h>
#include <stdarg.h> 
#include <errno.h>
#include <unistd.h>

#define MSG_LEN 255
#define SA struct sockaddr

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// prints out the error and ends execution
void err_n_die(const char *fmt, ...);

struct message {
  int type;
  int length;
  char msg[MSG_LEN];
};

int main(int argc, char** argv){

  // first check if we got the right number of arguments
  if ( argc != 3 )
    err_n_die("Invalid number of arguments!", argv[0]);

  // // // // // // // // // // // // // // // // // // // // // // //
  //  Begin the TCP Phase of execution : connect with server over TCP
  // // // // // // // // // // // // // // // // // // // // // // //

  // declaring variables

  // socket file descriptor
  int sockfd;
  // ports
  in_port_t udp_port, tcp_port = atoi(argv[2]);
  // server address struct
  struct sockaddr_in servaddr;
  // msg struct
  struct message msg;

  // get the socket file descriptor
  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    err_n_die("Unable to create a TCP socket");

  // set the fields of the server address
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port =  htons(tcp_port); // htons takes care of endianness

  // convert IP from dotted-decimal string to its binary value
  if ( inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0 ) 
    err_n_die("inet_pton error for %s ", argv[1]); 
  
  socklen_t servlen = sizeof(servaddr);

  // initiate connection
  if ( connect(sockfd, (SA*) &servaddr, servlen ) < 0 )
    err_n_die("Unable to connect to the server");
  
  printf(ANSI_COLOR_CYAN"\nProgram initiated.\n"ANSI_COLOR_RESET);

  // // // // // // // // // // // // // // // // // // // //
  // TCP conncection established. Lets ask for the udp port
  // // // // // // // // // // // // // // // // // // // //

  // prepare the message:
  msg.type = 1;
	msg.length = 0;
  bzero(msg.msg, MSG_LEN);

  // send:
  printf("\n[type=%d] SENDING..., requesting UDP port number\n", msg.type);
  write(sockfd, &msg, sizeof(msg));

  // recieve:
  if (read(sockfd, &msg, sizeof(msg)) < 0)
    err_n_die("connection failed");
  
  // show:
  udp_port = atoi(msg.msg);
  printf("[type=%d] RECEIVED, " ANSI_COLOR_YELLOW "UDP Port Number : %d\n" ANSI_COLOR_RESET, msg.type, udp_port);
  close(sockfd);
  printf( ANSI_COLOR_CYAN"TCP connection closed.\n" ANSI_COLOR_RESET);

  // // // // // // // // // // // // // // // // // // // //
  // Now set up connection using the UDP port
  // // // // // // // // // // // // // // // // // // // //

  servaddr.sin_port = htons(udp_port);
  servlen = sizeof(servaddr);
  // use a new socket for UDP
  if ((sockfd = socket (AF_INET, SOCK_DGRAM, 0)) <0)
    err_n_die("Unable to create a UDP socket");

  // connect the socket using the new port number stored in servaddr 
  if(connect(sockfd, (SA*) &servaddr, servlen) < 0)
    err_n_die("Unable to connect to the server");

  // // // // // // // // // // // // // // // // // // // //
  // Connected over UDP, now send the message
  // // // // // // // // // // // // // // // // // // // //

  // get the message to send
  msg.type=3;
  for(int i=0; i<MSG_LEN; i++)
    msg.msg[i]='\0';
  printf("\n\tEnter msg to send: ");
  scanf("%[^\n]", msg.msg);
  msg.length = strlen(msg.msg);
  printf("\n[type=%d] SENDING..., msg=" ANSI_COLOR_YELLOW"%s\n"ANSI_COLOR_RESET, msg.type, msg.msg);
  fflush(stdout);

  // send:
  write(sockfd, &msg, sizeof(msg));

  // recieve:
  if(read(sockfd, &msg, sizeof(msg)) < 0)
    err_n_die("connection failed");
  
  // show:
  printf("[type=%d] RECEIVED\n", msg.type);
  close(sockfd);
  printf(ANSI_COLOR_CYAN"UDP connection closed.\n"ANSI_COLOR_RESET);
  printf(ANSI_COLOR_CYAN"\nProgram terminated.\n"ANSI_COLOR_RESET);

  // // // // // // // // // // // // // // // // // // // //
  // The end.
  // // // // // // // // // // // // // // // // // // // //
}

// prints the error message and ends execution
void err_n_die(const char*fmt, ...){
  int errno_save; 
  va_list ap;

  // any system or library call can set errno, so we need to save it now
  errno_save = errno;

  // print out the fmt+args to standard out
  va_start(ap,fmt);
  vfprintf(stdout, fmt, ap);
  fprintf(stdout, "\n");
  fflush(stdout);

  // print out error message
  if(errno_save != 0){
    fprintf(stdout, "(errno = %d) : %s\n", errno_save, strerror(errno_save));
    fprintf(stdout, "\n");
    fflush(stdout);
  }
  va_end(ap);

  // die part:
  exit(1);
}
