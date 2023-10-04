# backdoor
Project Description: 
This project is written in c and  uses socket programming 
to create a simple server which adheres to HTTP 1.1 standard.

This web server also takes one command line argument while it is
listening for incoming connections and executes it when it receives a GET request in the specified format.

The socket type used is a stream socket as opposed to datagram sockets
It treats communications as a continuous stream of characters.
Once the socket is created and binded to server address, 
the socket begins listening.

A while loop is used to continuosly be listing in for requests.
In the while loop, commands are accepted from client sockets and
read into the buffer.

There are checks to make sure that requests are valid HTTP1.1 requests
The response is dynamically allocated using malloc.
The response is sent back to client.

If SIGINT is used then program is terminated.

get command from url is a function to get the command for the backdoor
functionality.
Conversions are made and the command with /exec/ is read in and executed.
