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

#define SOCKET_ERROR        -1
#define BUFFER_SIZE         100
#define HOST_NAME_SIZE      255
#define MAX_MSG_SZ      1024

#pragma GCC diagnostic ignored "-Wwrite-strings"

using namespace std;

int main(int argc, char *argv[]) {
	int hSocket;
	struct hostent* pHostInfo;
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
		printf("\nDownload: host, port number, URL, flag");
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
				err = 1;
				break;
			}
		}

		strcpy(strHostName, argv[optind]);
		nHostPort = atoi(argv[optind + 1]);
		url = argv[optind + 2];
	}


}