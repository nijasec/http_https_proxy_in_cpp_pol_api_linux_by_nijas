/*
 * Fastest HTTP/HTTPS proxy implementain using poll api 
 * Created By Nijas
 * 
 * */
#include <stdio.h>

#include <iostream>

#include <thread>

#include <sys/socket.h>

#include <stdlib.h>

#include <netinet/in.h>

#include <string.h>

#include <unistd.h>

#include <arpa/inet.h>

#include <pthread.h>

#include <signal.h>

#include <chrono>

#include <netdb.h>

#include <poll.h>

#define BUFSIZE 65536

using namespace std;

pthread_mutex_t lock;
//class  for client handle
void outs(const char * msg) {
  pthread_mutex_lock( & lock);
  cout << msg << endl;
  pthread_mutex_unlock( & lock);
}
char * substr(const char * src, int m, int n) {
  // get length of the destination string
  int len = n - m;

  // allocate (len + 1) chars for destination (+1 for extra null character)
  char * dest = (char * ) malloc(sizeof(char) * (len + 1));

  // extracts characters between m'th and n'th index from source string
  // and copy them into the destination string
  for (int i = m; i < n && ( * src != '\0'); i++) {
    * dest = * (src + i);
    dest++;
  }

  // null-terminate the destination string
  * dest = '\0';

  // return the destination string
  return dest - len;
}
char * trimwhitespace(char * str) {
  char * end;

  // Trim leading space
  while (isspace((unsigned char) * str)) str++;

  if ( * str == 0) // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while (end > str && isspace((unsigned char) * end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}

class HandleClient {

  public:

    void handleClient(int client) {

      char buff[1024];
      int len;
      char IP[8];
      char * request;

      len = read(client, buff, sizeof(buff));
      request = (char * ) malloc(len);
      memset(request, 0, sizeof(request));
      for (int k = 0; k < len; k++)
        request[k] = buff[k];

      // strcpy(request,buff);
      if (len > 0) {
        outs("Read data from client");
        char * hostbuf;
        int serverfd, serverport;
        hostbuf = (char * ) malloc(256);
        char * sstr = substr(buff, 0, 7);
        //outs(buff);
        extractHost(buff, hostbuf, len);
        if (strcmp(sstr, "CONNECT") == 0) {
          outs("Connect method");

          //next find port number
          char * host, * port;

          host = (char * ) malloc(256);
          memset(host, 0, sizeof(host));
          port = (char * ) malloc(256);
          memset(port, 0, sizeof(host));

          host = trimwhitespace(strtok(hostbuf, ":"));
          port = strtok(NULL, " ");

          serverport = atoi(port);

          printf("the port is: %d", serverport);
          outs(host);
          outs(port);

          calculateIP(client, IP, host);
          outs(IP);
          serverfd = connectToServer(IP, serverport);
          if (serverfd > 0) {
            outs("connected");
          }
          const char * reply = "HTTP/1.1 200 Connection established\r\n\r\n";
          write(client, reply, 1024);
          relay(client, serverfd);
        } else { // GET POST Methods
          serverport = 80;
          char * host, * port;

          host = (char * ) malloc(256);
          memset(host, 0, sizeof(host));
          host = trimwhitespace(hostbuf);
          calculateIP(client, IP, host);
          outs(IP);
          serverfd = connectToServer(IP, serverport);
          if (serverfd > 0) {
            outs("connected");
          }
          // printf("writinf length:%d",len);
          outs(request);
          //printf("data is:%s",request);
          write(serverfd, request, len);

          relay(client, serverfd);

        }
        // close(client);

      }

    }
  void extractHost(char * buff, char * hostbuf, int len) {
    int a = 0, j = 0, i;
    for (i = 0; i < len; i++) {
      if (buff[i] == 'H') {
        if (buff[i + 1] == 'o' && buff[i + 2] == 's' && buff[i + 3] == 't') {

          for (j = i + 5; buff[j] != '\r'; j++)
            hostbuf[a++] = buff[j];

        }
      }

    }
  }

  void relay_usingpoll(int fd0, int fd1) {
    int timeout;

    int nfds = 2, current_size = 0, i, j, len;
    struct pollfd fds[2];
    int rc, readerfd, writerfd, conn_close = 0;
    char buffer[BUFSIZE];

    //initalising poll
    memset(fds, 0, sizeof(fds));
    fds[0].fd = fd0;
    fds[0].events = POLLIN;
    fds[1].fd = fd1;
    fds[1].events = POLLIN;
    timeout = (3 * 60 * 1000);
    do {
      // outs("waiting on poll");
      rc = poll(fds, nfds, timeout);
      if (rc < 0) {
        perror("  poll() failed");
        break;
      }
      if (rc == 0) {
        outs("  poll() timed out.  End program.\n");
        break;
      }

      if (fds[0].revents == 0) {
        readerfd = 1;
        writerfd = 0;
        //  outs("read from server");
      } else {
        readerfd = 0;
        writerfd = 1;
        // outs("read from clienr");
      }

      rc = read(fds[readerfd].fd, buffer, sizeof(buffer));
      //  rc=recv(fds[readerfd].fd,buffer,sizeof(buffer),0);    
      if (rc < 0) {
        if (errno != EWOULDBLOCK) {

          outs("  rcv failed\n");
          conn_close = 1;
          //  close(fds[readerfd].fd);
          //close_conn = TRUE;
        }
        break;
      }
      if (rc == 0) {
        outs("  Connection closed end now\n");
        //close_conn = TRUE;
        conn_close = 1;
        break;
      }

      /*****************************************************/
      /* Data was received                                 */
      /*****************************************************/
      len = rc;
      //  printf("  %d bytes received\n", len);

      /*****************************************************/
      /* Echo the data back to the client                  */

      /*****************************************************/

      rc = write(fds[writerfd].fd, buffer, len);
      // rc = send(fds[writerfd].fd, buffer, len, 0);
      if (rc < 0) {
        outs("  send() failed");
        close(fds[writerfd].fd);
        conn_close = 1;
        break;
      }

    } while (1);
    if (conn_close) {
      close(fds[readerfd].fd);
      close(fds[writerfd].fd);

    }

  }

  void relay(int client, int remote) {
    relay_usingpoll(client, remote);

    return;

  }

  char getByte(int socket) {
    char buf[1], n;
    if (socket > 0) {
      n = read(socket, buf, 1);
      //cout<<"no. of bytes read"<<n<<std::endl;
      if (n > 0)
        return buf[0];
      else
        return 0x00;
    }
  }

  int connectToServer(char * ip, int port) {
    int sock;
    char * p;
    // outs(ip);
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      printf("\n Socket creation error \n");
      return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    //printf("port=%d", port);
    //outs(p);

    // Convert IPv4 and IPv6 addresses from text to binary form 
    if (inet_pton(AF_INET, ip, & serv_addr.sin_addr) <= 0) {
      printf("\nInvalid address/ Address not supported \n");
      return -1;
    }
    try {
      if (connect(sock, (struct sockaddr * ) & serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
      }

    } catch (char * msg) {
      cout << "error";
    }
    //cout << "connected" << endl;
    return sock;

  }
  int byte2int(char b) {
    int res = b;
    if (res < 0) res = (int)(0x100 + res);
    return res;
  }
  int calcPort(char Hi, char Lo) {

    return ((byte2int(Hi) << 8) | byte2int(Lo));
  }
  void calculateIP(int client, char * p, char * addr) {

    int i;
    struct hostent * host_entry;
    char * hostbuffer;

   

      host_entry = gethostbyname(addr);
      if (host_entry == NULL)
        outs("host entry null");
      else
        hostbuffer = inet_ntoa( * ((struct in_addr * ) host_entry -> h_addr_list[0]));

      sprintf(p, "%s", hostbuffer);

  

  }

 
  std::thread handleThread(int c) {
    return std::thread([ = ] {
      handleClient(c);
    });
  }

};

//class for main server
class ServerListener {

  public:
    bool isValid;
  void runServer(int port) {
    int i, server_fd, conn_num = 0;
    struct sockaddr_in address;

    int addrlen = sizeof(address);

    // Creating socket file descriptor 
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
      perror("socket failed");
      exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080 
    /* if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                   &opt, sizeof(opt))) 
     { 
         perror("setsockopt"); 
         exit(EXIT_FAILURE); 
     } */
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Forcefully attaching socket to the port 8080 
    if (bind(server_fd, (struct sockaddr * ) & address,
        sizeof(address)) < 0) {
      perror("bind failed");
      exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 5) < 0) {
      perror("listen");
      cout << "listen error";
      exit(EXIT_FAILURE);
    }
    isValid = true;
    std::cout << "....HTTPS Server.... " << std::endl;
    while (isValid) {

      int new_socket;
      if ((new_socket = accept(server_fd, (struct sockaddr * ) & address,
          (socklen_t * ) & addrlen)) < 0) {
        perror("accept error");
        cout << "accept error";
        exit(EXIT_FAILURE);
      }
      HandleClient clientHandler;
      std::thread t = clientHandler.handleThread(new_socket);
      t.detach();
      // cout<<"connection number"<<conn_num++<<endl;

    }

    outs("server loop ending");
    try {
      close(server_fd);
    } catch (char * msg) {
      std::cout << "Error closing sockewt" << msg;
    }

  }
  std::thread mainThread(int port) {
    return std::thread([ = ] {
      runServer(port);
    });
  }

};

void segfault(int signal, siginfo_t * si, void * arg) {
  printf("caught");
  exit(0);
}
void sig_handler(int signum) {
  std::cerr << "error=" << signum;
}
int main(int argc, char ** argv) {
  struct sigaction sa;

  try {
    int x = 0;
    int port = 8080;

    char q;
    ServerListener server;
    //memset(&sa,0,sizeof(struct sigaction))
    //  sig
    printf("HTTP/HTTPS proxy server implementation\n");
    /* this variable is our reference to the second thread */
    //SIG_IGN
    signal(SIGPIPE, sig_handler);
    // server.runServer();
    std::thread mainThread = server.mainThread(port);
    mainThread.detach();
    while (q != 'q') {
      cin >> q;

    }
    std::cout << "program exitiing" << endl;
    server.isValid = false;

  } catch (std::exception & e) {
    cout << "unknmmnwdw error";
  }
  return 0;
}