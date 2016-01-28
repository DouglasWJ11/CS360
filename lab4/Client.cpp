#include "Client.h"

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

int  main(int argc, char* argv[])
{
    int hSocket;                 /* handle to socket */
    struct hostent* pHostInfo;   /* holds info about a machine */
    struct sockaddr_in Address;  /* Internet socket address stuct */
    long nHostAddress;
    char pBuffer[BUFFER_SIZE];
    char strHostName[HOST_NAME_SIZE];
    int nHostPort;
    string url; 
    vector<char *> headerLines;
    char buffer[MAX_MSG_SZ];
    char contentType[MAX_MSG_SZ];
	
	extern char *optarg;
	int c, times_to_download= 1, err = 0, successCount = 0;
	bool debug = false;
	
	if(argc < 4)
	{
	    printf("\nUsage: client host-name host-port url\n");
        return 0;
	}
	else
	{
		while( ( c = getopt( argc, argv, "c:d") ) != -1)
		{
			switch( c )
			{
				case 'c':
					times_to_download = atoi( optarg );
					break;
				case 'd':
					debug = true;
					break;
				case '?':
					err = 1;
					break;
			}
		}

		strcpy(strHostName,argv[ optind]);
		nHostPort=atoi(argv[optind + 1]);
		url =  argv[optind + 2];
	}
	
	for(int i=0; i < times_to_download; i++)
	{
		bool success = true;
		if(debug)
		{
			printf("\nMaking a socket.");
		}
		/* make a socket */
		hSocket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		if(hSocket == SOCKET_ERROR)
		{			
			success = false;
			printf("\nCould not make a socket\n");
			return 0;
		}
		
		if(debug)
		{
			printf("\nGetting Hostname.");
		}
		/* get IP address from name */
		pHostInfo=gethostbyname(strHostName);
		
		if(pHostInfo == NULL)
		{
			success= false;
			printf("\nInvalid Hostname.\n");
			return 0;
			
		}
		/* copy address into long */
		memcpy(&nHostAddress,pHostInfo->h_addr,pHostInfo->h_length);
		/* fill address struct */
		Address.sin_addr.s_addr=nHostAddress;
		Address.sin_port=htons(nHostPort);
		Address.sin_family=AF_INET;
		
		string request = "GET " + url  + " HTTP/1.0\r\nHost: "+ strHostName + "\r\n\r\n"; 
		char* rPointer = new char[request.length() + 1];
		strcpy(rPointer, request.c_str());

		if(debug)
		{
			printf("\nConnecting to %s (%X) on port %d",strHostName,nHostAddress,nHostPort);
		}
		/* connect to host */
		if(connect(hSocket,(struct sockaddr*)&Address,sizeof(Address))
		   == SOCKET_ERROR)
		{
			printf("\nCould not connect to Host.\n");
			success= false;
			return 0;
		}

		/* 
		** number returned by read() and write() is the number of bytes
		** read or written, with -1 being that an error occured */
		
		write(hSocket,rPointer,request.length());

		if(debug)
		{
			printf("\n=======================\n");
			printf("\nWriting: \n%s",rPointer);
			printf("=======================\n");
		}
		
		// Read the header lines
		GetHeaderLines(headerLines, hSocket , false);

	  
		  
		if(debug)
		{
			printf("\nHeaders: \n");
			for (int i = 0; i < headerLines.size(); i++) 
			{
				printf("[%d] %s\n",i,headerLines[i]);
				if(strstr(headerLines[i], "Content-Type")) 
				{
					sscanf(headerLines[i], "Content-Type: %s", contentType);
				}
			}

			printf("\n=======================\n");
			printf("Headers are finished, now reading the file\n");
			printf("Content Type is %s\n",contentType);
			printf("=======================\n\n");
			
		}
	  
		// read and print the rest of the file
		int rval;
		while((rval = read(hSocket,buffer,MAX_MSG_SZ)) > 0)
		{
			if(times_to_download == 1)
			{
				write(1,buffer,rval);
			}
		}
		
		headerLines.clear();
		
		if(debug)
		{
			printf("\nClosing socket.\n\n");
		}
		/* close socket */
		if(close(hSocket) == SOCKET_ERROR)
		{
			success = false;
			printf("\nCould not close Socket.\n");
			return 0;
		}
		
		if(success)
		{
			successCount++;
		}
	}
	
	if(times_to_download > 1)
	{
		printf("Downloaded %d times.\n\n", successCount);
	}
}
