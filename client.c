#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

#define BUFLEN 100
#define GET 0
#define POST 1

char *r_path, *text, *total_url, *url, *port, *url_path, *path, *request, *rbuf;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void freeMemory()
{ //free all the allocated memory in program
    if (port != NULL)
        free(port);
    if (r_path != NULL)
        free(r_path);
    if (url_path != NULL)
        free(url_path);
    if (url != NULL)
        free(url);
    if (total_url != NULL)
        free(total_url);
    if (path != NULL)
        free(path);
    if (text != NULL)
        free(text);
    if (request != NULL)
        free(request);
    if (rbuf != NULL)
        free(rbuf);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void error(char *msg)
{ //function to print errors of system functions
    freeMemory();
    perror(msg);
    printf("\n");
    exit(0);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void commandError()
{ //function for errors in the input
    fprintf(stderr, "Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 …>] <URL> \n");
    freeMemory();
    exit(0);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void getHostError(char *msg)
{ //    prints error that happend in gethostbyname function
    herror("gethostbyname");
    printf("\n");
    freeMemory();
    exit(0);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
char *substring(char *s, int start, int end)
{ // this function return a sub string of s from start to end indexes
    if (start == end)
        return NULL;
    char *ret = (char *)calloc(end - start + 1, sizeof(char)); // allocate memory
    if (ret == NULL)
        error("couldn't allocate memory\n");
    int i = start, j = 0;
    for (; i < end; i++, j++)
        ret[j] = s[i]; //copy the necessary chars
    ret[j] = '\0';
    return ret;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int existValidP(int size, char *argv[])
{ /** this function checks if the argv contain the -p part and if its valid, if it is, the function returns 1 to
        announce that the type of the request is POST, if not returns 0;
    **/
    for (int i = 0; i < size; i++)
    {
        if (strcmp(argv[i], "-p") == 0)
        {
            if (i == size - 1)
                commandError();
            // -p appears in the url
            if ((strcmp(argv[i + 1], "-r") == 0))
                if ((argv[i + 2][0] >= '0' && argv[i + 2][0] <= '9')) //must be text after -p
                    commandError();

            return POST;
        }
    }
    return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int validR(int size, char *argv[])
{ /** this function checks if the argv contain the -r part and if its valid:
    1. if there is a number after
    2. if there are arguments in the number written
    3. if the arguments are in the form name=value
    if not returns a messege and exit
    **/
    int num;
    for (int i = 0; i < size; i++)
    {
        if (strcmp(argv[i], "-r") == 0 && strcmp(argv[i - 1], "-p") != 0) // check that the -r is not the text of -p
        {
            if (i == size - 1)
                commandError();
            if (strcmp(argv[i + 1], "-p") == 0)
                commandError();
            if (argv[i + 1][0] < '0' || argv[i + 1][0] > '9') // if the number is not in the scope
                commandError();
            num = argv[i + 1][0] - 48; // number of parameter related to -r
            if (num == 0)
            {
                if (argv[i + 2] != NULL && strchr(argv[i + 2], '=') != NULL) // if there are more paramenter than the number writen
                    commandError();
                else
                    return i;
            }
            i += 2;
            int j = 0;
            for (j = 0; j < num; j++)
            {
                if (argv[i + j] == NULL) // if there are less parameters than necessary
                    commandError();
                if (argv[i + j][0] == '=') // if the parameter starts with =
                    commandError();
                if (strchr(argv[i + j], '=') != NULL && strlen(strchr(argv[i + j], '=')) < 2) // if there is no value (name=value)
                    commandError();
                if (strchr(argv[i + j], '=') == NULL) // if the parameter is not in form: name=value
                    commandError();
            }
            if (argv[i + j] != NULL && strchr(argv[i + j], '=') != NULL) // if there are more paramenter than the number writen
                commandError();

            i -= 2;
            return i;
        }
    }
    // check that there are no arguments of -r when -r doesn't exist
    for( int i=0 ; i< size; i++)
        if( strchr(argv[i], '=') != NULL)
            commandError();
    return -1;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int check_valid_input(int size, char *argv[])
{ // this function check the validity of the command received, and returns the type of the request- GET=0 /POST=1
    int type = existValidP(size, argv);
    int valR = validR(size, argv);
    if (valR == -1)
        ;
    return type;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int size_of_r_path(char *argv[], int idx, int size)
{ // this function returns the length of the paths that comes in the arguments after -r
    int length = 0;
    for (int j = idx, k = 0; k < size; j++, k++)
    {
        length += strlen(argv[j]); //add to sum
    }
    return length;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

char *buildPpart(int size, char *argv[])
{ // this function recieves the argv and his size and returns a string that content the text after -p
    for (int i = 0; i < size; i++)
    {
        if (strcmp(argv[i], "-p") == 0)
        { // builed the body part from -p argument
            text = substring(argv[i + 1], 0, strlen(argv[i + 1]));
            return text;
        }
    }
    return NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

char *buildRpart(int size, char *argv[])
{ // this part returns a string that holds all the paths that passed in the input
    char *path_r;
    int numOfArgsR = 0;
    int r_path_len;
    int idxR = validR(size, argv);
    if (idxR == -1) // if -r doesnt appears in the command sent
    {
        path_r = NULL;
        return path_r;
    }
    numOfArgsR = argv[idxR + 1][0] - 48;
    // builed the path add from -r arguments
    if (numOfArgsR == 0)
        return NULL;

    int idx = idxR + 2;                                       // the first argument that has path beacuse the form is -r -> arg_num -> path...
    r_path_len = size_of_r_path(argv, idx, numOfArgsR);       // returns the length of the argument
    path_r = (char *)calloc((20 + r_path_len), sizeof(char)); // allocate memory for the array in the size
    if (path_r == NULL)                                       // check
        error("Couldn't allocate memory\n");
    for (int j = idx, k = 0; k < numOfArgsR; j++, k++)
    {                            //create the full string of the path from all the arguments after -r
        strcat(path_r, argv[j]); //String concatenation of the path
        if (k != numOfArgsR - 1)
            strcat(path_r, "&");
    }
    strcat(path_r, "\0");

    return path_r;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

char *extractUrlTotal(int size, char *argv[])
{ //returns the rest of the string that comes after http

    for (int i = 0; i < size; i++)
    { // builed the url part
        if (strstr(argv[i], "http://") != NULL || strstr(argv[i], "HTTP://") != NULL)
        {
            if (strcmp(argv[i - 1], "-p") == 0)     // if the http is part of the text after -p
                continue;
            if (strchr(argv[i], '=') != NULL)   // if the http is part of -r arguments
                continue;
            total_url = substring(argv[i], 7, strlen(argv[i]));
            if (total_url == NULL)
                commandError();
            return total_url;
        }
    }
    commandError();
    return NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

char *extractURLpart(char *urlTotal, int size)
{ // this part returns the host part in the url, without port or path
    int start = 0, end;
    for (int i = 0; i < size; i++)
    {
        if (urlTotal[i] == ':' || urlTotal[i] == '/')
        {
            start = 0;
            end = i;
            url = substring(urlTotal, start, end);
            if (url == NULL)
            {
                commandError();
            }

            return url;
        }
    }
    url = substring(urlTotal, start, size);
    if (url == NULL)
        commandError();

    return url;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

char *extractURL_PATHpart(char *urlTotal, int size)
{ //this function return the path part in url if exist
    int start = 0, end;

    if (strchr(urlTotal, '/') == NULL) //there is no path in the url
        return NULL;

    for (int i = 0; i < size; i++)
    {
        if (urlTotal[i] == '/')
        {
            start = i;
            end = size;
            path = substring(urlTotal, start, end);
            return path;
        }
    }
    return NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

char *extractURL_PORTpart(char *urlTotal, int size)
{ //if the url has port, returns a string with the port, otherwise return a string of "80"
    int start = 0, end;

    for (int i = 0; i < size; i++)
    {
        if (urlTotal[i] == ':') // if there are : there is a port in the url
        {
            if (i + 1 < size)
            {
                if (urlTotal[i + 1] == '-') // if the number is less then 0
                    commandError();
                int j = i + 1;
                if (urlTotal[j] == '0')
                    commandError();
                while (urlTotal[j] != '/' && j < size) // Promotes the index to the end of the string or until the path
                {
                    if (urlTotal[j] < '0' || urlTotal[j] > '9') // if there ar other chars in port than
                        commandError();
                    j++;
                }
                // found / or end of string
                start = i + 1;
                end = j;
                port = substring(urlTotal, start, end);

                if (port == NULL)
                    commandError();
                return port;
            }
        }
    }

    port = (char *)calloc(3, sizeof(char));
    if (port == NULL)
        error("couldn't allocate memory\n");
    strcpy(port, "80");

    return port;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

char *buildRequest(int type, char *path, char *url, char *text)
{ // this function build the HTTP request with all the relevant arguments
    char *request_type;
    char *host = "Host: ";
    char *protocol = " HTTP/1.0\r\n";

    if (type == GET)
        request_type = "GET ";
    else
        request_type = "POST ";

    int request_length = 0;
    if (text != NULL)
        request_length += strlen(text);
    if (path != NULL)
        request_length += strlen(path);
    if (url != NULL)
        request_length += strlen(url);

    request_length += strlen(protocol) + strlen(request_type) + strlen(host) + 50;
    request = (char *)calloc(request_length, sizeof(char));
    if (request == NULL)
        error("Couldn't allocate memory\n");

    //create the full request with all the parts we have splited]

    strcat(request, request_type);
    if (path != NULL)
        strcat(request, path);
    else
    {
        strcat(request, "/");
    }

    strcat(request, protocol);

    strcat(request, host);
    if (url != NULL)
        strcat(request, url);
    strcat(request, "\r\n");

    if (type == POST)
    { // create the body part if it is a post type
        char *body = (char *)calloc(strlen(text) + 50, sizeof(char));
        if (body == NULL)
            error("couldn't allocate memory\n");
        strcat(body, "Content-length:");
        char text_len[2];
        sprintf(text_len, "%ld", strlen(text));
        strcat(body, text_len);
        strcat(body, "\r\n\r\n");
        strcat(body, text);
        strcat(request, body);
        free(body);
    }
    else // if there is no body, needs to put \r\n twice
        strcat(request, "\r\n");

    return request;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

char *fullPath(char *url_path, char *r_path, int path_len)
{ //this function build a full path consist of url path and -r path
    if (path_len == 0)
        return NULL;

    char *path = (char *)calloc((10 + path_len), sizeof(char)); //will hold the full path
    if (path == NULL)                                           //check
        error("Couldn't allocate memory\n");

    if (url_path != NULL){
        strcpy(path, url_path);
    }
    if( url_path != NULL && r_path != NULL)
        strcat(path, "?");

    if (r_path != NULL)
        strcat(path, r_path);
    return path;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int main(int argc, char *argv[])
{

    if (argc == 1)
        commandError();

    int type = check_valid_input(argc, argv); //check that the input is valid and return GET = 0, POST =1
    r_path = buildRpart(argc, argv);          //hold the r path
    text = buildPpart(argc, argv);            // hold the text after -p
    total_url = extractUrlTotal(argc, argv);  //total url, include port and path if exist
    int total_url_len = strlen(total_url);

    url = extractURLpart(total_url, total_url_len);           // only the host part of the url
    port = extractURL_PORTpart(total_url, total_url_len);     // holds the port of the server from the url or 80 as default
    url_path = extractURL_PATHpart(total_url, total_url_len); //holds the path in th url if exist

    int path_len = 0;
    if (url_path != NULL)
        path_len += strlen(url_path);
    if (r_path != NULL)
        path_len += strlen(r_path);

    path = fullPath(url_path, r_path, path_len);

    int rc; /* system calls return value storage */
    int sockfd;
    rbuf = (char *)calloc(BUFLEN + 1, sizeof(char));
    if (rbuf == NULL)
        error("couldn't allocate memory\n");
    struct sockaddr_in serv_addr;
    struct hostent *server;

    //create socket for the client
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("socket failed");

    // change string url to IP number of the server
    server = gethostbyname(url);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(EXIT_FAILURE);
    }

    // update some variables
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(atoi(port));

    //connect to server
    rc = connect(sockfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (rc < 0)
        error("connect failed:");

    request = buildRequest(type, path, url, text);
    printf("HTTP request =\n%s\nLEN = %ld\n", request, strlen(request));

    // send and then receive messages from the server
    if ((rc = write(sockfd, request, strlen(request))) < 0)
        error("write failed:");

    int response_length = 0;
    rc = 1;
    while (rc > 0)
    {
        rc = read(sockfd, rbuf, BUFLEN);
        if (rc < 0)
            // couldnt read from server
            error("read() failed");

        else
        { // still read from the server
            rbuf[rc] = '\0';
            printf("%s", rbuf);
            response_length += rc;
        }
    }
    printf("\n Total received response bytes: %d\n", response_length);
    freeMemory();
    close(sockfd);
    return 0;
}
