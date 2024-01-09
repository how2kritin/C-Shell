#include "../main/headers.h"

// References: How to implement TCP sockets in C -> https://www.educative.io/answers/how-to-implement-tcp-sockets-in-c AND tutorial 4
// https://stackoverflow.com/questions/37231114/connect-to-website-purely-by-raw-socket-connection
// https://stackoverflow.com/questions/73661830/how-get-google-com-web-page-using-c-socket
// https://stackoverflow.com/questions/22077802/simple-c-example-of-doing-an-http-post-and-consuming-the-response
// strtok help (to detect empty line), but instead, found a better alternative -> https://stackoverflow.com/questions/57186657/how-to-detect-empty-line-with-strtok
// getaddrinfo() to get IP addresses of server -> https://man7.org/linux/man-pages/man3/getaddrinfo.3.html AND man getaddrinfo


void iMan(char* token){
    // Initialize variables:
    int sockFD;
    char host[PATH_MAX] = "man.he.net", URL[PATH_MAX] = {}, port[] = "80"; // Port 80 for HTTP.
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    memset(&hints, 0, sizeof(hints)); // Reset this struct at the start.
    sprintf(URL, "/?topic=%s&section=all", token);

    // Open a socket:
    if((sockFD = socket(PF_INET, SOCK_STREAM, 0)) == -1){
        fprintf(stderr, RED "ERROR: Failed to open a client socket. Please ensure that you have sufficient privileges to do so!\n" CRESET);
        close(sockFD);
        return;
    }

    // Set up getaddrinfo
    hints.ai_family = AF_UNSPEC; // Basically, use IPv4 or IPv6, depending on availability.
    hints.ai_socktype = SOCK_STREAM; // using TCP
    hints.ai_flags = 0;
    hints.ai_protocol = 0; // Use any protocol.

    if (getaddrinfo(host, port, &hints, &result) != 0) {
        perror("ERROR: Unable to retrieve address info from server");
        fprintf(stderr, BRED "Kindly ensure that you have a working, stable internet connection!\n" CRESET);
        return;
    }

    // THE FOLLOWING WAS OBTAINED DIRECTLY FROM man getaddrinfo examples! (the part where I connect to the server, atleast.)
    /* getaddrinfo() returns a list of address structures.
        Try each address until we successfully connect(2).
        If socket(2) (or connect(2)) fails, we (close the socket
        and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sockFD = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockFD == -1) continue;
        if (connect(sockFD, rp->ai_addr, rp->ai_addrlen) != -1) break; // Connected successfully.
        close(sockFD); // In case connect fails.
    }

    freeaddrinfo(result); // We no longer need this struct ptr.

    if (rp == NULL) { // Could not connect to a single address returned.
        fprintf(stderr, RED "ERROR: Failed to connect to the given host. Please ensure that the hostname is correct.\n" CRESET);
        return;
    }

    // To send GET request to the server:
    char request[2*BUFSIZ] = {};
    sprintf(request, "GET /%s HTTP/1.1\r\n"
                     "Host: %s\r\n"
                     "\r\n", URL, host);
    if(send(sockFD, request, strlen(request), 0) == -1){
        perror("Unable to send message to server");
        close(sockFD);
        return;
    }

    // Receive response from the server.
    ssize_t totalBufSize = 10000*BUFSIZ, totalReceived = 0, numBytesRec = 0;
    char* msgReceived = (char*)malloc(totalBufSize*sizeof(char));
    do{
        numBytesRec = recv(sockFD, msgReceived+totalReceived, totalBufSize - 1 - totalReceived, 0);
        if(numBytesRec == -1){
            perror("Received no response from server");
            free(msgReceived);
            close(sockFD);
            return;
        }
        else if(numBytesRec == 0) break; // We received the entirety of the response.
        totalReceived += numBytesRec;
    } while(totalReceived < totalBufSize-1);

    if(totalReceived == totalBufSize-1)
        fprintf(stderr,
                BYEL "Failed to receive the entirety of the response from the server, due to insufficient buffer size.\n"
                "Will continue to print whatever was received.\n"
                "Consider increasing the buffer size to prevent this issue.\n\n" CRESET);
    msgReceived[totalBufSize-1] = '\0';

    // Close the socket:
    close(sockFD);

    // Print response from the server, after some processing and formatting as given in the mini project 1 doc -> we need at least the NAME, SYNOPSIS and DESCRIPTION.
    const char* delimiters = "\n\n";
    char* headerEnd = strstr(msgReceived, delimiters); // Check for where header ends, i.e., first occurrence of "\n\n".
    if(!headerEnd){
        fprintf(stderr, RED "ERROR:\n"
               "\t\t\tNo such command!\n" CRESET);
        free(msgReceived);
        return;
    }
    headerEnd[0] = '\0';
    char* headEnd = strstr(headerEnd + strlen(delimiters), delimiters); // Check where head ends, i.e., second occurrence of "\n\n".
    char* prevBlock = headerEnd, *currBlock = headEnd;

    bool isHead = true; // To not print the head of html file.
    int numPrinted = 0; // Removing the header, head and footer, if nothing is printed on stdout, then that means the manpage doesn't exist for that command.
    while(currBlock){ // This will ensure that the footer (with copyright info of the man.he.net webpages, useless to us) won't be printed either.
        currBlock[0] = '\0';
        if(!isHead) printf("%s\n\n", prevBlock + strlen(delimiters)), numPrinted++;
        else isHead = false;
        prevBlock = currBlock;
        currBlock = strstr(currBlock + strlen(delimiters), delimiters); // Check for where head ends, i.e., second occurrence of "\n\n".
    }

    if(numPrinted == 0) fprintf(stderr, RED "ERROR:\n\t\t\tNo such command!\n" CRESET);

    free(msgReceived);
}