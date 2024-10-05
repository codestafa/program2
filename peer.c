#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/types.h>

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


typedef struct {
  char **fileNames;  // Array of file names
  int fileCount;     // Number of files found
} FileList;

FileList fileCounter(void) {
  DIR *dir;
  struct dirent *entry;
  char *dirName = "./SharedFiles";  // Directory to scan
  int count = 0;
  int maxFiles = 100;  // Set a maximum number of files to track
  char **charArr = malloc(maxFiles * sizeof(char*)); // Allocate memory for the array of strings

  if (!charArr) {
    perror("malloc");
    exit(EXIT_FAILURE);  // Exit if memory allocation fails
  }

  // Open the directory
  dir = opendir(dirName);
  if (!dir) {
    perror("opendir");
    free(charArr);  // Free allocated memory if directory fails to open
    exit(EXIT_FAILURE);
  }

  // Loop through the directory entries
  while ((entry = readdir(dir)) != NULL && count < maxFiles) {
    // Check if the entry is a regular file
    if (entry->d_type == DT_REG) {  // DT_REG means regular file
      charArr[count] = malloc(strlen(entry->d_name) + 1); // Allocate memory for the file name
      if (!charArr[count]) {
        perror("malloc");
        // Cleanup if allocation fails
        for (int i = 0; i < count; i++) {
          free(charArr[i]);
        }
        free(charArr);
        closedir(dir);
        exit(EXIT_FAILURE);
      }
      strcpy(charArr[count], entry->d_name);  // Copy the file name
      count++;
    }
  }

  // Close the directory stream
  closedir(dir);

  // Create and return a FileList struct
  FileList result;
  result.fileNames = charArr;
  result.fileCount = count;

  return result;
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

int main(int argc, char *argv[]) {

  if (argc != 4) {  // Check if the correct number of arguments is provided
    printf("Usage: %s <arg1> <arg2> <arg3>\n", argv[0]);
    return 1;
  }


  char userCommand[100];
  char *host = argv[1];
  char * port = argv[2];
  char * fileLocation = "SharedFiles";
  int s;
  bool userJoined = false;
  int total = 0;

  int id = atoi(argv[3]);
  char buffer[1024];

  // Join
  unsigned char* idBytes = malloc(5);
  idBytes[0] = 0x00;

  // Publish
  unsigned char* publishMessage = malloc(5);

  FileList files = fileCounter();  // Pass the address of fileCount to store the number of files
  publishMessage[0] = 0x1;

  // Convert id to network byte order (big-endian)
  uint32_t net_id = htonl(id);  // Convert id to 4-byte network byte order
  uint32_t fileCount_htonl = htonl(files.fileCount);  // Convert id to 4-byte network byte order

  // Copy the network byte ordered id into num[1] to num[4]
  memcpy(&idBytes[1], &net_id, 4);

  memcpy(&publishMessage[1], &fileCount_htonl, 4);
  memcpy(&publishMessage[2], &files.fileNames, sizeof(files.fileNames));


  if ((s = lookup_and_connect(host, port)) < 0) {
    exit(1);
  }

  while (1) {
    printf("Enter JOIN, PUBLISH, SEARCH, or exit: \n");

    fgets(userCommand, sizeof(userCommand), stdin);
    userCommand[strcspn(userCommand, "\n")] = 0;
    userCommand[sizeof(userCommand) - 1] = '\0';


    // Join logic
    if ((strcmp(userCommand, "JOIN") == 0 || strcmp(userCommand, "join") == 0) && !userJoined) {
      printf("Joining...\n");
          if (send(s, idBytes, sizeof(idBytes), 0) == -1) {
              perror("sendall");
          }
          else {
            printf("Joined...\n");
            userJoined = true;

          }
    }

    if ((strcmp(userCommand, "PUBLISH") == 0 || strcmp(userCommand, "publish") == 0) && !userJoined) {
      printf("publishing...\n");
      if (send(s, publishMessage, sizeof(publishMessage), 0) == -1) {
        perror("sendall");
      }
      else {
        printf("Published...\n");
      }
    }

    // Exit logic
    if (strcmp(userCommand, "EXIT") == 0 || strcmp(userCommand, "exit") == 0) {
      printf("Exiting...\n");
      break;
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
