#include <sys/types.h>
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

#define SOCKET_ERROR        -1
#define BUFFER_SIZE         1024
#define QUEUE_SIZE          5

#pragma GCC diagnostic ignored "-Wwrite-strings"

using namespace std;

string get_file(char* file_path, string serving_dir)
{
	//gets rid of the "/" before trying to find the file
	if(strcmp(file_path,"/") == 0 || strcmp(file_path, "") == 0)
	{
		//Try to find index, if not then display directory
		file_path = "index.html";
	}
	else
	{
		file_path++;
	}
	
	/*
	if(serving_dir != "/" || serving_dir !=".")
	{
		printf("\n\nTHIS RIGHT HERE RIGHT NOW- SERVING DIR: %s\n\n", serving_dir.c_str());
		string s= serving_dir + "/" + file_path;
		copy(s.begin(), s.end(), file_path);
	}
	*/
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
	string filetype= " ";
	
	if(filename.length() > 4)
	{
		string file_end = filename.substr( filename.size()-4,4);
		
		if(file_end == "html")
		{
			filetype="text/html";
		}
		else if(file_end == ".txt")
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
	}
	
	return filetype;
}

int main(int argc, char* argv[])
{
    int hSocket,hServerSocket;  /* handle to socket */
    struct hostent* pHostInfo;   /* holds info about a machine */
    struct sockaddr_in Address; /* Internet socket address stuct */
    int nAddressSize=sizeof(struct sockaddr_in);
    int nHostPort;
    char* client_request;
    char pBuffer[BUFFER_SIZE];
    string serving_dir;
    
    struct stat filestat;

    if(argc < 3)
      {
        printf("\nUsage: server host-port serving-dir\n");
        return 0;
      }
    else
      {
        nHostPort=atoi(argv[1]);
        //serving_dir = argv[2];
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


    printf("\nMaking a listen queue of %d elements",QUEUE_SIZE);
    /* establish listen queue */
    if(listen(hServerSocket,QUEUE_SIZE) == SOCKET_ERROR)
    {
        printf("\nCould not listen\n");
        return 0;
    }

    for(;;)
    {
        printf("\nWaiting for a connection\n");
        /* get the connected socket */
        hSocket=accept(hServerSocket,(struct sockaddr*)&Address,(socklen_t *)&nAddressSize);
		
       	if(hSocket < 0)
       	{
			printf("ERROR: Could not accept connection");
			return 0;
       	}
       	else
       	{
			//Add new task
			printf("\nGot a connection. Reading from the client.\n ");
			
			read(hSocket,pBuffer,sizeof(pBuffer));
	  
			printf("Here is what I got: \n\n%s", pBuffer);
	 
			char * request_type = strtok(pBuffer, " ");
			char * request_filepath = strtok(NULL, " ");

			printf("\n\nType: %s URL: %s", request_type, request_filepath);
			
			printf("\n\nWriting back.\n");
			
		  //I need to take in the URL from the client (parse the header) and find that file on my server
		  //Once I find the file on the server I need to retrieve it and send it back with a 200
				
		  string content= get_file(request_filepath, serving_dir);
		  string contentType = get_contentType(request_filepath);
		  				
		  printf("\n\nContent Type: %s \n\nContent: %s", contentType.c_str(), content.c_str());	
		  ostringstream stream;	
		  
		  stream << "HTTP/1.1 ";
		  if(contentType == " " || content == "")
		  {
			  stream << "404 Not Found\r\nMIME-Version:1.0\r\nContent-Type:text/html\r\nContent-Length:9\r\n\r\nNOT FOUND";
		  }
		  else
		  {
			  stream << "200 OK\r\nMIME-Version:1.0\r\nContent-Type:" << contentType << "\r\nContent-Length:" << content.length() << "\r\n\r\n";// << content;
		  }
				
		  string response = stream.str();
		  char* rPointer = new char[response.length() + 1];
		  strcpy(rPointer, response.c_str());
		  
		  //Gets rid of the "/" before the filepath
		  request_filepath++;
		  				
		  write(hSocket, rPointer, response.length());
		  sendfile(hSocket, open(request_filepath, O_RDONLY), NULL,  filestat.st_size);
		  
		  printf("\nClosing the socket");
		  /* close socket */
		  memset(pBuffer, 0, sizeof(pBuffer));
		  if(close(hSocket) == SOCKET_ERROR)
			{
			  printf("\nCould not close socket\n");
			  return 0;
			}
		}
    }
}
