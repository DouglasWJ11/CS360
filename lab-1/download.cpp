#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <vector>             // stl vector
#include <fcntl.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include<iostream>

#define SOCKET_ERROR        -1
#define BUFFER_SIZE         100
#define HOST_NAME_SIZE      255
#define MAX_MSG_SZ      1024
#define MAX_GET 	1000

#pragma GCC diagnostic ignored "-Wwrite-strings"

using namespace std;

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
    for (i = 0; s[i] != ':'; i++)
    {
        if (s[i] >= 'a' && s[i] <= 'z')
            s[i] = 'A' + (s[i] - 'a');
        
        if (s[i] == '-')
            s[i] = '_';
    }
    
}

// When calling CGI scripts, you will have to convert header strings
// before inserting them into the environment.  This routine does most
// of the conversion
char *FormatHeader(char *str, char *prefix)
{
    char *result = (char *)malloc(strlen(str) + strlen(prefix));
    char* value = strchr(str,':') + 2;
    UpcaseAndReplaceDashWithUnderline(str);
    *(strchr(str,':')) = '\0';
    sprintf(result, "%s%s=%s", prefix, str, value);
    return result;
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
                line = FormatHeader(tline, "HTTP_");
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

int main(int argc, char *argv[]) {
	int wSocket;
	struct hostent* hostInfo;
	struct sockaddr_in Address;
	long nHostAddress;
	char pBuffer[BUFFER_SIZE];
	char strHostName[HOST_NAME_SIZE];
	int nHostPort;
	string url;
	vector<char *> headerLines;
	char buffer[MAX_MSG_SZ];
	char contentType[MAX_MSG_SZ];

	extern char *optarg;
	int c, downloadTimes = 1, err = 0, successCount = 0;
	bool debug = false;

	if (argc < 4) {
		printf("\nUsage: download host port path \n"); 
		return 0;
	}
	else {
		while ((c = getopt(argc, argv, "c:d")) != -1) {
			switch (c) {
			case 'c':
				downloadTimes = atoi(optarg);
				break;
			case 'd':
				debug = true;
				break;
			case '?':
				cout << "\nUsage: download host port path\n";
				return 0;
			}
		}

		strcpy(strHostName, argv[optind]);
		nHostPort = atoi(argv[optind + 1]);
		url = argv[optind + 2];
	}
	
	for(int i=0; i<downloadTimes; i++){
		bool success=true;
		if(debug){
			printf("\nCreating socket");
		}
		
		wSocket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		if(wSocket==SOCKET_ERROR){
			success=false;
			printf("\nFAILED TO CREATE SOCKET");
			return 0;
		}
		
		if(debug){
			printf("\nGetting Hostname.");
		}
		
		hostInfo=gethostbyname(strHostName);
		
		if(hostInfo==NULL){
			success=false;
			printf("\nInvalid Hostname.\n");
			return 0;
		}
		
		memcpy(&nHostAddress,hostInfo->h_addr,hostInfo->h_length);

		Address.sin_addr.s_addr=nHostAddress;
		Address.sin_port=htons(nHostPort);
		Address.sin_family=AF_INET;

		string request = "GET " + url + " HTTP/1.0\r\nHost: "+ strHostName + "\r\n\r\n";
		char* rPointer = new char[request.length()+1];
		strcpy(rPointer, request.c_str());
		
		if(debug){
			printf("\nConnecting to %s (%X) on port %d", strHostName,nHostAddress,nHostPort);
		}
		if(connect(wSocket,(struct sockaddr*)&Address,sizeof(Address))==SOCKET_ERROR){
			printf("\nCoult not connect to Host.\n");
			success=false;
			return 0;
		}
		
		write(wSocket,rPointer,request.length());
		
		if(debug){
			printf("\nWriting \n%s",rPointer);
		}

		GetHeaderLines(headerLines,wSocket,false);
		
		if(debug){
			printf("\nHeaders: \n");
			for(int i=0; i<headerLines.size(); i++){
				printf("[%d] %s \n",i,headerLines[i]);
				if(strstr(headerLines[i], "Content-Type")){
					sscanf(headerLines[i], "Content-Type: %s", contentType);
				}
			}
			
			printf("\nHeaders are finished now reading the file\n");
			printf("Content Type is %s\n",contentType);
		}
		
		int readval;
		while((readval=read(wSocket,buffer,MAX_MSG_SZ))>0){
			if(downloadTimes==1){
				write(1,buffer,readval);
			}
		}
		
		headerLines.clear();

		if(debug){
			printf("\nClosing socket.\n\n");
		}
		
		if(close(wSocket)==SOCKET_ERROR){
			success=false;
			printf("\nCould not close socket.\n");
			return 0;
		}
		if(success){
			successCount++;
		}
	}
	
	if(downloadTimes>1){
		printf("Downloaded %d times.\n\n",successCount);
	}
}
