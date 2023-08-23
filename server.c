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
#define MAX_CONNECTIONS 10 

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// print out the error and end execution
void err_n_die(const char *fmt, ...);

struct message{
  int type;
  int length;
  char msg[MSG_LEN];
};

int main (int argc, char **argv) {

  // first check if we got the right number of arguments
  if ( argc != 2 )
    err_n_die("Invalid number of arguments!", argv[0]);

  // // // // // // // // // // // // // // // // // // // //
  // Start the server: create a socket and listen for clients
  // // // // // // // // // // // // // // // // // // // //

  // declaring variables

  // socket file descriptors
  int listenfd, conn_o_tcpfd, conn_o_udpfd;
  // ports
  in_port_t udp_port, tcp_port = atoi(argv[1]);
  // client address and server address struct
  struct sockaddr_in cliaddr, servaddr;
  // msg struct
  struct message msg;

  // create a TCP socket
  if ( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    err_n_die("Unable to create a socket");

  // set the fields of the server address
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // we'll respond to any address!
  servaddr.sin_port =  htons(tcp_port); // htons takes care of endianness

  // bind the listen socket to the server address
  if ( (bind(listenfd, (SA*) &servaddr, sizeof(servaddr))) < 0 )
    err_n_die("bind error!");
  // listen for connections
  if ( (listen(listenfd, MAX_CONNECTIONS)) < 0 )
    err_n_die("listen error!");
    
  printf(ANSI_COLOR_CYAN "\n\tServer up and running at port %d\n\n" ANSI_COLOR_RESET, tcp_port);

  while(1) {
    // each iteration of this loop, the parent process will wait for a connection
    socklen_t clilen = sizeof(cliaddr);
    conn_o_tcpfd = accept(listenfd, (SA*) &cliaddr, &clilen); // got a connection!

    // // // // // // // // // // // // // // // // // // // //
    // create a child process to handle the client
    // // // // // // // // // // // // // // // // // // // //

    // to handle concurrency, we need a separate process to handle each request.
    // thus we fork and let the child process handle the request, while parent continues to accept other clients
    pid_t pid = fork();

    if ( pid < 0 )
      err_n_die("fork error!");
    else if ( pid == 0 ){

      printf(ANSI_COLOR_CYAN"Handling connection at ip %s with process id %d\n" ANSI_COLOR_RESET, inet_ntoa(cliaddr.sin_addr), getpid());

      close(listenfd); // we're not listening anymore

      // // // // // // // // // // // // // // // // // // // //
      // TCP phase : get a udp port number to send to the client 
      // // // // // // // // // // // // // // // // // // // //

      // read the message from the client
      if ( read(conn_o_tcpfd, &msg, MSG_LEN) < 0)
        err_n_die("connection failed");
            
      printf("[type=%d] RECEIVED, lenght=%d, content=%s\n", msg.type, msg.length, msg.msg);

      // create a UDP socket
	    if ((conn_o_udpfd = socket (AF_INET, SOCK_DGRAM, 0)) <0)
		    err_n_die("Problem in creating udp socket\n");

      // get the udp port number
	    servaddr.sin_port = htons(0); // let the system pick a port number
      // calling bind with port number 0 tells the kernel to choose the local port number
	    if (bind (conn_o_udpfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0)
		    err_n_die("Problem in binding to udp socket\n");

      // getsockname will get the port number that the kernel has assigned
	    struct sockaddr_in temp; // creating a temporary sockaddr_in structure to pass to getsockname
	    socklen_t templen = sizeof temp;
	    getsockname(conn_o_udpfd, (SA*)&temp,&templen);

      udp_port = ntohs(temp.sin_port);

      // got the port, send it to the client
	    msg.type = 2;
      msg.length = sprintf(msg.msg,"%d", udp_port);
	    printf("[type=%d] SENDING..., " ANSI_COLOR_YELLOW "UDP Port Number : %d\n" ANSI_COLOR_RESET,msg.type, udp_port);
      write(conn_o_tcpfd, &msg, MSG_LEN);
	    
      close(conn_o_tcpfd);
			
      // // // // // // // // // // // // // // // // // // // //
      // UDP Phase: get the message from the client over UDP
      // // // // // // // // // // // // // // // // // // // //

      // conn_o_udpfd is the socket that we will use to receive the message
      // just need to specify from where to recieve (and send) the message, 
      // for that we use recvfrom and sendto functions

      // read:
	    recvfrom(conn_o_udpfd,&msg,sizeof(msg),0,(SA*)&cliaddr, &clilen);

      // show:
      printf("[type=%d] RECEIVED, lenght=%d, content=" ANSI_COLOR_YELLOW "%s" ANSI_COLOR_RESET "\n", msg.type, msg.length, msg.msg);
      fflush(stdout);

      // send:
	    msg.type=4; msg.length = 0; msg.msg[0] ='\0';
	    printf("[type=%d] SENDING...\n",msg.type);
	    sendto(conn_o_udpfd, &msg, sizeof(msg), 0,(SA*)&cliaddr, clilen);

      // no need to close the sockets since we're exiting the process, kernel will do it for us
      printf(ANSI_COLOR_CYAN"Terminated connection at ip %s with process id %d\n\n" ANSI_COLOR_RESET, inet_ntoa(cliaddr.sin_addr), getpid());
      fflush(stdout);
      exit(0);
      // // // // // // // // // // // // // // // // // // // //
      // End of our communication with this client.
      // // // // // // // // // // // // // // // // // // // //
    }
    close(conn_o_tcpfd);
  }
  close(listenfd);
  return 0;
}

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