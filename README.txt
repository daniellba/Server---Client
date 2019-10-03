
	
**********Description**********

This program is presenting HTTP server, that implement the full HTTP specification, but only a very limited subset of it.
It is implement the following A HTTP server that:
	- Constructs an HTTP response based on client's request.
	- Sends the response to the client. 

	
**********Functions**********

int readAndCheckRequest(void* fd);
int integerCheck(char* temp);
char *get_mime_type(char *name);
void getDate(char* respond);
void foundRespond(char* respond, char* path);
void badRequestRespond(char* respond);
void forbiddenRespond(char* respond);
void notFoundRespond(char* respond);
void internalServerErrorRespond(char* respond);
void notSupportedRespond(char* respond);
void dirContentRespond(char* respond, char* path, char** files, char** filesLastModified, char filesSizes[][22], int counter);
void fileRespond(char* respond, char* path, char* size);


**********Program Files**********

-server.c -  handle the connections with the clients.
-threadpool.c - implement pool of threads and when there will be available thread (can be immediately), 
				it will handle the connection in the server(read request and write response).
-README.txt - the file contain an information about the file.


**********How to compile**********

Write in the terminal 'gcc -Wall -o server server.c threadpool.c threadpool.h'.
**you need the 'threadpool.h' file(is not in the file).


**********How to run**********

Write in the terminal ./server <port> <pool-size> <max-number-of-request>


**********Output**********

printing the request and the response that received from the server that the client sent.

