#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <sstream>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <dirent.h>
#include <queue>
#include <iostream>

#define SOCKET_ERROR        -1
#define BUFFER_SIZE         1024
#define QUEUE_SIZE          5000
#define MAX_MSG_SZ			1024

#pragma GCC diagnostic ignored "-Wwrite-strings"

using namespace std;

//sem_wait increments the counter
//sem_post decrements the counter

queue<int> work_tasks; //All the tasks 

sem_t work_mutex; //Controls queues. Makes sure only one task is moving at a time 
sem_t work_to_do; //Tells us how many tasks are on the queue
sem_t space_on_q; // Tells us if there is any space on the queue

struct thread_info
{
    int thread_id;
    int another_number;
};

// Determine if the character is whitespace
bool isWhitespace(char c)
{
    switch (c)
    {
        case '\r':
        case '\n':
        case ' ':
        case '\0':
            return true;
        default:
            return false;
    }
}

// Strip off whitespace characters from the end of the line
void chomp(char *line)
{
    int len = strlen(line);
    while (isWhitespace(line[len]))
    {
        line[len--] = '\0';
    }
}

// Read the line one character at a time, looking for the CR
// You dont want to read too far, or you will mess up the content
char * GetLine(int fds)
{
    char tline[MAX_MSG_SZ];
    char *line;

    int messagesize = 0;
    int amtread = 0;
    while((amtread = read(fds, tline + messagesize, 1)) < MAX_MSG_SZ)
    {
        if (amtread > 0)
            messagesize += amtread;
        else if (amtread == 0)
		{
			printf("Didn't read anything in.");
			exit(2);
		}
		else
			{
				perror("Socket Error is:");
				fprintf(stderr, "Read Failed on file descriptor %d messagesize = %d\n", fds, messagesize);
				exit(2);
			}
			//fprintf(stderr,"%d[%c]", messagesize,message[messagesize-1]);
			if (tline[messagesize - 1] == '\n')
				break;
    }
    tline[messagesize] = '\0';
    chomp(tline);
    line = (char *)malloc((strlen(tline) + 1) * sizeof(char));
    strcpy(line, tline);
    //fprintf(stderr, "GetLine: [%s]\n", line);
    return line;
}
    
// Change to upper case and replace with underlines for CGI scripts
void UpcaseAndReplaceDashWithUnderline(char *str)
{
    int i;
    char *s;
    s = str;
    string temp = str;
    
    if(temp.find(':'))
    {
		for (i = 0; s[i] != ':'; i++)
		{
			if (s[i] >= 'a' && s[i] <= 'z')
				s[i] = 'A' + (s[i] - 'a');
			
			if (s[i] == '-')
				s[i] = '_';
		}
    }
}

// When calling CGI scripts, you will have to convert header strings
// before inserting them into the environment.  This routine does most
// of the conversion
char *FormatHeader(char *str, char *prefix)
{
    char *result = (char *)malloc(strlen(str) + strlen(prefix));
    char* value = strchr(str,':') + 2;
    
    fflush(stdout); // Will now print everything in the stdout buffer
    UpcaseAndReplaceDashWithUnderline(str);
    *(strchr(str,':')) = '\0';
    sprintf(result, "%s%s=%s", prefix, str, value);
    return result;
}

void GetTitleInfo(vector<char *> &headerLines, int skt)
{
    char *line;
    char *tline;
    
	tline = GetLine(skt);
    line = (char *)malloc((strlen(tline) + 10) * sizeof(char));
    sprintf(line, "HTTP_%s", tline);   
    headerLines.push_back(line);
    free(tline);
}

// Get the header lines from a socket
//   envformat = true when getting a request from a web client
//   envformat = false when getting lines from a CGI program

void GetHeaderLines(vector<char *> &headerLines, int skt, bool envformat)
{
    // Read the headers, look for specific ones that may change our responseCode
    char *line;
    char *tline;
    
    tline = GetLine(skt);

    while(strlen(tline) != 0)
    {
        if (strstr(tline, "Content-Length") || 
            strstr(tline, "Content-Type"))
        {
            if (envformat)
				line = FormatHeader(tline, "");
            else
                line = strdup(tline);
        }
        else
        {
            if (envformat)
				if(strstr(tline, ":") != NULL)
				{
					line = FormatHeader(tline, "HTTP_");
				}
            else
            {
                line = (char *)malloc((strlen(tline) + 10) * sizeof(char));
                sprintf(line, "HTTP_%s", tline);                
            }
        }
        //fprintf(stderr, "Header --> [%s]\n", line);
        
        headerLines.push_back(line);
        free(tline);
       
        tline = GetLine(skt);
    }
    
    free(tline);
}

string makeDirList()
{
	  int len;
	  DIR *dirp;
	  struct dirent *dp;
	  string content;

	  dirp = opendir(".");
	  while ((dp = readdir(dirp)) != NULL)
		content+= ("\n%s\n", dp->d_name);
	  (void)closedir(dirp);
	  
	  //printf("\n\n\nReturning Dir: \n\n%s\n\n", content.c_str());
	  return content;
}

string get_file(char* file_path, string serving_dir)
{
	//gets rid of the "/" before trying to find the file
	if(strcmp(file_path,"/") == 0 || strcmp(file_path, "") == 0)
	{
		//Try to find index, if not then display directory
		file_path = "index.html";
		return makeDirList();
	}
	else
	{
		file_path++;
	}
	
	try{
		//Set the mode to be binary
		ifstream ifs(file_path, ios::in | ios::binary);
		string content;
		content.assign( (istreambuf_iterator<char>(ifs) ),
						   (istreambuf_iterator<char>()    ) );
		return content;
	}
	catch(int e)
	{
		printf("\nCouldn't get it. Error: %d\n", e);
		return "";
	}
}

string get_contentType(string filename)
{
	string filetype="text/html";
	
	if(filename.length() > 4)
	{
		string file_end = filename.substr( filename.size()-4,4);

		if(file_end == ".txt")
		{
			filetype="text/plain";
		}
		else if(file_end == ".jpg")
		{
			filetype="image/jpg";
		}
		else if(file_end == ".gif")
		{
			filetype="image/gif";
		}	
		else if(file_end == ".cgi" || file_end == ".pl")
		{
			filetype="cgi";
		}	
		else
		{
			filetype="text/html";
		}
	}
	
	return filetype;
}

string readBody(int hSocket, int size)
{	
	char tline[MAX_MSG_SZ];

    int messagesize = 0;
    int amtread = 0;
    while((amtread = read(hSocket, tline + messagesize, 1)) > 0)
    {
        if (amtread > 0)
            messagesize += amtread;
        if(messagesize >= size)
				break;
    }
    tline[messagesize] = '\0';
	string content(tline);
	
	return content;
}

void forkMe(char* client_request, string filename, vector<char*> headerLines, int contentLength, int hSocket)
{
	int pid;
	int status = 0;	
	int ServeToCGIpipefd[2];
	int CGIToServepipefd[2];
	pipe(ServeToCGIpipefd);
	pipe(CGIToServepipefd);
	
	char *args[] = {const_cast<char*> (filename.c_str()),NULL};
	
	char *env[] = { (char*)malloc((headerLines.size() + 1) * sizeof(char*)) };
	int i;
	for(i = 0; i < headerLines.size(); i++)
	{
		env[i] = (char *)malloc((strlen(headerLines[i]) + 1)  * sizeof(char));
		strcpy(env[i], headerLines[i]);
		if(strstr(env[i], "CONTENT_LENGTH"))
		{
			sscanf(env[i], "CONTENT_LENGTH=%d", &contentLength);
		}
	}
	env[i] = NULL;
	
	pid = fork();
	if (pid != 0)
	{
		// Parent
		close(ServeToCGIpipefd[0]);
		close(CGIToServepipefd[1]);
		
		if(strstr(client_request,"HTTP_GET") == NULL)
		{
				string test = readBody(hSocket, contentLength);//get the body from the socket
				write(ServeToCGIpipefd[1], test.c_str(), test.length());
				//cout << "TEST: " << test << "\n\n";
				//write it to servetocgipipe[1] 
		}

		pid = wait(NULL);
		char buffer[255];
		memset(buffer, 0, sizeof(buffer));
		int readsize = 0;

		//Write the header and then read until you can't read anymore. 
		write(hSocket, "HTTP/1.0 200 OK\r\nMIME-Version:1.0\r\n", 29);
		while((readsize = read(CGIToServepipefd[0], buffer, sizeof(buffer))) > 0)
		{
			write(hSocket, buffer, readsize);
			memset(buffer, 0, sizeof(buffer));
		}
		close(ServeToCGIpipefd[1]);
		close(CGIToServepipefd[0]);
	}
	else
	{
		close(ServeToCGIpipefd[1]);
		dup2(ServeToCGIpipefd[0], 0);
		close(CGIToServepipefd[0]);
		dup2(CGIToServepipefd[1], 1);
		execve(filename.c_str(), args, env);
	}
}

void* serve( void* in_data )
{
	char pBuffer[BUFFER_SIZE];
	char* client_request;
	char* query_string = NULL;
	string serving_dir;
    struct thread_info* t_info = ( struct thread_info* ) in_data;
    int tid = t_info->thread_id;
	vector<char *> headerLines;
	char * request_filepath = NULL;

    while( 1 )
    {
		sem_wait( &work_to_do); //Is there work to do? Okay, then I will do it. If not we're just waiting
        sem_wait( &work_mutex ); //I'm locking myself so that no one else can touch me.
        
        int hSocket = work_tasks.front(); //Take the first thing in the work queue
        work_tasks.pop(); //Pop it from the queue
        			
			// Read the header lines
			headerLines.clear();
			GetHeaderLines(headerLines, hSocket , true);
			//Get the first header line. Seperate the client request from the request filepath			
			client_request = strtok(headerLines.at(0), " ");
			request_filepath = strtok(NULL, " ");
			if(request_filepath == NULL)
			{
				request_filepath = "";
			}
			if(strstr(request_filepath, "?") != NULL)
			{
				request_filepath = strtok(request_filepath, "?");
				query_string = strtok(NULL, "\n");
			}
			
			printf("\nRequest Type: %s Filepath: %s Query_String: %s\n", client_request, request_filepath, query_string);
			fflush(stdout);
	 			
		  //I need to take in the URL from the client (parse the header) and find that file on my server
		  //Once I find the file on the server I need to retrieve it and send it back with a 200
			
		  string content= get_file(request_filepath, serving_dir);
		  string contentType = get_contentType(request_filepath);
		  ostringstream stream;	
		  		  
		  //Gets rid of the "/" before the filepath
		  request_filepath++;
		  stream << "HTTP/1.0 ";
		  if(content == "")
		  {
			  stream << "404 Not Found\r\nMIME-Version:1.0\r\nContent-Type:text/html\r\nContent-Length:9\r\n\r\nNOT FOUND";
		  }
		  else if( contentType =="cgi")
		  {
				headerLines.push_back("GATEWAY_INTERFACE=CGI/1.1");
				
				char * str = new char[strlen(request_filepath)+13];
				strcpy(str, "REQUEST_URI=");
				strcat(str, request_filepath);
				headerLines.push_back(str);
				
				if(strstr(client_request, "HTTP_GET") != NULL)
				{
					headerLines.push_back("REQUEST_METHOD=GET");
				}
				else
				{
					headerLines.push_back("REQUEST_METHOD=POST");
				}
				if(query_string != NULL)
				{
					char * str2 = new char[strlen(query_string)+13];
					strcpy(str2, "QUERY_STRING=");
					strcat(str2, query_string);
					headerLines.push_back(str2);
				}
				forkMe(client_request, request_filepath, headerLines, content.length(), hSocket);
		  }
		  else
		  {
			  stream << "200 OK\r\nMIME-Version:1.0\r\nContent-Type:" << contentType << "\r\nContent-Length:" << content.length() << "\r\n\r\n"; //<< content;
		  }
				
		  
		if( contentType != "cgi"  || content == "")
		{
			string response = stream.str();
			char* rPointer = new char[response.length() + 1];
			strcpy(rPointer, response.c_str());				
			write(hSocket, rPointer, response.length());
			sendfile(hSocket, open(request_filepath, O_RDONLY), NULL,  content.length());
		}
		  
		  //printf("\nClosing the socket\n\n");
		  /* close socket */
		  memset(pBuffer, 0, sizeof(pBuffer));
		  shutdown(hSocket, SHUT_RDWR);
		  if(close(hSocket) == SOCKET_ERROR)
			{
			  printf("\nCould not close socket\n");
			  fflush(stdout);
			  return 0;
			}
       
	//read from the file descriptor (work_object_grabbed)
	//parse the headers
	//find the resource
        //    check if folder or file
	//    Get file contents
	//    get file length
	//    get file extensions for http response
        // format your http response (headers and body)
	// write response you requested
	
	        
        sem_post( &work_mutex ); //Alright, I'm done. Freeing myself up
        sem_post( &space_on_q ); //We can add one to the space, we finished. 
}
}

int main(int argc, char* argv[])
{
	int hSocket,hServerSocket;  /* handle to socket */
	struct hostent* pHostInfo;   /* holds info about a machine */
	struct sockaddr_in Address; /* Internet socket address stuct */
	int nAddressSize=sizeof(struct sockaddr_in);
	int nHostPort;
	int threadCount;
	
    if(argc < 4)
      {
        printf("\nUsage: server host-port thread-count serving-dir\n");
        fflush(stdout);
        return 0;
      }
    else
      {
        nHostPort=atoi(argv[1]);
        threadCount = atoi(argv[2]);
        //serving_dir = argv[3];
      }

    printf("\nStarting server");

    printf("\nMaking socket");
    /* make a socket */
    hServerSocket=socket(AF_INET,SOCK_STREAM,0);

    if(hServerSocket == SOCKET_ERROR)
    {
        printf("\nCould not make a socket\n");
        return 0;
    }

    /* fill address struct */
    Address.sin_addr.s_addr=INADDR_ANY;
    Address.sin_port=htons(nHostPort);
    Address.sin_family=AF_INET;

    printf("\nBinding to port %d",nHostPort);

    /* bind to a port */
    if(bind(hServerSocket,(struct sockaddr*)&Address,sizeof(Address))
                        == SOCKET_ERROR)
    {
        printf("\nCould not connect to host\n");
        fflush(stdout);
        return 0;
    }
 /*  get port number */
    getsockname( hServerSocket, (struct sockaddr *) &Address,(socklen_t *)&nAddressSize);
    printf("opened socket as fd (%d) on port (%d) for stream i/o\n",hServerSocket, ntohs(Address.sin_port) );

        printf("Server\n\
              sin_family        = %d\n\
              sin_addr.s_addr   = %d\n\
              sin_port          = %d\n"
              , Address.sin_family
              , Address.sin_addr.s_addr
              , ntohs(Address.sin_port)
            );

	printf("\nMaking a Thread Pool\n\n");
	
	sem_init( &work_mutex, 0, 1 ); //Only one thread can access this, protects the mutual exclusion - The queue will work with this
    sem_init( &work_to_do, 0, 0);
    sem_init( &space_on_q, 0, QUEUE_SIZE);

    pthread_t threads[ threadCount ];

    struct thread_info all_thread_info[ threadCount ];

	//Creating a thread pool
    for( int i = 0; i < threadCount; i++ )
    {
        sem_wait( &work_mutex );
 
        printf("creating thread: %d \t\n", i);
        all_thread_info[ i ].thread_id = i;
        pthread_create( &threads[ i ], NULL, serve, ( void* ) &all_thread_info[ i ] );
 
        sem_post( &work_mutex );
    }


    printf("\nMaking a listen queue of %d elements",QUEUE_SIZE);
    /* establish listen queue */
    if(listen(hServerSocket,QUEUE_SIZE) == SOCKET_ERROR)
    {
        printf("\nCould not listen\n");
        fflush(stdout);
        return 0;
    }

    for(;;)
    {
        /* get the connected socket */
		hSocket= accept(hServerSocket,(struct sockaddr*)&Address,(socklen_t *)&nAddressSize);
				
       	if(hSocket < 0)
       	{
			printf("ERROR: Could not accept connection");
			fflush(stdout);
       	}
       	else
       	{
			
			sem_wait( &space_on_q);  //"Yes I have enough space on the queue"
			sem_wait( &work_mutex);  //"Now I need to lock the queue so that nobody tries to take this until i'm done"
        
			work_tasks.push(hSocket); 
        
			sem_post( &work_mutex); //
			sem_post( &work_to_do); //There is now work to do, let's go do it
		}
    }
}
