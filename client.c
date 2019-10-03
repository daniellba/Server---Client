
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>

#define SIZE 4096
#define HTTP_PORT "80"

typedef struct Request {
	char *host;
	char *port;
	char *path;
	char *variables;
	char *text;
	char *req;
	int length;
	int flagP;
	int flagR;
	int flagHTTP;
}Request;

void createRequest(Request *request);
void errorFunc(Request *request);
void freeAll(Request *request);
int httpPrefix(const char *pre, const char *str);

int main(int argc, char *argv[])
{
	Request *request = (Request*)calloc(1, sizeof(Request));
	if (request == NULL)
	{
		freeAll(request);
		exit(1);
	}

	request->host = NULL;
	request->port = NULL;
	request->path = NULL;
	request->variables = NULL;
	request->text = NULL;
	request->req = NULL;
	request->length = 0;
	request->flagP = 0;
	request->flagR = 0;
	request->flagHTTP = 0;

	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-r") == 0 && request->flagR == 0)		//check the '-r' flag.
		{
			char *num = argv[i + 1];								//check if the argument is integer.
			for (int j = 0; j < strlen(num); j++)
			{
				if (!isdigit(num[j]))
				{
					errorFunc(request);
				}
			}

			int varNum = atoi(argv[i + 1]), count = 0;
			for (int j = i + 2; j < (varNum + i + 2); j++)			//check if all the variables are correct.
			{
				char *temp = strchr(argv[j], '=');
				
				if (temp != NULL && strlen(temp) != strlen(argv[j]) && strlen(temp) != 1)
				{													//check for each variable that it has '=' in the right place.
					count++;
				}

				else
				{
					errorFunc(request);
				}
			}
			
			if (count != varNum)									//check if the number of the variables is correct.
			{
				errorFunc(request);
			}

			else if	(varNum != 0)									//concatenation all the variables into a single string.
			{
				int sum = 0;
				for (int j = i + 2; j < varNum + i + 2; j++)		//sum all of the variables length.
				{
					sum += strlen(argv[j]);
				}

				request->variables = (char*)calloc(sum + varNum + 1, sizeof(char));
				if (request->variables == NULL)						//allocate memory for 'variables' string.
				{
					freeAll(request);
					exit(1);
				}

				strcat(request->variables, "?");

				for (int j = i + 2; j < varNum + i + 2; j++)		//concatenation the variables inside the string.
				{
					strcat(request->variables, argv[j]);
					if (j < varNum + i + 1)
					{
						strcat(request->variables, "&");
					}
				}
			}

			request->flagR++;										//change the flag.
			i = i + varNum + 1;										//go to the next flag.
		}

		else if (strcmp(argv[i], "-p") == 0 && request->flagP == 0)	//check the '-p' flag.
		{
			request->text = (char*)calloc(strlen(argv[i + 1]) + 1, sizeof(char));
			if (request->text == NULL)								//allocate memory for text string and insert the text inside.
			{
				freeAll(request);
				exit(1);
			}

			strcpy(request->text, argv[i + 1]);
			request->length = strlen(request->text);				//insert the length of the text to the variable in struct.
			
			request->flagP++;										//change the flag.
			i = i + 1;												//go to the next flag.
		}

		else if (strstr(argv[i], "http://") != NULL)				//checking if its URL.
		{
			char *tempURL = strchr(argv[i], '/')+2;		
			char *temp = strchr(tempURL, '/');						//find the path.

			int check = httpPrefix("http://", argv[i]);				//checks that there is nothing before the 'http://' in the URL. 
			if (check == 0)
			{
				errorFunc(request);
			}

			if (strcmp(tempURL, "") == 0)							//if there is no host.
			{
				request->host = (char*)calloc(2, sizeof(char));
				if (request->host == NULL)
				{
					freeAll(request);
					exit(1);
				}

				strcpy(request->host, "");
				request->port = (char*)calloc(3, sizeof(char));
				if (request->port == NULL)
				{
					freeAll(request);
					exit(1);
				}
				strcpy(request->port, HTTP_PORT);
			}

			if (temp != NULL)										//if there is a path.
			{
				request->path = (char*)calloc(strlen(temp) + 1, sizeof(char));
				if (request->path == NULL)							//allocate memory for the path and insert it to the variable in struct.
				{
					freeAll(request);
					exit(1);
				}

				strcpy(request->path, temp);
			}

			else													//if there is no path insert '/'.
			{
				request->path = (char*)calloc(2, sizeof(char));		//allocate memory for the path and insert it to the variable in struct.
				if (request->path == NULL)
				{
					freeAll(request);
					exit(1);
				}
				strcpy(request->path, "/");
			}
			
			temp = strtok(tempURL, "/");

			if (temp != NULL)										//find the host and the port.
			{
				char *tempB = strchr(temp, ':');

				if (tempB != NULL)									//if there is a port.
				{
					tempB++;
					for (int j = 0; j < strlen(tempB); j++)			//check if the argument is integer.
					{
						if (!isdigit(tempB[j]))
						{
							errorFunc(request);
						}
					}
					
					request->port = (char*)calloc(strlen(tempB) + 1, sizeof(char));
					if (request->port == NULL)						//allocate memory for the port and insert it to the variable in struct.
					{
						freeAll(request);
						exit(1);
					}

					strcpy(request->port, tempB);
					int length = strlen(temp) - strlen(tempB) - 1;
					request->host = (char*)calloc(length + 1, sizeof(char));
					if (request->host == NULL)						//allocate memory for the host and insert it to the variable in struct.
					{
						freeAll(request);
						exit(1);
					}

					strncpy(request->host, temp, length);
				}

				else												//if there is no port insert port '80'. 
				{
					request->port = (char*)calloc(3, sizeof(char));	//allocate memory for the port and insert it to the variable in struct.
					if (request->port == NULL)
					{
						freeAll(request);
						exit(1);
					}

					strcpy(request->port, HTTP_PORT);
					request->host = (char*)calloc(strlen(temp) + 1, sizeof(char));
					if (request->host == NULL)						//allocate memory for the host and insert it to the variable in struct.
					{
						freeAll(request);
						exit(1);
					}
					strcpy(request->host, temp);
				}
			}
			request->flagHTTP++;
		}

		else														//if this is no flag ('-p', '-r') or URL
		{
			errorFunc(request);
		}
	}

	if (request->flagHTTP != 1)										//if there is no URL in the command line.
	{
		errorFunc(request);
	}

	createRequest(request);											//creating the request to the server.	
	printf("HTTP request =\n%s\nLEN = %ld\n", request->req, strlen(request->req));//printing the request and its length.

	int sockfd, rc, rd = 0;											//initialization of all the parameters of the socket.
	struct sockaddr_in serv_addr;
	struct hostent * server;
	char buffer[1];

	sockfd = socket(PF_INET, SOCK_STREAM, 0);						//stating a TCP socket.
	if (sockfd < 0)
	{
		freeAll(request);
		perror("ERROR opening socket");
		exit(1);
	}

	server = gethostbyname(request->host);							//using the host to get IP.
	if (server == NULL)
	{
		close(sockfd);
		freeAll(request);
		herror("ERROR, no such host");
		exit(1);
	}

	serv_addr.sin_family = PF_INET;
	bcopy((char*)server->h_addr_list[0], (char*)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(atoi(request->port));				//converts the unsigned short integer hostshort from host byte order to network byte order.

	rc = connect(sockfd, (const struct sockaddr*)&serv_addr, sizeof(serv_addr)); //stating a connection with the server.
	if (rc < 0)
	{
		close(sockfd);
		freeAll(request);
		perror("ERROR connecting");
		exit(1);
	}

	rc = write(sockfd, request->req, strlen(request->req));			//write to the server.
	if (rc < 0)
	{
		close(sockfd);
		freeAll(request);
		perror("ERROR reading response from socket");
		exit(1);
	}

	int size = 0;

	while ((rd = read(sockfd, &buffer, 1)) != 0)					//read from the server.
	{
		if (rd < 0)
		{
			close(sockfd);
			freeAll(request);
			perror("ERROR reading response from socket");
			exit(1);
		}

		size++;
		printf("%s", buffer);
	}

	printf("\n Total received response bytes: %d\n", size);
	close(sockfd);													//close the socket.
	freeAll(request);												//free all the allocate memory.

	return 0;
}

void createRequest(Request *request)
{									//function that creat the request for the server.
	int sumLength = strlen(request->path) + strlen(request->host);
																	//sum the length of the request.
	if (request->variables)											//if there is a variables.
	{
		sumLength += strlen(request->variables);
	}
	
	if (request->text)												//if there is a text.
	{	
		sumLength += (strlen(request->text) + 55);

		request->req = (char*)calloc(sumLength, sizeof(char));		//allocate memory for the request.
		if (request->req == NULL)
		{
			freeAll(request);
			exit(1);
		}

		strcat(request->req, "POST ");
	}

	else															//if there is no text.
	{
		sumLength += 26;

		request->req = (char*)calloc(sumLength, sizeof(char));		//allocate memory for the request.
		if (request->req == NULL)
		{
			freeAll(request);
			exit(1);
		}

		strcat(request->req, "GET ");
	}

	strcat(request->req, request->path);							//concatenation the 'path'.
	
	if (request->variables)											//if there is a variables concatenate it.
	{
		strcat(request->req, request->variables);
	}

	strcat(request->req, " HTTP/1.0\r\n");
	strcat(request->req, "HOST: ");												
	strcat(request->req, request->host);							//concatenation the 'host'.
	strcat(request->req, "\r\n");

	if (request->text)												//if there is a text concatenate it and the length of the text.
	{
		strcat(request->req, "Content-length: ");
		char temp[10];
		memset(temp, '\0', 10);
		sprintf(temp, "%d", request->length);
		strcat(request->req, temp);
		strcat(request->req, "\r\n\r\n");
		strcat(request->req, request->text);
	}

	else
	{
		strcat(request->req, "\r\n");
	}
}

void errorFunc(Request *request)
{									//function for errors, print error message free all the memory and exit the program.
	printf("Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 …>] <URL>\n");
	freeAll(request);
	exit(1);
}

void freeAll(Request *request)
{									//function that free all the allocate memory.
	if (request->host)
	{
		free(request->host);
	}

	if (request->path)
	{
		free(request->path);
	}
			
	if (request->port)
	{
		free(request->port);
	}

	if (request->variables)
	{
		free(request->variables);
	}

	if (request->text)
	{
		free(request->text);
	}

	if (request->req)
	{
		free(request->req);
	}

	free(request);
}

int httpPrefix(const char *pre, const char *str)
{									//function that check if there is nothing before the 'http://' in the URL.
	return (strncmp(pre, str, strlen(pre)) == 0);
}
