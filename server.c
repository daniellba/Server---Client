

#include "threadpool.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>

#define FIRST_LINE_LENGTH 4000
#define ENTITY_LINE_LENGTH 500
#define RESPOND_LENGTH 500
#define MAX_TIME 128
#define RFC1123FMT "%A, %D %B %Y %H:%M:%S GMT"

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


int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Usage: server <port> <pool-size> <max-number-of-request>\n");
        exit(1);
    }

    int port = integerCheck(argv[1]);
    int pool_size = integerCheck(argv[2]);
    int max_number_of_request = integerCheck(argv[3]);

    if (port <= 0 || pool_size <= 0 || max_number_of_request <= 0)  //Check the input.
    {
        printf("Usage: server <port> <pool-size> <max-number-of-request>\n");
        exit(1);
    }
    
    threadpool* tp = create_threadpool(pool_size);                  //Creating pool of threads.
    if (tp == NULL)
    {
        perror("Error in creating treadpool");
        exit(1);
    }

    int sockfd;                                                     //socket descriptor
    int *newsockfd = (int*)calloc(max_number_of_request, sizeof(int));
    struct sockaddr_in serv_addr;                                   //used by bind()

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
	    perror("ERROR opening socket \n");
	    exit(1);
    }

    serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);						        //theses struct will contain the ip of each client.
    socklen_t addr_len = sizeof(serv_addr);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{																//using the bind func to bind the current adress to the correct file descriptor.
		perror("ERROR on binding \n");
		exit(1);
	}

	if (listen(sockfd, 5) < 0)
    {
		perror("ERROR on listen");
        exit(1);
    }
    
    for(int i = 0; i < max_number_of_request; i++)
    {
        newsockfd[i] = accept(sockfd, (struct sockaddr*)&serv_addr, &addr_len);
        if(newsockfd[i] < 0)
        {
            perror("ERROR on accept");
            exit(1);
        }

        dispatch(tp, readAndCheckRequest, &newsockfd[i]);
    }
    
    destroy_threadpool(tp);
    free(newsockfd);
    close(sockfd);

    return 0;
}

int readAndCheckRequest(void* fd)
{
    int i = 0;
    int sockfd = *(int*)fd;
    char buff[FIRST_LINE_LENGTH] = "";
    memset(buff, '\0', FIRST_LINE_LENGTH);

    while(read(sockfd, buff + i, 1) > 0)                            //Read the request from client.
    {
        if (*(buff + i) == '\r')
        {
            break;
        }
        i++;
    }
    
    char* method;
    char* path;
    char* protocol;

    method = strtok(buff, " ");
    if (method == NULL)                                             //If there is no method in the request write '400 Bad Request' Respond.
    {
        char* respond = (char*)calloc(RESPOND_LENGTH, sizeof(char));
        if (respond == NULL)
        {
            char res[RESPOND_LENGTH] = "";
            memset(res, '\0', RESPOND_LENGTH);
            internalServerErrorRespond(res);                        //If there is a problem with memory allocation write '500 Internal Server Error' Respond.
            write(sockfd, res, strlen(res));
            close(sockfd);
            return 1;
        }

        badRequestRespond(respond);
        write(sockfd, respond, strlen(respond));
        free(respond);
        close(sockfd);
        return 1;
    }

    path = strtok(NULL, " ");
    if (path == NULL)                                               //If there is no path in the request write '400 Bad Request' Respond.
    {
        char* respond = (char*)calloc(RESPOND_LENGTH, sizeof(char));
        if (respond == NULL)
        {
            char res[RESPOND_LENGTH] = "";
            memset(res, '\0', RESPOND_LENGTH);
            internalServerErrorRespond(res);                        //If there is a problem with memory allocation write '500 Internal Server Error' Respond.
            write(sockfd, res, strlen(res));
            close(sockfd);
            return 1;
        }

        badRequestRespond(respond);
        write(sockfd, respond, strlen(respond));
        free(respond);
        close(sockfd);
        return 1;
    }

    protocol = strtok(NULL, "\r");
    if (protocol == NULL)                                           //If there is no protocol in the request write '400 Bad Request' Respond.
    {
        char* respond = (char*)calloc(RESPOND_LENGTH, sizeof(char));
        if (respond == NULL)
        {
            char res[RESPOND_LENGTH] = "";
            memset(res, '\0', RESPOND_LENGTH);
            internalServerErrorRespond(res);                        //If there is a problem with memory allocation write '500 Internal Server Error' Respond.
            write(sockfd, res, strlen(res));
            close(sockfd);
            return 1;
        }
        badRequestRespond(respond);
        write(sockfd, respond, strlen(respond));
        free(respond);
        close(sockfd);
        return 1;
    }
    
    if ((strcmp(protocol, "HTTP/1.0") != 0) && (strcmp(protocol, "HTTP/1.1") != 0))//If the protocol is not 'HTTP/1.0' or 'HTTP/1.1' write '400 Bad Request' Respond.
    {
        char* respond = (char*)calloc(RESPOND_LENGTH, sizeof(char));
        if (respond == NULL)
        {
            char res[RESPOND_LENGTH] = "";
            memset(res, '\0', RESPOND_LENGTH);
            internalServerErrorRespond(res);                        //If there is a problem with memory allocation write '500 Internal Server Error' Respond.
            write(sockfd, res, strlen(res));
            close(sockfd);
            return 1;
        }

        badRequestRespond(respond);
        write(sockfd, respond, strlen(respond));
        free(respond);
        close(sockfd);
        return 1;
    }

    if (strcmp(method, "GET") != 0)                                 //If the method is not 'GET' write '501 Not supported' Respond.
    {
        char* respond = (char*)calloc(RESPOND_LENGTH, sizeof(char));
        if (respond == NULL)
        {
            char res[RESPOND_LENGTH] = "";
            memset(res, '\0', RESPOND_LENGTH);
            internalServerErrorRespond(res);                        //If there is a problem with memory allocation write '500 Internal Server Error' Respond.
            write(sockfd, res, strlen(res));
            close(sockfd);
            return 1;
        }

        notSupportedRespond(respond);
        write(sockfd, respond, strlen(respond));
        free(respond);
        close(sockfd);
        return 1;
    }
    
    struct stat sb;
    char temp[FIRST_LINE_LENGTH] = "";
    strcat(temp, ".");
    strcat(temp, path);
    strcpy(path, temp);

    if ((stat(path, &sb) != 0) && (S_ISDIR(sb.st_mode) == 0))       //If the path is no file or directory write '404 Not Found' Respond.
    {
        char* respond = (char*)calloc(RESPOND_LENGTH, sizeof(char));
        if (respond == NULL)
        {
            char res[RESPOND_LENGTH] = "";
            memset(res, '\0', RESPOND_LENGTH);
            internalServerErrorRespond(res);                        //If there is a problem with memory allocation write '500 Internal Server Error' Respond.
            write(sockfd, res, strlen(res));
            close(sockfd);
            return 1;
        }

        notFoundRespond(respond);
        write(sockfd, respond, strlen(respond));
        free(respond);
        close(sockfd);
        return 1;
    }

    else if ((stat(path, &sb) == 0 ) && (S_ISDIR(sb.st_mode) != 0)) //If the path is a directory.
	{
		int slashSize = strlen(path) - 1;                           //The location that supposed to be a '/' in the path (in the end).
        
        char* respond = (char*)calloc(RESPOND_LENGTH, sizeof(char));
        if (respond == NULL)
        {
            char res[RESPOND_LENGTH] = "";
            memset(res, '\0', RESPOND_LENGTH);
            internalServerErrorRespond(res);                        //If there is a problem with memory allocation write '500 Internal Server Error' Respond.
            write(sockfd, res, strlen(res));
            close(sockfd);
            return 1;
        }

		if (strcmp(&(path[slashSize]), "/") != 0)                   //If there is no '/' in the end of the path.
		{
			foundRespond(respond, path);
            write(sockfd, respond, strlen(respond));
            free(respond);
            close(sockfd);
            return 1;		
		}

		if (strcmp(&(path[slashSize]), "/") == 0)                   //If there is '/' in the end of the path.
		{
			int counter = 0, htmlFlag = 0;
			DIR *directory;
			struct dirent *dir_st;
            
			directory = opendir(path);
			if(directory == NULL)
			{
				notFoundRespond(respond);
                write(sockfd, respond, strlen(respond));
                free(respond);
                close(sockfd);
                closedir(directory);
                return 1;
			}
					
			dir_st = readdir(directory);
			while(dir_st != NULL)                                   //Check if there is 'index.html' file in the directory.
			{   
				if(strcmp(dir_st->d_name, "index.html") != 0)
				{
					counter++;
					dir_st = readdir(directory);
				}

				else
				{
					htmlFlag = 1;                                   //The file 'index.html' is exist.
						
					char *file = (char*)calloc(RESPOND_LENGTH*sb.st_size, sizeof(char));
					if (file == NULL)
					{
						char res[RESPOND_LENGTH] = "";
                        memset(res, '\0', RESPOND_LENGTH);
                        internalServerErrorRespond(res);            //If there is a problem with memory allocation write '500 Internal Server Error' Respond.
                        write(sockfd, res, strlen(res));
                        free(respond);
                        closedir(directory);                       
                        close(sockfd);
                        return 1;
					}

                    char newPath[strlen(path) + 11];                //Creating a new path to check the file 'index.html'.
                    memset(newPath, '\0', strlen(path) + 11);
                    strcpy(newPath, path);
					strcat(newPath,"index.html");

					char fileSize[sb.st_size];                      //Get and set the size of 'index.html'.
					memset(fileSize, '\0', sizeof(fileSize));
                    sprintf(fileSize, "%lu", sb.st_size);
					
                    fileRespond(respond, newPath, fileSize);        //Create the file content respond.
                    write(sockfd, respond, strlen(respond));        //Write to client.

					FILE *fp = fopen(newPath, "rb");                //Open the file.
                    fseek(fp, 0, SEEK_SET);

                    int rc = 0;
                    while((rc = fread(file, sizeof(char), sb.st_size + 1, fp)) > 0)
                    {
                        write(sockfd, file, rc);                    //Write the file to the client.
                    }
                    
					free(file);
			        free(respond);
                    close(sockfd);
					fclose(fp);
					break;
				}
			}
            closedir(directory);
            
			if (htmlFlag == 0)                                      //If there is no 'index.html' file in the directory.
			{
				int i = 0;
                DIR *tempDirectory;
				char* files[counter], *filesLastModified[counter], filesSizes[counter][22], timebuf[128];
				memset(files, '\0', sizeof(files));
				memset(filesLastModified, '\0', sizeof(filesLastModified));
				memset(filesSizes, '\0', sizeof(filesSizes));
                
                tempDirectory = opendir(path);
			    if(tempDirectory == NULL)
			    { 
                    notFoundRespond(respond);
                    write(sockfd, respond, strlen(respond));
                    free(respond);
                    close(sockfd);
                    closedir(directory);
                    closedir(tempDirectory);
                    return 1;
			    }
                
				dir_st = readdir(tempDirectory);
				while(dir_st != NULL)                               //Goes on all files in the directory and get all the information(file name, last modified, size).
				{
					files[i] = dir_st->d_name;
					char tempPath[strlen(path)+strlen(dir_st->d_name)];
					strcpy(tempPath, path);
					strcat(tempPath, dir_st->d_name);
					struct stat s;
					stat(tempPath, &s);
					strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&s.st_mtime));
					filesLastModified[i] = timebuf;
                    
					if(S_ISDIR(s.st_mode) == 0)                     //If its file.
					{
						sprintf(filesSizes[i], "%lu", s.st_size);
					}

					else                                            //If its directory.
					{
						strcpy(filesSizes[i], "-1");	
					}
                    
					i++;
					dir_st = readdir(tempDirectory);
				}
                
			    free(respond);
                char* respond = (char*)calloc(RESPOND_LENGTH*counter, sizeof(char));
                if (respond == NULL)
                {
                    char res[RESPOND_LENGTH] = "";
                    memset(res, '\0', RESPOND_LENGTH);
                    internalServerErrorRespond(res);                //If there is a problem with memory allocation write '500 Internal Server Error' Respond.
                    write(sockfd, res, strlen(res));
                    close(sockfd);
                    return 1;
                }

				dirContentRespond(respond, path, files, filesLastModified, filesSizes, counter);//Create the directory content respond.
				write(sockfd, respond, strlen(respond));            //Write to client.
                free(respond);
                close(sockfd);
                closedir(tempDirectory);
			}
	    }
    }
        
    else if((stat(path, &sb) == 0 ) && (S_ISDIR(sb.st_mode) == 0))  //If the path is a file.
	{   
		if(S_ISREG(sb.st_mode) == 0)                                //If the file is not regular file write '403 Forbidden' Respond.
		{
			char* respond = (char*)calloc(RESPOND_LENGTH, sizeof(char));
            if (respond == NULL)
            {
                char res[RESPOND_LENGTH] = "";
                memset(res, '\0', RESPOND_LENGTH);
                internalServerErrorRespond(res);                    //If there is a problem with memory allocation write '500 Internal Server Error' Respond.
                write(sockfd, res, strlen(res));
                close(sockfd);
               return 1;
            }

            forbiddenRespond(respond);
            write(sockfd, respond, strlen(respond));
            free(respond);
            close(sockfd);
            return 1;
		}

		else if(!(sb.st_mode&S_IRUSR && sb.st_mode&S_IRGRP && sb.st_mode&S_IROTH))//If there is no reading permissions write '403 Forbidden' Respond.
		{
			char* respond = (char*)calloc(RESPOND_LENGTH, sizeof(char));
            if (respond == NULL)
            {
                char res[RESPOND_LENGTH] = "";
                memset(res, '\0', RESPOND_LENGTH);
                internalServerErrorRespond(res);                    //If there is a problem with memory allocation write '500 Internal Server Error' Respond.
                write(sockfd, res, strlen(res));
                close(sockfd);
               return 1;
            }

            forbiddenRespond(respond);
            write(sockfd, respond, strlen(respond));
            free(respond);
            close(sockfd);
            return 1;
		}

		else
		{
			char tempPath[strlen(path)];
            memset(tempPath, '\0', strlen(path));
			char* ptr;
			ptr = strchr(path, '/');
			
            while(ptr != NULL)                                      //If the path has permissions.
			{
                int size = (int)((ptr+1)-path);
				strncat(tempPath, path, size);
				struct stat s;
				stat(tempPath, &s);
				
                if(!(s.st_mode&S_IRUSR && s.st_mode&S_IRGRP && s.st_mode&S_IROTH))
				{
					char* respond = (char*)calloc(RESPOND_LENGTH, sizeof(char));
                    if (respond == NULL)
                    {
                        char res[RESPOND_LENGTH] = "";
                        memset(res, '\0', RESPOND_LENGTH);
                        internalServerErrorRespond(res);            //If there is a problem with memory allocation write '500 Internal Server Error' Respond.
                        write(sockfd, res, strlen(res));
                        close(sockfd);
                        return 1;
                    }

                    forbiddenRespond(respond);
                    write(sockfd, respond, strlen(respond));
                    free(respond);
                    close(sockfd);
                    return 1;;
				}

				else
				{
					ptr += 1;
					ptr = strchr(ptr, '/');
					if(ptr == NULL)
						break;			
				}
			}

			struct stat s;
			stat(path, &s);

            char* respond = (char*)calloc(RESPOND_LENGTH+s.st_size, sizeof(char));
            if (respond == NULL)
            {
                char res[RESPOND_LENGTH] = "";
                memset(res, '\0', RESPOND_LENGTH);
                internalServerErrorRespond(res);                    //If there is a problem with memory allocation write '500 Internal Server Error' Respond.
                write(sockfd, res, strlen(res));
                close(sockfd);
                return 1;
            }

			char *size = (char*)calloc((s.st_size + 1), sizeof(char));
			if(size == NULL)
			{
				char res[RESPOND_LENGTH] = "";
                memset(res, '\0', RESPOND_LENGTH);
                internalServerErrorRespond(res);                    //If there is a problem with memory allocation write '500 Internal Server Error' Respond.
                write(sockfd, res, strlen(res));
                free(respond);
                close(sockfd);
                return 1;
			}

	    	char *file = (char*)calloc((s.st_size + 1), sizeof(char));
			if(file == NULL)
			{
				char res[RESPOND_LENGTH] = "";
                memset(res, '\0', RESPOND_LENGTH);
                internalServerErrorRespond(res);                    //If there is a problem with memory allocation write '500 Internal Server Error' Respond.
                write(sockfd, res, strlen(res));
                free(respond);
                free(size);
                close(sockfd);
                return 1;
			}

			sprintf(size, "%lu", s.st_size);                        //Get the size of the file.
			fileRespond(respond, path, size);                       //Create the file content respond.
			write(sockfd, respond, strlen(respond));                //Write to client.
			
            FILE *fp = fopen(path, "rb");                           //Open the file.
			fseek(fp, 0, SEEK_SET);

            int rc = 0;
			while((rc = fread(file, sizeof(char), s.st_size + 1, fp)) > 0)
            {
				write(sockfd, file, rc);                            //Write the file to the client.
            }

			free(file);
			free(size);
			free(respond);
			fclose(fp);
	    }
    }
    
    close(sockfd);
    return 0;
}

int integerCheck(char* temp)                                        //Function that checks if the argument is an integer.
{
    for (int j = 0; j < strlen(temp); j++)
	{
		if (!isdigit(temp[j]))
		{
			printf("Usage: server <port> <pool-size> <max-number-of-request>\n");
            exit(1);
		}
	}

    return atoi(temp);
}

char *get_mime_type(char *name)                                     //Function that checks the file type.
{
    char *ext = strrchr(name, '.');
    if (!ext) return NULL;
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".au") == 0) return "audio/basic";
    if (strcmp(ext, ".wav") == 0) return "audio/wav";
    if (strcmp(ext, ".avi") == 0) return "video/x-msvideo";
    if (strcmp(ext, ".mpeg") == 0 || strcmp(ext, ".mpg") == 0) return "video/mpeg";
    if (strcmp(ext, ".mp3") == 0) return "audio/mpeg";
    return NULL;
} 

void getDate(char* respond)                                         //Function that create the Date line in the respond.
{
    strcat(respond, "Date: ");	
	time_t now;
	char timebuf[MAX_TIME] = "";
	now = time(NULL);
	strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));
	strcat(respond, timebuf);		
	strcat(respond, "\r\n");
}

void foundRespond(char* respond, char* path)                        //Function that create the '302 Found' Respond.
{
    strcat(respond, "HTTP/1.0 302 Found\r\nServer: webserver/1.0\r\n");//First line and server.
    
    getDate(respond);                                               //Date.

    strcat(respond, "Location: ");                                  //Location.
    strcat(respond, path);
    strcat(respond, "/\r\n");
                
    strcat(respond, "Content-Type: text/html\r\nContent-Length: 123\r\nConnection: close\r\n\r\n");//Content type, Content length and Connection.

    strcat(respond, "<HTML><HEAD><TITLE>302 Found</TITLE></HEAD>\r\n");//Body.
    strcat(respond, "<BODY><H4>302 Found</H4>\r\n");
    strcat(respond, "Directories must end with a slash.\r\n");
    strcat(respond, "</BODY></HTML>\r\n\r\n");
}

void badRequestRespond(char* respond)                               //Function that create the '400 Bad Request' Respond.
{    
    strcat(respond, "HTTP/1.0 400 Bad Request\r\nServer: webserver/1.0\r\n");//First line and server.

    getDate(respond);                                               //Date.
    
    strcat(respond, "Content-Type: text/html\r\nContent-Length: 113\r\nConnection: close\r\n\r\n");//Content type, Content length and Connection.

    strcat(respond, "<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD>\r\n");//Body.
    strcat(respond, "<BODY><H4>400 Bad request</H4>\r\n");
    strcat(respond, "Bad Request.\r\n");
    strcat(respond, "</BODY></HTML>\r\n\r\n");
}

void forbiddenRespond(char* respond)                                //Function that create the '403 Forbidden' Respond.
{
    strcat(respond, "HTTP/1.0 403 Forbidden\r\nServer: webserver/1.0\r\n");//First line and server.
    
    getDate(respond);                                               //Date.
    
    strcat(respond, "Content-Type: text/html\r\nContent-Length: 111\r\nConnection: close\r\n\r\n");//Content type, Content length and Connection.

    strcat(respond, "<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD>\r\n");//Body.
    strcat(respond, "<BODY><H4>403 Forbidden</H4>\r\n");
    strcat(respond, "Access denied.\r\n");
    strcat(respond, "</BODY></HTML>\r\n\r\n");
}

void notFoundRespond(char* respond)                                 //Function that create the '404 Not Found' Respond.
{
    strcat(respond, "HTTP/1.0 404 Not Found\r\nServer: webserver/1.0\r\n");//First line and server.
    
    getDate(respond);                                               //Date.
    
    strcat(respond, "Content-Type: text/html\r\nContent-Length: 112\r\nConnection: close\r\n\r\n");//Content type, Content length and Connection.

    strcat(respond, "<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>\r\n");//Body.
    strcat(respond, "<BODY><H4>404 Not Found</H4>\r\n");
    strcat(respond, "File not found.\r\n");
    strcat(respond, "</BODY></HTML>\r\n\r\n");
}

void internalServerErrorRespond(char* respond)                      //Function that create the '500 Internal Server Error' Respond.
{
    strcat(respond, "HTTP/1.0 500 Internal Server Error\r\nServer: webserver/1.0\r\n");//First line and server.
    
    getDate(respond);                                               //Date.
    
    strcat(respond, "Content-Type: text/html\r\nContent-Length: 144\r\nConnection: close\r\n\r\n");//Content type, Content length and Connection.

    strcat(respond, "<HTML><HEAD><TITLE>500 Internal Server Error</TITLE></HEAD>\r\n");//Body.
    strcat(respond, "<BODY><H4>500 Internal Server Error</H4>\r\n");
    strcat(respond, "Some server side error.\r\n");
    strcat(respond, "</BODY></HTML>\r\n\r\n");
}

void notSupportedRespond(char* respond)                             //Function that create the '501 Not supported' Respond.
{
    strcat(respond, "HTTP/1.0 501 Not supported\r\nServer: webserver/1.0\r\n");//First line and server.
   
    getDate(respond);                                               //Date.
    
    strcat(respond, "Content-Type: text/html\r\nContent-Length: 129\r\nConnection: close\r\n\r\n");//Content type, Content length and Connection.

    strcat(respond, "<HTML><HEAD><TITLE>501 Not supported</TITLE></HEAD>\r\n");//Body.
    strcat(respond, "<BODY><H4>501 Not supported</H4>\r\n");
    strcat(respond, "Method is not supported.\r\n");
    strcat(respond, "</BODY></HTML>\r\n\r\n");
}

void dirContentRespond(char* respond, char* path, char** files, char** filesLastModified, char filesSizes[][22], int counter)
{                                                                   //Function that return the contents of the directory in the format as in file 'dir_content.txt'.
    char* body = (char*)calloc(RESPOND_LENGTH*counter, sizeof(char));
    
    strcat(body, "<HTML>\r\n<HEAD><TITLE>Index of ");               //Create body.
    strcat(body, path);
    strcat(body, "</TITLE></HEAD>\r\n\r\n<BODY>\r\n<H4>Index of ");
    strcat(body, path);
    strcat(body, "</H4>\r\n\r\n<table CELLSPACING=8>\r\n<tr><th>Name</th><th>Last Modified</th><th>Size</th></tr>\r\n\r\n");
    
    for(int i = 0; i < counter; i++)
    {
        strcat(body, "<tr>\r\n");
        strcat(body, "<td><A HREF=\"");
        strcat(body, files[i]);
        
        if (strcmp(filesSizes[i], "-1") == 0)                       //Directory.
        {
            strcat(body, "/");
        }
        
        strcat(body, "\">");
        strcat(body, files[i]);
        strcat(body, "</A></td><td>");
        strcat(body, filesLastModified[i]);
        strcat(body, "</td><td>");
        
        if (strcmp(filesSizes[i], "-1") != 0)                       //File.
        {
            strcat(body, filesSizes[i]);
        }
        
        strcat(body, "</td>\r\n</tr>\r\n\r\n");
    }
    strcat(body, "</table>\r\n\r\n<HR>\r\n\r\n<ADDRESS>webserver/1.0</ADDRESS>\r\n\r\n</BODY></HTML>\r\n");
    

    strcat(respond, "HTTP/1.0 200 OK\r\nServer: webserver/1.0\r\n");//First line and server.

    getDate(respond);                                               //Date.
    
    char size[22];
    sprintf(size, "%ld", strlen(body));
    strcat(respond, "Content-Type: text/html\r\nContent-Length: "); //Content type.
    strcat(respond, size);                                          //Content length.
    strcat(respond, "\r\n");

    char timebuf[MAX_TIME];
    struct stat sb;
    if (!stat(path, &sb))                                           //Last modified.
    {
        strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&sb.st_mtime));
        strcat(respond, "Last-Modified: ");
        strcat(respond, timebuf);
        strcat(respond, "\r\n");
    }

    strcat(respond, "Connection: close\r\n\r\n");                   //Connection.
    strcat(respond, body);                                          //Body.

    free(body);
}

void fileRespond(char* respond, char* path, char* size)             //Function that return the contents of file as in file 'file.txt'.
{
    strcat(respond, "HTTP/1.0 200 OK\r\nServer: webserver/1.0\r\n");//First line and server.

    getDate(respond);                                               //Date.

    char* type = get_mime_type(path);
    if (type != NULL)                                               //Content type.
    {
        strcat(respond, "Content-Type: ");
        strcat(respond, type);
        strcat(respond, "\r\n");
    }
    
    strcat(respond, "Content-Length: ");                            //Content length.
    strcat(respond, size);
    strcat(respond, "\r\n");

    char timebuf[MAX_TIME];
    struct stat sb;
    if (!stat(path, &sb))                                           //Last modified.
    {
        strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&sb.st_mtime));
        strcat(respond, "Last-Modified: ");
        strcat(respond, timebuf);
        strcat(respond, "\r\n");
    }

    strcat(respond, "Connection: close\r\n\r\n");                   //Connection.
}