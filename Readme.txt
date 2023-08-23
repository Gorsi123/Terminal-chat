Group 5
-------
Rollnumbers:
- 200101014
- 200101015
- 200101016
- 200101017

To execute:
----------------
1. first build the executables
  $  gcc -o c client.c 
  $  gcc -o s server.c 
2. Run:
  $ ./s <port_number>
  $ ./c <server_ip> <port_number>

Code Explanation:
-----------------

Server first starts listening at the port specified in the command line arguments.
It does so by binding a TCP socket with the server address struct containing the
port number, then calls listen to listen on this port. 

Then for each client request, it connects over TCP, and forks itself to let the child
handle the request, while the parent process continues to listen for more connections.
The child then uses bind to ask kernel for an empty UDP port, sends it to the client 
and then communicates over that port. Bind when called with port number 0 tells the 
kernel to choose the port number.

When the client starts it connects with the server over TCP using the ip address and 
port number specified in the command line arguments. We don't need to choose the 
client's port number, kernel will do it for us. After recieving UDP port number, it
closes the TCP connection and connects over UDP, then asks user to enter a message
that it sends to the server over UDP, and after recieving confirmation reply, it
closes the connection.
