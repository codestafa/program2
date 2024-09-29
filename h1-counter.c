#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>

// Change printf to getline or more elegant solution
// Change lower to upper to more elegant solution

#define SERVER_PORT "80"
#define MAX_LINE 256

/*
 * EECE 446
 * Mustafa Ali
 * Mohsen Amiri
 * Program 1
 * Fall 2024
 */

/* Custom function that abstracts away the first few parts of connecting with the server */
int lookup_and_connect( const char *host, const char *service );

/* Custom function from Beej that handles partial sends */
int sendall(int s, char *buf, int len)
{
  int total = 0;
  int bytesleft = len;
  int n;

  while(total < len) {
    n = send(s, buf+total, bytesleft, 0);
    if (n == -1) { break; }
    total += n;
    bytesleft -= n;
  }

  len = total;

  return n==-1?-1:0;
}

/* Custom function that handles partial recieves */
int recvall(int s, char *buf, int *len) {
  int total = 0;
  int bytesleft = *len;
  int n;
  while (total < *len) {
    n = recv(s, buf + total, bytesleft, 0);
    if (n <= 0) { break; }
    total += n;
    bytesleft -= n;
  }

  *len = total;

  return total;
}

void toUpper(char *str) {
  for (int i = 0; str[i]; i++) {
    str[i] = toupper(str[i]);
  }
}

int main(int argc, char *argv[]) {

  if (argc != 4) {  // Check if the correct number of arguments is provided
    printf("Usage: %s <arg1> <arg2> <arg3>\n", argv[0]);
    return 1;
  }

  char userCommand[100];
  char *host = argv[1];
  const char * port = argv[2];
  int id = atoi(argv[3]);
  char buffer[1024];
  int s;
  int total = 0;
  int count = 0;

  if ((s = lookup_and_connect(host, port)) < 0) {
    exit(1);
  }

  while (1) {
    printf("Enter JOIN, PUBLISH, SEARCH, or exit: \n");
    fgets(userCommand, sizeof(userCommand), stdin);
    printf("i am here")
    // Remove newline character if present
    userCommand[strcspn(userCommand, "\n")] = 0;
    printf("i am here 2")

    // Ensure the string is null-terminated
    userCommand[sizeof(userCommand) - 1] = '\0';
    printf("i am here 3")

    // Debugging: Print the string and its length
    printf("You entered: '%s', length: %ld\n", userCommand, strlen(userCommand));
    printf("i am here 4")

    // Join logic
    if (strcmp(userCommand, "JOIN") == 0 || strcmp(userCommand, "join") == 0) {
      printf("Joining...\n");
      break; // Make sure this is not commented out
    }
    printf("i am here 5")

    // Exit logic
    if (strcmp(userCommand, "EXIT") == 0 || strcmp(userCommand, "exit") == 0) {
      printf("Exiting...\n");
      break;
    }
  }

  }



  //    char *file = "GET /~kkredo/file.html HTTP/1.0\r\n\r\n";
  //    int fileLength = strlen(file);
  //
  //    // Using custom send function for partial sends
  //    if (sendall(s, file, &fileLength) == -1) {
  //        perror("sendall");
  //        printf("%d bytes have been sent\n", fileLength);
  //    }
  //
  //    // Using custom recv function for partial sends
  //    int recvIt = recvall(s, buffer, &chunkSize);
  //
  //    while ((recvIt > 0)) {
  //        buffer[recvIt] = '\0';
  //        char *h1Tag = strstr(buffer, "<h1>");
  //        while (h1Tag) {
  //            // If the h1 tag exists, increment the count
  //            count++;
  //            h1Tag = strstr(h1Tag + 4, "<h1>");
  //        }
  //        total += recvIt; // Total bytes that have been recieved
  //        recvIt = recvall(s, buffer, &chunkSize);
  //    }
  //
  //    close(s);
  //
  //    printf("Number of <h1> tags: %d\n", count);
  //    printf("Number of bytes: %d\n", total);

  return 0;
}



int lookup_and_connect( const char *host, const char *service ) {
  struct addrinfo hints;
  struct addrinfo *rp, *result;
  int s;

  /* Translate host name into peer's IP address */
  memset( &hints, 0, sizeof( hints ) );
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;
  hints.ai_protocol = 0;

  if ( ( s = getaddrinfo( host, service, &hints, &result ) ) != 0 ) {
    fprintf( stderr, "stream-talk-client: getaddrinfo: %s\n", gai_strerror( s ) );
    return -1;
  }

  /* Iterate through the address list and try to connect */
  for ( rp = result; rp != NULL; rp = rp->ai_next ) {
    if ( ( s = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol ) ) == -1 ) {
      continue;
    }

    if ( connect( s, rp->ai_addr, rp->ai_addrlen ) != -1 ) {
      break;
    }

    close( s );
  }
  if ( rp == NULL ) {
    perror( "stream-talk-client: connect" );
    return -1;
  }
  freeaddrinfo( result );

  return s;
}
