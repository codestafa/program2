#include <dirent.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int lookup_and_connect( const char *host, const char *service );


typedef struct {
  char **fileNames;  // Array of file names
  int fileCount;     // Number of files found
  int bitCount;      // Total size of all file names + delimiters
} FileList;

FileList fileCounter(void) {
  DIR *dir;
  struct dirent *entry;
  char *dirName = "./SharedFiles"; // Directory to scan
  int count = 0;
  int maxFiles = 100; // Set a maximum number of files to track
  char **charArr = malloc(maxFiles * sizeof(char *)); // Allocate memory for the array of strings

  if (!charArr) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  dir = opendir(dirName);
  if (!dir) {
    perror("opendir");
    free(charArr); // Free memory if directory fails to open
    exit(EXIT_FAILURE);
  }

  // Loop through directory entries
  while ((entry = readdir(dir)) != NULL && count < maxFiles) {
    if (entry->d_type == DT_REG) { // Only regular files
      size_t len = strlen(entry->d_name); // Get the length of the filename
      charArr[count] = malloc(len + 1); // Allocate memory for the filename + null terminator
      if (!charArr[count]) {
        perror("malloc");
        for (int i = 0; i < count; i++) {
          free(charArr[i]);
        }
        free(charArr);
        closedir(dir);
        exit(EXIT_FAILURE);
      }
      strcpy(charArr[count], entry->d_name); // Copy the filename into charArr[count]
      count++;
    }
  }
  unsigned int bitCount = 0;
  for (int i = 0; i < count; i++) {
    bitCount += strlen(charArr[i]); // Include string length + null terminator
  }

  bitCount += count;
  closedir(dir);

  FileList result;
  result.fileNames = charArr;
  result.fileCount = count;
  result.bitCount = bitCount;
  return result;
}


void join(char joinMessage[], int id) {
  joinMessage[0] = 0x00; // Set first byte to JOIN action
  uint32_t net_id = htonl(id); // Convert ID to network byte order
  memcpy(joinMessage + 1, &net_id, 4); // Copy the 4-byte ID
}

FileList publish(char publishMessage[], int id, int messageSize, FileList files) {
  publishMessage[0] = 1;

  uint32_t fileCount_htonl = htonl(files.fileCount);
  memcpy(publishMessage + 1, &fileCount_htonl, 4);

  int offset = 5;
  for (int i = 0; i < files.fileCount; i++) {
    int nameLength = strlen(files.fileNames[i]) + 1;
    memcpy(publishMessage + offset, files.fileNames[i], nameLength);
    offset += nameLength;
  }
  return files;
}

void search(char searchMessage[], char *searchCommand) {
  searchMessage[0] = 0x02;
  printf("Enter file name to search: ");
  fgets(searchCommand, 100, stdin);
  searchCommand[strcspn(searchCommand, "\n")] = 0;

  memcpy(searchMessage + 1, searchCommand, strlen(searchCommand) + 1);

}

void freeFileList(FileList files) {
  for (int i = 0; i < files.fileCount; i++) {
    free(files.fileNames[i]);
  }
  free(files.fileNames);
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    printf("Usage: %s <host> <port> <id>\n", argv[0]);
    return 1;
  }

  char userCommand[100];
  char searchCommand[100];
  char *host = argv[1];
  char *port = argv[2];
  int s;
  bool userJoined = false;
  int id = atoi(argv[3]);

  // Join
  char joinMessage[5];

  // Publish
  FileList files = fileCounter();
  char publishMessage[files.bitCount + 5];

  // Search
  char searchMessage[100];
  char searchResponse[10]; // Increase buffer size for search response

  if ((s = lookup_and_connect(host, port)) < 0) {
    exit(1);
  }

  while (1) {
    printf("Enter JOIN, PUBLISH, SEARCH, or exit: \n");
    fgets(userCommand, sizeof(userCommand), stdin);
    userCommand[strcspn(userCommand, "\n")] = 0; // Remove newline

    if (strcmp(userCommand, "EXIT") == 0 || strcmp(userCommand, "exit") == 0) {
      printf("Exiting...\n");
      close(s);
      break;
    }

    if ((strcmp(userCommand, "JOIN") == 0 || strcmp(userCommand, "join") == 0) && !userJoined) {
      join(joinMessage, id);
      if (send(s, joinMessage, sizeof(joinMessage), 0) == -1) {
        perror("send");
      } else {
        userJoined = true;
      }
    }

    if ((strcmp(userCommand, "PUBLISH") == 0 || strcmp(userCommand, "publish") == 0) && userJoined) {
      publish(publishMessage, id, files.bitCount + 5, files);
      if (send(s, publishMessage, files.bitCount + 5, 0) == -1) {
        perror("send");
      }
    }

    if ((strcmp(userCommand, "SEARCH") == 0 || strcmp(userCommand, "search") == 0) && userJoined) {
      search(searchMessage, searchCommand);
      if (send(s, searchMessage, strlen(searchMessage) + 1, 0) == -1) {
        perror("send");
      } else {
        int recvIt = recv(s, searchResponse, 10,0);
        if (recvIt > 0) {
          searchResponse[recvIt] = '\0'; // Null-terminate the response

          uint32_t peerID;
          uint32_t ipAddress;
          uint16_t port;

          memcpy(&peerID, &searchResponse[0], 4);
          memcpy(&ipAddress, &searchResponse[4], 4);
          memcpy(&port, &searchResponse[8], 2);

          peerID = ntohl(peerID);
          ipAddress = ntohl(ipAddress);
          port = ntohs(port);

          if (peerID != 0) {
            printf("File found at:\n");
            printf("  Peer %u\n", peerID);
            printf("  %u.%u.%u.%u",
                   (ipAddress >> 24) & 0xFF,
                   (ipAddress >> 16) & 0xFF,
                   (ipAddress >> 8) & 0xFF,
                   ipAddress & 0xFF);
            printf(":%u\n", port);
          } else {
            printf("  File not found.\n");
          }
        }
      }
    }
  }

  close(s);
  freeFileList(files);
  return 0;
}


int lookup_and_connect(const char *host, const char *service) {
  struct addrinfo hints;
  struct addrinfo *rp, *result;
  int s;

  /* Translate host name into peer's IP address */
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;
  hints.ai_protocol = 0;

  if ((s = getaddrinfo(host, service, &hints, &result)) != 0) {
    fprintf(stderr, "stream-talk-client: getaddrinfo: %s\n", gai_strerror(s));
    return -1;
  }

  /* Iterate through the address list and try to connect */
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    if ((s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1) {
      continue;
    }

    if (connect(s, rp->ai_addr, rp->ai_addrlen) != -1) {
      break;
    }

  }
  if (rp == NULL) {
    perror("stream-talk-client: connect");
    return -1;
  }
  freeaddrinfo(result);

  return s;
}
