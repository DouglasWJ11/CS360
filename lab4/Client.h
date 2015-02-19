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