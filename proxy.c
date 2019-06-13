#include <stdio.h>
#include "csapp.h"


/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 16777216 
#define MAX_OBJECT_SIZE 8388608 

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_connection_hdr = "Proxy-Connection: close\r\n";



/* cache constructor */
typedef struct cacheNode {
	char* hostname;
	char* path;
	char* port;
	char* obj;
	int objsize;
	struct cacheNode *next;
	struct cacheNode *prev;
}cacheNode;

typedef struct dllcacheNode {
	cacheNode *head;
	cacheNode *tail;
	int listSize;
}dllcacheNode;


void doit( int fd, dllcacheNode *CACHE );
void parse_uri( char* uri, char* hostname, char* path, char* port );
void buildHTTPReq(char* method, char* path, char* version, char* httpReq );
void parse_hdr(char* headReq, char* hostname, char* httpReq, char* port, rio_t *client_rio );
void echo_client(char* hostname, char* port, char* path, char* headReq, int fd, dllcacheNode* CACHE );

/* list function */
void insertHead(dllcacheNode* listPtr, char* hostname, char* port, char* path, int objsize );
void removeTail(dllcacheNode* listPtr );
cacheNode* Cachecheck( dllcacheNode* listPtr, char* hostname, char* port, char* path );

int main(int argc, char **argv)
{

	dllcacheNode *CACHE = (struct dllcacheNode*)malloc(MAX_CACHE_SIZE);
	int listenfd, connfd;
	char hostname[MAXLINE], port[MAXLINE];
	socklen_t clientlen;
	struct sockaddr_storage clientaddr; 

	if (argc != 2) {
		fprintf(stderr, "usage: %s <port> \n", argv[0]);
		exit(1);
	}
	
	Signal(SIGPIPE, SIG_IGN);
	listenfd = Open_listenfd(argv[1]);
	while (1) {
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
		Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE,
			port, MAXLINE, 0);
		printf("Accepted connection from (%s, %s)\n", hostname, port);
		doit( connfd, CACHE );
		Close( connfd );
	}

    return 0;
}
/* Use for perform as information transection in proxy with client and with server*/
void doit(int fd, dllcacheNode *CACHE)
{
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE],
		hostname[MAXLINE]="", path[MAXLINE]="", port[MAXLINE]="",
		httpReq[MAXLINE]="", headReq[MAXLINE]="";

	rio_t rio;
	rio_readinitb(&rio, fd);
	if (!rio_readlineb(&rio, buf, MAXLINE))
		return;
	printf( "%s", buf );
	sscanf( buf, "%s %s %s", method, uri, version );
	if (strcasecmp(method, "GET")) {	
		//printf( "501, Not Implemented Tiny does not implement this method");
		return;
	}

	/* parse uri and split them into hostname, port and path */
	parse_uri(uri, hostname, path, port);

	/* cache condition check to determine use cache or not 
	 * if the request destination is not stored in cache
	 * build the request to ask server for information 
	 * if it is stored, use it without asking server. */
	cacheNode* temp = Cachecheck(CACHE, hostname, port, path);
	if (temp == NULL) {
		/* build the request header */
		buildHTTPReq(method, path, version, httpReq);
		parse_hdr(headReq, hostname, httpReq, port, &rio);

		/* proxy act as a client, ask information for server */
		echo_client(hostname, port, path, headReq, fd, CACHE);
	}

	else {
		/* swap the required cache to head to fit LRU 
		 * I choose to insert a copy of the node and 
		 * delete the old one. */
		insertHead(CACHE, hostname, port, path, temp->objsize);
		memcpy((CACHE->head)->obj, temp->obj, temp->objsize);
		if (temp == CACHE->tail) 
			removeTail(CACHE);

		else {
			/* repair list connection and remove the node */
			(temp->prev)->next = temp->next;
			(temp->next)->prev = temp->prev;
			CACHE->listSize = (CACHE->listSize) - ((CACHE->head)->objsize);
			free(temp->hostname);
			free(temp->port);
			free(temp->path);
			free(temp->obj);
			free(temp);
		}

		/* write the content to browser with the cache content */
		Rio_writen(fd, CACHE->head->obj, CACHE->head->objsize);	
	}
	return;
}

void parse_uri(char* uri, char* hostname, char* path, char* port)
{
	/* pointers use to point to different part of uri */
	char *tempUri ="";
	char *tempUriForPort="";
	char *tempUriForPath="";

	/* check if uri contains "http://", if so use the content after it */
	if (strpbrk(uri, "http://") != NULL) {
		tempUri = strpbrk(uri, "http://");   //http://web.mit.edu:[port]/index.html
		tempUri = tempUri + 7;				 //web.mit.edu:[port]/index.html
	}
	else
		tempUri = uri;
	
	/* check the content after "/" which is the part of path 
	 * if there's a path, copy the path to the string, or give the string a "/" 
	 * set the string split mark at "/" to split path off */
	if (strpbrk(tempUri, "/") != NULL) {
		strcpy(path, strpbrk(tempUri, "/"));         //path: /index.html
		tempUriForPath = strpbrk(tempUri, "/");		 //tempUri: web.mit.edu:port:port\0
		*tempUriForPath = '\0';
	}
	else
		strcpy(path, "/");

	/* check the content after";", which is the part of port 
	 * if it exists, copy it, or set it to 80, set the split mark to split port */
	if (strpbrk(tempUri, ":") != NULL) {			//web.mit.edu:[port]\0
		tempUriForPort = strpbrk(tempUri, ":");     //:[port]
		strcpy(port, tempUriForPort + 1);	        //[port]
		*tempUriForPort = '\0';                     //tempUri: web.mit.edu\0xxx\0xxx
	}
	else
		strcpy(port, "80\0");

	 /* copy rest things to hostname */
	strcpy(hostname, tempUri);  //hostname: web.mit.edu\0

	return;
}

void buildHTTPReq(char* method, char* path, char* version, char*httpReq)
{
	/* build the http request */
	strcat(httpReq, method); //req: get
	strcat(httpReq, " ");    
	strcat(httpReq, path);   //req: get /path
	strcat(httpReq, " ");    
	strcat(httpReq, "HTTP/1.0"); //req: get /path http/1.0
	strcat(httpReq, "\r\n"); //req: get /path http/1.0\r\n

	return;
}

void parse_hdr(char* headReq, char* hostname, char*httpReq, char*port, rio_t *client_rio)

{
	char HostHdr[MAXLINE] = "Host: ";
	char buf[MAXLINE], otherHdr[MAXLINE] = "";
	strcat(HostHdr, hostname);
	strcat(HostHdr, ":");
	strcat(HostHdr, port);
	strcat(HostHdr, "\r\n");

	strcat(headReq, httpReq);
	strcat(headReq, HostHdr);
	strcat(headReq, user_agent_hdr);
	strcat(headReq, connection_hdr);
	strcat(headReq, proxy_connection_hdr);
	while (Rio_readlineb(client_rio, buf, MAXLINE) > 0) {
		if (!(strcmp(buf, "\r\n")))
			break;
		strcat(otherHdr, buf);
		strcat(otherHdr, "\r\n");
	}
	if (strlen(otherHdr) != 0)
		strcat(headReq, otherHdr);
	strcat(headReq, "\r\n");

	return;
}

void echo_client(char* hostname, char* port, char* path, char* headReq, int fd, dllcacheNode* CACHE )
{
	rio_t rio1;
	char buf[MAXLINE];
	memset(buf, '\0', sizeof(buf) - 1);
	/* a space used to copy the buf from reading */
	char *buf4cache = malloc(MAX_OBJECT_SIZE);
	memset(buf4cache, '\0', sizeof(buf4cache) - 1);
	/* use to calculate objsize */
	int objsize = 0;

	int clientfd = Open_clientfd(hostname, port);
	Rio_readinitb(&rio1, clientfd);
	Rio_writen(clientfd, headReq, strlen(headReq));
	
	size_t n;

	/* copy data each by each 
	 * write the data each by each */
	while ((n = Rio_readnb(&rio1, buf, MAXLINE))!= 0) {
		objsize = objsize + n;
		if (objsize < MAX_OBJECT_SIZE) {
			memcpy(buf4cache, buf,n);
			buf4cache += n;
		}
		Rio_writen(fd, buf, n);
	}
	/* put the pointer point back to the beginning */
	buf4cache = buf4cache - objsize;

	Close(clientfd);

	/* copy the heap value to array to invoid segmentation fault*/
	char toCACHE[MAX_OBJECT_SIZE] = "";
	memcpy(toCACHE, buf4cache, objsize);
	free(buf4cache);

	if (objsize < MAX_OBJECT_SIZE ) {
		
		/* calculate the future size to determine if need to remove tail */
		while ((CACHE->listSize+objsize) > MAX_CACHE_SIZE) {
			removeTail(CACHE);
		}
		/* add the content to the cache */
		insertHead(CACHE, hostname, port, path, objsize);
		memcpy((CACHE->head)->obj, toCACHE, objsize);

	}

	return;
}

void insertHead(dllcacheNode *listPtr, char* hostname, char* port, char* path, int size ) {
	/* Create a new heap memory, and initialize the new head node */
	cacheNode *newHead = (struct cacheNode*)malloc(sizeof(struct cacheNode));
	newHead -> hostname = malloc(strlen(hostname)+1);
	strcpy(newHead->hostname, hostname);
	newHead->port = malloc(strlen(port)+1);
	strcpy(newHead->port, port);
	newHead->path = malloc(strlen(path)+1);
	strcpy(newHead->path, path);
	newHead->obj = malloc(size +1 );
	newHead->objsize = size;
	(listPtr->listSize) += size;
	/* if the list is not empty
	* link the new node to the front of old head
	* then complete the list */
	if (listPtr->head != NULL) {
		newHead->prev = NULL;
		newHead->next = listPtr->head;
		(listPtr->head)->prev = newHead;
		listPtr->head = newHead;
	}
	/* if the list is empty
	* add the new node as head and tail*/
	else {
		newHead->prev = NULL;
		newHead->next = NULL;
		listPtr->head = newHead;
		listPtr->tail = newHead;
	}
	return;
}

void removeTail(dllcacheNode *listPtr) {
  /* empty list case, which is handled in main.c*/
  if(listPtr -> head == NULL)
    return;

  int removeSize;
  removeSize = (listPtr->tail)->objsize;
  cacheNode * deadNode = listPtr -> tail;
  /* the case of that the list contains two or more nodes
   * remove the tail and connect the list appropriately 
   * free the head memory */
  if(listPtr -> head != listPtr -> tail){
    listPtr -> tail = deadNode -> prev;
    deadNode -> prev = NULL;
    (listPtr -> tail) -> next = NULL;
	free(deadNode -> hostname);
	free(deadNode->port);
	free(deadNode -> path);
	free(deadNode->obj);
    free(deadNode);
	listPtr->listSize = (listPtr->listSize) - removeSize;
  }

  /* case of only one node in list
   * removing it equals to free the list */
  else if (listPtr -> head == listPtr -> tail && listPtr -> head != NULL){
	deadNode = (listPtr->head) -> next;
	free(listPtr->head->hostname);
	free(listPtr->head->port);
	free(listPtr->head->path);
	free(listPtr->head->obj);
	free(listPtr->head);
        listPtr -> head = NULL;
        listPtr -> tail = NULL;
	listPtr->listSize = 0;
  }

  return;
} 


cacheNode* Cachecheck(dllcacheNode* listPtr, char* hostname, char* port, char* path) {
    /* to check if there's a such node with same content in the list */
	if ((listPtr->head) == NULL)
		return NULL;
	cacheNode* tmp = listPtr->head;
	while (tmp != NULL) {
		if (!(strcasecmp(tmp->hostname, hostname))&& !(strcasecmp(tmp->port, port))
			&& !(strcasecmp(tmp->path, path)))
			return tmp;
		tmp = tmp->next;
	}
	return NULL;
}
