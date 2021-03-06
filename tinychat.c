/*
 * tinychat.c - [Starting code for] a web-based chat server.
 *
 * Based on:
 *  tiny.c - A simple, iterative HTTP/1.0 Web server that uses the 
 *      GET method to serve static and dynamic content.
 *   Tiny Web server
 *   Dave O'Hallaron
 *   Carnegie Mellon University
 */
#include "csapp.h"
#include "dictionary.h"
#include "more_string.h"

void doit(int fd);
void *go_doit(void *connfdp);
dictionary_t *read_requesthdrs(rio_t *rp);
void read_postquery(rio_t *rp, dictionary_t *headers, dictionary_t *d);
void parse_query(const char *uri, dictionary_t *d);
void serve_form(int fd, const char *pre_content);
void request_form(int fd, const char *conversation);
void serve_convo(int fd, const char *name, const char *topic, char *conversation);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);
static void print_stringdictionary(dictionary_t *d);

static dictionary_t *convo;
static dictionary_t *request_convo;

int main(int argc, char **argv) 
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  convo = make_dictionary(COMPARE_CASE_SENS, free);
  request_convo = make_dictionary(COMPARE_CASE_SENS, free);

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);

  /* Don't kill the server if there's an error, because
     we want to survive errors due to a client. But we
     do want to report errors. */
  exit_on_error(0);

  /* Also, don't stop on broken connections: */
  Signal(SIGPIPE, SIG_IGN);

  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    if (connfd >= 0) {
      Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                  port, MAXLINE, 0);
      printf("Accepted connection from (%s, %s)\n", hostname, port);

      int *connfdp;
      pthread_t th;
      connfdp = malloc(sizeof(int));
      *connfdp = connfd;
      Pthread_create(&th, NULL, go_doit, connfdp);
      Pthread_detach(th);
    }
  }
}

void *go_doit(void *connfdp)
{
   int connfd = *(int *)connfdp;
   free(connfdp);
   doit(connfd);
   Close(connfd);
   return NULL;
}

/*
 * doit - handle one HTTP request/response transaction
 */
void doit(int fd) 
{
  char buf[MAXLINE], *method, *uri, *version;
  rio_t rio;
  dictionary_t *headers, *query;

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);
  if (Rio_readlineb(&rio, buf, MAXLINE) <= 0)
    return;
  printf("%s", buf);
  
  if (!parse_request_line(buf, &method, &uri, &version)) {
    clienterror(fd, method, "400", "Bad Request",
                "TinyChat did not recognize the request");
  } else {
    if (strcasecmp(version, "HTTP/1.0")
        && strcasecmp(version, "HTTP/1.1")) {
      clienterror(fd, version, "501", "Not Implemented",
                  "TinyChat does not implement that version");
    } else if (strcasecmp(method, "GET")
               && strcasecmp(method, "POST")) {
      clienterror(fd, method, "501", "Not Implemented",
                  "TinyChat does not implement that method");
    } else {
      headers = read_requesthdrs(&rio);

      /* Parse all query arguments into a dictionary */
      query = make_dictionary(COMPARE_CASE_SENS, free);

      parse_uriquery(uri, query);
      print_stringdictionary(query);
      if (!strcasecmp(method, "POST")) {
        read_postquery(&rio, headers, query);

    }

      /* For debugging, print the dictionary */
      print_stringdictionary(query);

      if(strcasecmp(method, "POST")){

      /* The start code sends back a text-field form: */
      	if(starts_with("/conversation", uri)){
      		char *topic = dictionary_get(query, "topic");
  			char *conversation = dictionary_get(request_convo, topic);

  			request_form(fd, conversation);
      	} else if(starts_with("/say", uri)){
      		char *user = dictionary_get(query, "user");
  			char *topic = dictionary_get(query, "topic");
  			char *conversation = dictionary_get(convo, topic);
  			char *request_conversation = dictionary_get(request_convo, topic);
  			char *content = dictionary_get(query, "content");

      		if(conversation == NULL){
	  		conversation = ""; 	
	  		request_conversation = "";	
		  	}

		  	if(content == NULL){
		  			content = "";
		  			conversation = append_strings(conversation, strdup(content), NULL);
		  			request_conversation = append_strings(request_conversation, strdup(content), NULL);
		  		} else{

		  			conversation = append_strings(conversation, strdup("<p>"), user, strdup(": "), content, strdup("</p>"), NULL);
		  			request_conversation = append_strings(request_conversation, user, strdup(": "), content, "\r\n", NULL);
		  		}

		  		dictionary_set(convo, (const char *)topic, conversation);
		  		dictionary_set(request_convo, (const char *)topic, request_conversation);

		  		print_stringdictionary(convo);	
		  		serve_convo(fd, (const char *)user, (const char *)topic, conversation); 	
		  	
      	} else{
      		serve_form(fd, "Welcome to TinyChat");
    	}
      

  } else if(strcasecmp(method, "GET")){
  	printf("%s\n", uri);
  	char *user = dictionary_get(query, "user");
  	char *topic = dictionary_get(query, "topic");
  	char *conversation = dictionary_get(convo, topic);
  	char *request_conversation = dictionary_get(request_convo, topic);
  	char *content = dictionary_get(query, "content");

  	if(conversation == NULL){
  		conversation = ""; 	
  		request_conversation = "";	
  	}

  	if(content == NULL || !strcasecmp(content, "")){
  			content = "";
  			conversation = append_strings(conversation, strdup(content), NULL);
  			request_conversation = append_strings(request_conversation, strdup(content), NULL);
  		} else{

  			conversation = append_strings(conversation, strdup("<p>"), user, strdup(": "), content, strdup("</p>"), NULL);
  			request_conversation = append_strings(request_conversation, user, strdup(": "), content, "\r\n", NULL);
  		}

  		dictionary_set(convo, (const char *)topic, conversation);
  		dictionary_set(request_convo, (const char *)topic, request_conversation);

  		print_stringdictionary(convo);
  		printf("%s\n", "Here");
  		serve_convo(fd, (const char *)user, (const char *)topic, conversation); 	
  }


      /* Clean up */
      free_dictionary(query);
      free_dictionary(headers);
    }

    /* Clean up status line */
    free(method);
    free(uri);
    free(version);
  }
}

/*
 * read_requesthdrs - read HTTP request headers
 */
dictionary_t *read_requesthdrs(rio_t *rp) 
{
  char buf[MAXLINE];
  dictionary_t *d = make_dictionary(COMPARE_CASE_INSENS, free);

  Rio_readlineb(rp, buf, MAXLINE);
  printf("%s", buf);
  while(strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    parse_header_line(buf, d);
  }
  
  return d;
}

void read_postquery(rio_t *rp, dictionary_t *headers, dictionary_t *dest)
{
  char *len_str, *type, *buffer;
  int len;
  
  len_str = dictionary_get(headers, "Content-Length");
  len = (len_str ? atoi(len_str) : 0);

  type = dictionary_get(headers, "Content-Type");
  
  buffer = malloc(len+1);
  Rio_readnb(rp, buffer, len);
  buffer[len] = 0;

  if (!strcasecmp(type, "application/x-www-form-urlencoded")) {
    parse_query(buffer, dest);
  }

  free(buffer);
}

static char *ok_header(size_t len, const char *content_type) {
  char *len_str, *header;
  
  header = append_strings("HTTP/1.0 200 OK\r\n",
                          "Server: TinyChat Web Server\r\n",
                          "Connection: close\r\n",
                          "Content-length: ", len_str = to_string(len), "\r\n",
                          "Content-type: ", content_type, "\r\n\r\n",
                          NULL);
  free(len_str);

  return header;
}

/*
 * serve_form - sends a form to a client
 */
void serve_form(int fd, const char *pre_content)
{
  size_t len;
  char *body, *header;
  

  printf("%s\n", "serve_form");
  body = append_strings("<html>",
  						"<body>\r\n",
  						"<div align=\"center\">",
                        "<p>Welcome to TinyChat</p>",
                        "\r\n<form action=\"conversation\" method=\"post\"",
                        " enctype=\"application/x-www-form-urlencoded\"",
                        " accept-charset=\"UTF-8\">\r\n",
                        "Name: <input type=\"text\" name=\"user\"><br>\r\n",
                        "Topic: <input type=\"text\" name=\"topic\"><br><br>\r\n",
                        "<input type=\"submit\" value=\"Join Conversation\">\r\n",
                        "</form></div></body></html>\r\n",
                        NULL);
  
  len = strlen(body);

  /* Send response headers to client */
  header = ok_header(len, "text/html; charset=utf-8");
  Rio_writen(fd, header, strlen(header));
  printf("Response headers:\n");
  printf("%s", header);

  free(header);

  /* Send response body to client */
  Rio_writen(fd, body, len);

  free(body);
}

void request_form(int fd, const char *conversation)
{
  size_t len;
  char *body, *header;
  
  body = append_strings(conversation, NULL);
  
  len = strlen(body);

  /* Send response headers to client */
  header = ok_header(len, "text/plain; charset=utf-8");
  Rio_writen(fd, header, strlen(header));
  printf("Response headers:\n");
  printf("%s", header);

  free(header);

  /* Send response body to client */
  Rio_writen(fd, body, len);

  free(body);
}

/*
 * serve_convo - sends a form to a client
 */
void serve_convo(int fd, const char *user, const char *topic, char *conversation)
{
  size_t len;
  char *body, *header;

  body = append_strings("<html><body>\r\n",
  						"<div align=\"center\">",
                        "<h1>TinyChat - ",topic,"</h1></div>",
                        "\r\n<form action=\"conversation?topic=", topic, "\" method=\"post\"",
                        " enctype=\"application/x-www-form-urlencoded\"",
                        " accept-charset=\"UTF-8\">\r\n",
                        conversation, "<br>", user,
                        ": <input type=\"text\" name=\"content\">\r\n",
                        "<input type=\"hidden\" name=\"user\" value=\"",user,"\">",
                        "<input type=\"hidden\" name=\"topic\" value=\"",topic,"\">",
                        "<input type=\"submit\" value=\"Send\">\r\n",
                        "</form></body></html>\r\n",
                        NULL);
  
  len = strlen(body);

  /* Send response headers to client */
  header = ok_header(len, "text/html; charset=utf-8");
  Rio_writen(fd, header, strlen(header));
  printf("Response headers:\n");
  printf("%s", header);

  free(header);

  /* Send response body to client */
  Rio_writen(fd, body, len);

  free(body);
}

/*
 * clienterror - returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
  size_t len;
  char *header, *body, *len_str;

  body = append_strings("<html><title>Tiny Error</title>",
                        "<body bgcolor=""ffffff"">\r\n",
                        errnum, " ", shortmsg,
                        "<p>", longmsg, ": ", cause,
                        "<hr><em>The Tiny Web server</em>\r\n",
                        NULL);
  len = strlen(body);

  /* Print the HTTP response */
  header = append_strings("HTTP/1.0 ", errnum, " ", shortmsg,
                          "Content-type: text/html; charset=utf-8\r\n",
                          "Content-length: ", len_str = to_string(len), "\r\n\r\n",
                          NULL);
  free(len_str);
  
  Rio_writen(fd, header, strlen(header));
  Rio_writen(fd, body, len);

  free(header);
  free(body);
}

static void print_stringdictionary(dictionary_t *d)
{
  int i, count;

  count = dictionary_count(d);
  for (i = 0; i < count; i++) {
    printf("%s=%s\n",
           dictionary_key(d, i),
           (const char *)dictionary_value(d, i));
  }
  printf("\n");
}
