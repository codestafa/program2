#include <dirent.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/*
 * EECE 446
 * Mustafa Ali
 * Mohsen Amiri
 * Program 2
 * Fall 2024
 */

int lookup_and_connect(const char *host, const char *service);

typedef struct {
  char **fileNames; // Array of file names
  int fileCount;    // Number of files found
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
    exit(EXIT_FAILURE); // Exit if memory allocation fails
  }

  // Open the directory
  dir = opendir(dirName);
  if (!dir) {
    perror("opendir");
    free(charArr); // Free allocated memory if directory fails to open
    exit(EXIT_FAILURE);
  }

  // Loop through the directory entries
  while ((entry = readdir(dir)) != NULL && count < maxFiles) {
    // Check if the entry is a regular file
    if (entry->d_type == DT_REG) { // DT_REG means regular file
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
      strcpy(charArr[count], entry->d_name); // Copy the file name
      count++;
    }
  }

  // Print the number of files found
  printf("File count: %d\n", count);
  unsigned int bitcount = 0;
  // Print the hex representation of file names
  printf("Hex representation of files:\n");
  for (int i = 0; i < count; i++) {
      bitcount = bitcount + strlen(charArr[i]);
    }
    printf("%d ", bitcount);
    printf("\n");


  // Close the directory stream
  closedir(dir);

  // Create and return a FileList struct
  FileList result;
  result.fileNames = charArr;
  result.fileCount = count;

  return result;
}

/* Custom function that handles partial receives */
int recvall(int s, char *buf, int len) {
  int total = 0;
  int bytesleft = len;
  int n;
  while (total < len) {
    n = recv(s, buf + total, bytesleft, 0);
    if (n <= 0) {
      break; // Break on error or connection closed
    }
    total += n;
    bytesleft -= n;
  }
  return total;
}

void join(char joinMessage[], int id, int messageSize) {
  joinMessage[0] = 0x00;  // Set the first byte to 0x00 (JOIN action)

  // Convert the ID to network byte order and copy it into the message
  uint32_t net_id = htonl(id);  // Ensure ID is in network byte order
  memcpy(joinMessage + 1, &net_id, 4);  // Copy the 4-byte ID

  // Print the hex representation of the message
  printf("Hex representation of joinMessage: ");
  printf("Message length: %d\n", messageSize);
  for (int i = 0; i < messageSize; i++) {  // Loop over the actual size (5 bytes)
    printf("0x%02x ", (unsigned char)joinMessage[i]);
  }
  printf("\n");
}

FileList publish(char publishMessage[], int id, int messageSize) {
  FileList files = fileCounter();
  publishMessage[0] = 1;

  uint32_t fileCount_htonl = htonl(files.fileCount);
  memcpy(publishMessage + 1, &fileCount_htonl, 4);

  int offset = 5; // Starts after the first 5 bytes
  for (int i = 0; i < files.fileCount; i++) {
    int nameLength = strlen(files.fileNames[i]) + 1;
    memcpy(publishMessage + offset, files.fileNames[i], nameLength);
    offset += nameLength;
  }
  printf("Hex representation of publishMessage: ");
  for (int i = 0; i < messageSize; i++) {
    printf("0x%02x ", (unsigned char)publishMessage[i]);
  }
  printf("\n");

  return files;
}

void search(char searchMessage[], char *searchCommand) {
  searchMessage[0] = 0x2;

  printf("Enter file name to search: ");
  fgets(searchCommand, 100, stdin);
  searchCommand[strcspn(searchCommand, "\n")] = 0;
  searchCommand[strlen(searchCommand)] = '\0';

  memcpy(searchMessage + 1, searchCommand, strlen(searchCommand) + 1);

  printf("Hex representation of searchMessage: ");
  for (int i = 0; i <= strlen(searchMessage); i++) {
    printf("0x%02x ", (unsigned char)searchMessage[i]);
  }
  printf("\n");
}


void freeFileList(FileList files) {
  for (int i = 0; i < files.fileCount; i++) {
    free(files.fileNames[i]);
  }
  free(files.fileNames);
}

int main(int argc, char *argv[]) {
  if (argc != 4) {  // Check if the correct number of arguments is provided
    printf("Usage: %s <host> <port> <id>\n", argv[0]);
    return 1;
  }

  char userCommand[100];
  char searchCommand[100];
  char *host = argv[1];
  char *port = argv[2];
  int s;
  char searchResponse[10];
  bool userJoined = false;
  int id = atoi(argv[3]);

  // Join
  char joinMessage[5];

  // Publish
  char publishMessage[1200];

  // Search
  char searchMessage[100];   // Properly sized

  if ((s = lookup_and_connect(host, port)) < 0) {
    exit(1);
  }

  while (1) {
    printf("Enter JOIN, PUBLISH, SEARCH, or exit: \n");
    fgets(userCommand, sizeof(userCommand), stdin);
    userCommand[strcspn(userCommand, "\n")] = 0; // Remove newline

    // Exit condition
    if ((strcmp(userCommand, "EXIT") == 0 || strcmp(userCommand, "exit") == 0)) {
      printf("Exiting the program...\n");
      close(s); // Close the socket connection
      break;
    }

    // Join logic
    if ((strcmp(userCommand, "JOIN") == 0 || strcmp(userCommand, "join") == 0) && !userJoined) {
      join(joinMessage, id, sizeof(joinMessage));
      printf("Joining...\n");
      if (send(s, joinMessage, sizeof(joinMessage), 0) == -1) {
        perror("send");
      } else {
        printf("Joined...\n");
        userJoined = true;
      }
    }

    // Publish logic
    if ((strcmp(userCommand, "PUBLISH") == 0 || strcmp(userCommand, "publish") == 0) && userJoined) {
      FileList files = publish(publishMessage, id, sizeof(publishMessage));
      printf("Publishing...\n");
      int publishSize = 9; // Start with fixed header size
      for (int i = 0; i < files.fileCount; i++) {
        publishSize += strlen(files.fileNames[i]) + 1; // Calculate total size
      }
      if (send(s, publishMessage, publishSize, 0) == -1) {
        perror("send");
      } else {
        printf("Published...\n");
      }
      freeFileList(files); // Free the file names
    }

    // Search logic
    if ((strcmp(userCommand, "SEARCH") == 0 || strcmp(userCommand, "search") == 0) && userJoined) {
      search(searchMessage, searchCommand);
      printf("Searching...\n");
      if (send(s, searchMessage, strlen(searchMessage), 0) == -1) {
        perror("send");
      } else {
        printf("Searching for file... %s\n", searchCommand);
        int recvIt = recvall(s, searchResponse, strlen(searchResponse));
        while(recvIt > 0) {
          recvIt = recvall(s, searchResponse, strlen(searchResponse));
        }
        if (recvIt == 0) {
          // Extract the peer ID
          uint32_t peerID;
          memcpy(&peerID, &searchResponse[0], 4);
          peerID = ntohl(peerID);  // Convert from network byte order to host byte order

          // Extract the IPv4 address
          uint32_t ipAddress;
          memcpy(&ipAddress, &searchResponse[4], 4);
          ipAddress = ntohl(ipAddress);  // Convert to host byte order

          // Extract the port number
          uint16_t port;
          memcpy(&port, &searchResponse[8], 2);
          port = ntohs(port);  // Convert to host byte order

          if (peerID != 0) {
            printf("File found at:\n");
            printf("Peer %u\n", peerID);
            printf("IP Address: %u.%u.%u.%u\n",
                   (ipAddress >> 24) & 0xFF,
                   (ipAddress >> 16) & 0xFF,
                   (ipAddress >> 8) & 0xFF,
                   ipAddress & 0xFF);
            printf(":%u\n", port);
          } else {
            printf("File not found.\n");
          }
        } else {
          printf("Error receiving search response.\n");
        }
      }
    }
  }

  close(s); // Close the socket
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
