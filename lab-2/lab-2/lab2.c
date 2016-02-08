
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

#define SOCKET_ERROR        -1
#define BUFFER_SIZE         10000
#define MESSAGE             "This is the message I'm sending back and forth"
#define QUEUE_SIZE          5
#define MAXPATH 1000

string getContentType(string filename){
	string filetype;
	char c=filename.back();
	switch(c){
	case 'l' :
		filetype="text/html";
		break;
	case 't':
		filetype="text/plain";
		break;
	case 'f':
		filetype="image/gif";
		break;
	case 'g':
		filetype="image/jpg";
		break;
	default :
		filetype="invalid";
	}
	return filetype;
}

long getContentLength(string filename){
	struct stat stat_buf;
	int rc=stat(filename.c_str(), &stat_buf);
	return rc==0? stat_buf.st_size:-1;
}

bool exists (const string& name){
	struct stat buffer;
	return(stat (name.c_str(),&buffer)==0);
}

bool isDirectory(const string& name){
	struct stat s;
	return stat(name.c_str(), &s)==0 && s.st_mode&S_IFDIR;
}

string createIndex(string root, string path){
	string index= "<h1>" +path+" Index </h1><ul>";
	int len;
	DIR *dirp;
	struct dirent *dp;
	
	dirp=opendir((root+path).c_str());
	if(path!="/")path+="/";
	while((dp=readdir(dirp))!=NULL){
		string file=string(dp->d_name);
		if(file!="." && file !=".."){
			index+="<li><a href='"+path + file +"'>" + file + "</li>";
		}
	}
	(void)closedir(dirp);
	return index+"</ul>";
}

int main(int argc, char* argv[])
{
    int hSocket,hServerSocket;  /* handle to socket */
    struct hostent* pHostInfo;   /* holds info about a machine */
    struct sockaddr_in Address; /* Internet socket address stuct */
    int nAddressSize=sizeof(struct sockaddr_in);
    char pBuffer[BUFFER_SIZE];
    int nHostPort;
    string directory_root;
    char _directory_root[MAXPATH];

    if(argc < 3)
      {
        printf("\nUsage: server host-port dir\n");
        return 0;
      }
    else
      {
        nHostPort=atoi(argv[1]);
	directory_root=argv[2];
	strcpy(_directory_root,argv[2]);
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
int optval = 1;
setsockopt (hServerSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    for(;;)
    {
        printf("\nWaiting for a connection\n");
        /* get the connected socket */
        hSocket=accept(hServerSocket,(struct sockaddr*)&Address,(socklen_t *)&nAddressSize);
	


        printf("\nGot a connection from %X (%d)\n",
              Address.sin_addr.s_addr,
              ntohs(Address.sin_port));
        memset(pBuffer,0,sizeof(pBuffer));
        int rval = read(hSocket,pBuffer,BUFFER_SIZE);
	printf("Got from browser %d\n%s\n",rval, pBuffer);
#define MAXPATH 1000
	char path[MAXPATH];
	rval = sscanf(pBuffer,"GET %s HTTP/1.1",path);
	
	
	printf("Got rval %d, path %s\n",rval,path);
	char fullpath[MAXPATH];
	sprintf(fullpath,"%s%s",argv[2], path);
	printf("fullpath %s\n",fullpath);
	memset(pBuffer,0,sizeof(pBuffer));
	
	if(!exists(directory_root+path)){
		cout<<"File does not exist"<<endl;
		sprintf(pBuffer,"HTTP/1.1 404 Not Found\r\nContent-Type:text/html\r\nContent-Length:7\r\n\r\nFailure");
		write(hSocket, pBuffer, strlen(pBuffer));
	} else if(isDirectory(directory_root+path)){
		if(exists(directory_root+path+"/index.html")){
			sprintf(path,"%s/index.html", path);
			cout <<"Folder has an index"<<path<<endl;
			string contentType =getContentType(path);
			long contentLength=getContentLength(directory_root+path);
				
			sprintf(pBuffer,"HTTP/1.1 200 ok\r\n\ Content-Type: %s\r\n\ Content-Length: %ld\r\n\r\n", contentType.c_str(), contentLength);
			write(hSocket, pBuffer, strlen(pBuffer));

			printf(pBuffer);
			sprintf(fullpath,"%s%s",_directory_root, path);

			FILE *fp=fopen(fullpath,"r");
			char *buffer = (char *)malloc(contentLength+1);

			fread(buffer, contentLength, 1, fp);
			write(hSocket, buffer, contentLength);
			free(buffer);
			fclose(fp);
		} else{
			cout<<"There is no index in this directory\n";
			string index=createIndex(directory_root, path);
			sprintf(pBuffer,"HTTP/1.1 200 OK\r\n\ Content-Type: text/html\r\n\ Content-Length: %ld\r\n\r\n", index.length());

			write(hSocket, pBuffer, strlen(pBuffer));
			write(hSocket, index.c_str(), strlen(index.c_str()));
		}
	} else{
		string contentType=getContentType(path);
		long contentLength= getContentLength(directory_root+path);

		sprintf(pBuffer,"Http/1.1 200 OK\r\n\ Content-Type: %s\r\n\ Content-Length: %ld\r\n\r\n", contentType.c_str(), contentLength);
		write(hSocket, pBuffer, strlen(pBuffer));
		FILE *fp=fopen(fullpath,"r");
		char *buffer=(char *)malloc(contentLength+1);
		fread(buffer,contentLength,1,fp);
		write(hSocket, buffer, contentLength);
		free(buffer);
		fclose(fp);
	}

#ifdef notdef
linger lin;
unsigned int y=sizeof(lin);
lin.l_onoff=1;
lin.l_linger=10;
setsockopt(hSocket,SOL_SOCKET, SO_LINGER,&lin,sizeof(lin));	
shutdown(hSocket, SHUT_RDWR);
#endif
    printf("\nClosing the socket");
        /* close socket */
        if(close(hSocket) == SOCKET_ERROR)
        {
         printf("\nCould not close socket\n");
         return 0;
        }
    }
}
