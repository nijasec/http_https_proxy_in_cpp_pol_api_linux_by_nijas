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

struct settings{
    
    int port=8080;
    int enableproxy=0;
    char http_proxy[1024];
    int httpport=80;
    
}defsettings;
using namespace std;
pthread_mutex_t lock;
struct logger
{
 char *date;
 const  char *msg;
};
int checkHost(char *host) //firewall
{
    FILE *firewall=fopen("nijas_firewall_file.txt","r+");
    if(firewall==NULL)
        firewall=fopen("nijas_firewall_file.txt","w+");
    char buff[1024];
    fread(buff,sizeof(buff),1,firewall);
     pthread_mutex_lock( & lock);
 
  //cout<<buff<<endl;
  pthread_mutex_unlock( & lock);
   
    fclose(firewall);
    return 0;
}
 char** str_split(char* a_str, const char a_delim,int len,int *c)// split string into tokens as array
    {
    char** result    = 0;
    int count     = 0;
    int k=0,i,j=0;
    char* tmp        = a_str,*t1;
    t1= (char * )malloc(len);
    for(i=0;i<len;i++)//let me calculate how many elements req.
            if (a_delim ==tmp[i])count++;   
            
           
            if(count==0){
                *c=0;
                return NULL;
            }
            else
                *c=count+1;
    result=(char **)malloc(sizeof(char*) * (count+1)); //add one  to copy last section
    for(i=0;i<len;i++)
    {
        j=0;
        while(tmp[i]!=a_delim && i<len){            t1[j++]=tmp[i++];        }
        t1[j]='\0'; //appending a null character
    result[k++]=strdup(t1); //duplicate copy stroing into result
      }
    free(t1);
    return result;
    }

void logwriter(const char *data)//logging
{
    struct logger log;
    time_t now;
	time(&now);
	char *date = ctime(&now);
    FILE *logfile=fopen("nijas_hp_log.txt","a");
	date[strlen(date) - 1] = '\0';
   fprintf(logfile,"%s: %s\n",date,data);
   fclose(logfile);
 }
void outs(const char * msg) 
{ //logging
  pthread_mutex_lock( & lock);
  logwriter(msg);
  pthread_mutex_unlock( & lock);
}
void outsp(const char * msg) 
{ //print on screen
  pthread_mutex_lock( & lock);
  cout << msg << endl;
  pthread_mutex_unlock( & lock);
}

//calcluate ip outside
void calculateIP( char * p, char * addr) 
{
    struct hostent * host_entry;
    char * hostbuffer;
      host_entry = gethostbyname(addr);
      if (!host_entry)
        outs("host entry null");
      else
        hostbuffer = inet_ntoa( * ((struct in_addr * ) host_entry -> h_addr));
      sprintf(p, "%s", hostbuffer);
  }

char * substr(const char * src, int m, int n) 
{ //funtion to find substr
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
char * trimwhitespace(char * str) 
{ //whitespace 

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

//function to uodate settingsfile

int updateSettings(struct settings &temp){
    
     FILE *config;
       config=fopen("nijas_proxy_settings.txt","w");
          

   
    
   
    if (NULL == config)
    {
         perror("opening config");
        return (-1);
    }
 
    fwrite(&temp,sizeof(struct settings),1,config);
 
    fclose(config);
    return 1;
}

//function to read settings
int readSettings(struct settings &temp)
{
    FILE* config=fopen("nijas_proxy_settings.txt","r");
    fread(&temp, sizeof(struct settings), 1, config);
     printf("port = %d enable=%d remoteproxy=%s remoteport=%d\n",temp.port,temp.enableproxy,temp.http_proxy,temp.httpport); 
  //  server.port=atoi(st[])
  fclose(config);
}

class ControlHandleClient 
{//hanlde control clients
    public:
      void extractGET(char *buff,int len)
      {
          int k=0,tokensize=0;
          char *temp;
          temp=(char*) malloc(len);
          for(int i=0;i<len;i++)
          {
              if(buff[i]=='?')
              {
                  int j=i+1;
                  while(buff[j]!=' '){
                      temp[k++]=buff[j++];
                      
                  }
                      
                      temp[k]='\0';break;
              }
          }
          outsp(temp);
          char **token=str_split(temp,'&',strlen(temp),&tokensize);
          if(tokensize>0)
          {
          for(k=0;k<tokensize;k++)
          {
              outsp(token[k]);
          }
              
          }
          
      }
      void handleClient(int client) {

      char buff[2048],IP[8];
      int len,tokensize;
      char * request;
         try{
              len = read(client, buff, 2048);
           }catch(exception ex)
        {
            cout<<"read error";
        }
        if(len>0)
        {
           // extractGET(buff,len);
            
            const char *headerresponse="HTTP/1.1 200 OK \r\nServer: Nijas_proxy\r\nContent-Type: text/html charset=utf-8\r\n\r\n";
            FILE *fp=fopen("index.html","r");
            char *content;
            write(client,headerresponse,strlen(headerresponse));
            content = (char*) malloc (sizeof(char)*1024);
            if (content == NULL) {fputs ("Memory error",stderr); exit (2);}
            long ptr=0;
            while(1)
            {
                
               len= fread(content,1,1024,fp);
               if(len<1)
                   break;
                 write(client,content,len);
                 
                 //fseek(fp,)
            }
            
           
            free(content);
            close(client);
            fclose(fp);
        }

      }
      std::thread handleThread(int c) {
        return std::thread([ = ] {
          handleClient(c);
        });
    }
}; 


class HandleClient 
{// handle client

public:
     char* http_proxy;
      int http_port=80;
      int enableproxy=0;
          int extractHost(char * buff, char * hostbuf, int len) {
            int a = 0, j = 0, i,flag=0;
            for (i = 0; i < len; i++) {
              if (buff[i] == 'H') {
                if (buff[i + 1] == 'o' && buff[i + 2] == 's' && buff[i + 3] == 't') {
                flag=1;
                  for (j = i + 6; buff[j] != '\r'; j++)
                    hostbuf[a++] = buff[j];

                    hostbuf[a++]='\0';
                    break;
                }
              }

            }
            return flag;
          }
  void relay_usingpoll(int fd0, int fd1) {
    int timeout,nfds = 2, current_size = 0, i, j, len, rc, readerfd, writerfd;;
    struct pollfd fds[2];
    char buffer[BUFSIZE];
    memset(fds, 0, sizeof(fds));
    fds[0].fd = fd0;
    fds[0].events = POLLIN;
    fds[1].fd = fd1;
    fds[1].events = POLLIN;
    timeout = (3 * 60 * 1000);
    do {
      rc = poll(fds, nfds, timeout);
      if (rc < 0) {
       outs("  poll() failed");
        break;
      }
      if (rc == 0) {
        outs("  poll() timed out.  End program.\n");
        break;
      }

      if (fds[0].revents == 0) {
        readerfd = 1;
        writerfd = 0;
          } else {
        readerfd = 0;
        writerfd = 1;
        }

      rc = read(fds[readerfd].fd, buffer, sizeof(buffer));
      if (rc < 0) {
        if (errno != EWOULDBLOCK) {
            outs("  rcv failed\n");
              }
        break;
      }
      if (rc == 0) {
        outs("  Connection closed end now\n");
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
      if (rc < 0) {
        outs("  send() failed");
              break;
      }

    } while (1);
   }

  void relay(int client, int remote) {
    relay_usingpoll(client, remote);
    return;
 }
  int connectToServer(char * ip, int port) {
    int sock;
    char * p;
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      printf("\n Socket creation error \n");
    return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
     // Convert IPv4 and IPv6 addresses from text to binary form 
    
    if (inet_pton(AF_INET, ip, & serv_addr.sin_addr) <= 0) {
        printf("got invalid address\n");
        return -1;
    }
    try {
      if (connect(sock, (struct sockaddr * ) & serv_addr, sizeof(serv_addr)) < 0) {
    
       printf("\nConnection Failed to  \n");
       return -1;
      }

    } catch (char * msg) {
      cout << "error";
    }
    //cout << "connected" << endl;
    return sock;

  }
 
 

};

void clientThread(HandleClient &hc,int client)
{
     char buff[1024];
      int len;
      char IP[8];
      char * request;
     
       
          int tokensize;
try{
      len = read(client, buff, 1024);
   }catch(exception ex)
{
    cout<<"read error";
}
      request = (char * ) malloc(len);//store copy of recvd data 
      memset(request, '\0', sizeof(request));
      for (int k = 0; k < len; k++)
        request[k] = buff[k];

int serverfd, serverport;
      if (len > 10) {
          
        outs("Read data from client");
        
        if(hc.enableproxy)
       {
          //outsp(hc.http_proxy);
           calculateIP(IP,hc.http_proxy);
//outsp(IP);
           serverfd=hc.connectToServer(IP,hc.http_port);
           if(serverfd>0)
           {
               write(serverfd,request,len);
               read(serverfd,buff,1024);
               write(client,buff,1024);
               hc.relay(client,serverfd);
              close(serverfd); 
           }
           close(client);
           
           
           
           return;
       }
        char * hostbuf;
        char **tokens;
        
        hostbuf = (char * ) malloc(len);
        memset(hostbuf,'\0',sizeof(hostbuf));
        char * sstr = substr(buff, 0, 7);
        int a=hc.extractHost(buff, hostbuf, len);
       if(a==0)//"Host: " property not found
       {
           close(client);
           free(hostbuf);
           free(request);
           return;
       }
       
       
        if (strcmp(sstr, "CONNECT") == 0) {
          outs("Connect method");

          //next find port number
          char * host, * port;
         
         // outsp(hostbuf);
          //char *p="onnumilla thoru istas";
          tokens=str_split(hostbuf,':',strlen(hostbuf),&tokensize);// www.google.com:443 seperating into two array
           
          if(tokensize!=0)
          {     
             
            
          host = tokens[0];
          port = tokens[1];
          serverport = atoi(port);
          checkHost(host);
          calculateIP(IP, host);
          outs(IP);
         // outsp(IP);
          serverfd = hc.connectToServer(IP, serverport);
          
       
          if (serverfd > 0) {
            outs("connected");
          
         
          const char * reply = "HTTP/1.1 200 Connection established\r\n\r\n";
          write(client, reply, strlen(reply));
          hc.relay(client, serverfd);
          }
          }
          
        } else { // GET POST Methods
          serverport = 80;//default http port
          char * host, * port;
          tokens=str_split(hostbuf,':',strlen(hostbuf),&tokensize);
          if(tokensize!=0)
          {
              port=tokens[1];
              serverport=atoi(port);
              //outsp(port);
          }
       
          calculateIP(IP, hostbuf);
          outs(IP);
          serverfd = hc.connectToServer(IP, serverport);
        
      
          if (serverfd > 0) {
           
          write(serverfd, request, len);

         hc.relay(client, serverfd);
          close(serverfd);
          }
         
    
        }
        // 
  free(hostbuf);
  free(tokens);
      }
        
         
        close(client);
    
}
//class for main server
class ServerListener 
{

  public:
    bool isValid;
    int server_fd;
    int port=8080;
    int enableproxy=0;
    char *httpproxy;
    int httpport=80;
    void finish()
    {
        isValid=false;
        close(server_fd);
    }
 

};
// notice that the object is passed by reference
void servthread(ServerListener& o) 
{
     int i,  conn_num = 0;
    struct sockaddr_in address;

    int addrlen = sizeof(address);

    // Creating socket file descriptor 
    if ((o.server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
      perror("socket failed");
      exit(EXIT_FAILURE);
    }
    
   
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(o.port);

    // Forcefully attaching socket to the port 8080 
    if (bind(o.server_fd, (struct sockaddr * ) & address,
        sizeof(address)) < 0) {
      perror("bind failed");
      exit(EXIT_FAILURE);
    }
    if (listen(o.server_fd, 5) < 0) {
      perror("listen");
      cout << "listen error";
      exit(EXIT_FAILURE);
    }
    o.isValid = true;
    std::cout << "....HTTPS Server.... " << std::endl;
    while (o.isValid) {

      int new_socket;
      if ((new_socket = accept(o.server_fd, (struct sockaddr * ) & address,
          (socklen_t * ) & addrlen)) < 0) {
        perror("accept error");
        cout << "accept error";
      //  exit(EXIT_FAILURE);
      }
      
      //exit(0);
      HandleClient clientHandler;
      clientHandler.enableproxy=o.enableproxy;
      clientHandler.http_proxy=o.httpproxy;
      clientHandler.http_port=o.httpport;
      std::thread t(clientThread,ref(clientHandler),new_socket);
      t.detach();
      // cout<<"connection number"<<conn_num++<<endl;

    }

    outsp("server loop ending");
    
    
    
    
} 

class Controlserver
{
    
    public:
    bool isValid;
     int server_fd;
    
     void finish()
    {
        isValid=false;
        close(server_fd);
       // server.finish();
         //serverThread.join();
        
        
    }
   
    void startServer(ServerListener server)
    {
        outsp("Loading config");
      //  char **settings;
        
   struct settings wst={8083,0,"testproxy.com",80};
   int r= updateSettings(wst);
    if(r>0)
        outsp("success");
   struct settings stt;
    readSettings(stt);
    server.port=stt.port;
    server.enableproxy=stt.enableproxy;
    server.httpproxy=stt.http_proxy;
    server.httpport=stt.httpport;
    
          std::thread t1(servthread, std::ref(server));
       t1.detach();
       // sleep(5);
    //    std::thread serverThread = this->server.mainThread(port);
       
    }
    
    void closeServer(ServerListener &server)
    {
        server.finish();
    }
    
    void runServer(int port) {
   
     
   
    

  }
 
  

};
 


void segfault(int signal, siginfo_t * si, void * arg) 
{
  printf("caught");
  exit(0);
}
void sig_handler(int signum) {
  std::cerr << "error=" << signum;
}


void controlthread(Controlserver &control,ServerListener &server,int port)
{
     int i,  conn_num = 0;
    struct sockaddr_in address;

    int addrlen = sizeof(address);

    // Creating socket file descriptor 
    if ((control.server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
      perror("socket failed");
      exit(EXIT_FAILURE);
    }
    
    //lets open settings file
    
   
        char argport[256];
        char *http_proxy;
  /*  FILE *fptr;
    if ((fptr = fopen("nijas_proxy_settings.txt", "r")) == NULL)
    {
        printf("Error! opening file");
        // Program exits if file pointer returns NULL.
        exit(1);         
    }
    // reads text until newline 
   // fscanf(fptr,"%[^\n]", c);
      fscanf(fptr,"port=%s", argport);
      
      outsp(argport);
      
      fscanf(fptr,"http_proxy=%s", http_proxy);
      outsp(http_proxy);
   // fscanf(fptr,"%s", c);
     // outsp(c);
    fclose(fptr);
    */
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
    if (bind(control.server_fd, (struct sockaddr * ) & address,
        sizeof(address)) < 0) {
      perror("bind failed");
      exit(EXIT_FAILURE);
    }
    if (listen(control.server_fd, 5) < 0) {
      perror("listen");
      cout << "listen error";
      exit(EXIT_FAILURE);
    }
    control.isValid = true;
    std::cout << "....Control Server started.... " << std::endl;
  control.startServer(ref(server));
 
    while (control.isValid) {

      int new_socket;
      if ((new_socket = accept(control.server_fd, (struct sockaddr * ) & address,
          (socklen_t * ) & addrlen)) < 0) {
        perror("accept error");
        cout << "accept error";
      //  exit(EXIT_FAILURE);
      }
      
      //exit(0);
      ControlHandleClient clientHandler;
      std::thread t = clientHandler.handleThread(new_socket);
      t.detach();
      // cout<<"connection number"<<conn_num++<<endl;

    }

    outsp("control server loop ending");
   outsp("let me close server ");
   server.finish();
  
    
}
int main(int argc, char ** argv) {
  struct sigaction sa;

  try {
    int x = 0,port = 8080,controlport=8081;
    char q;
    
    Controlserver controlserver;
    ServerListener server;
    printf("HTTP/HTTPS proxy server implementation\n");
    //SIG_IGN
    std::thread controlThread(controlthread, std::ref(controlserver),std::ref(server),controlport);
    signal(SIGPIPE, sig_handler);
    while (q != 'q') {
      cin >> q;

    }
    std::cout << "wating for control server" << endl;
    controlserver.finish();
    controlThread.join();
    

  } catch (std::exception & e) {
    cout << "unknm error";
  }
  return 0;
}