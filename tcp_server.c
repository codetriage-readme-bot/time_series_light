#include "tcp_server.h"

struct tcp_server *tcp_server_create(int port) {	
	struct tcp_server *server; 
	if( port < 0 ) {
		return NULL;
	}
	server = malloc(sizeof(struct tcp_server));
	if (!server)
		return NULL;
	ipaddr addr = iplocal(NULL, port, 0);
  server->sk   = tcplisten(addr, 10);
	if (!server->sk) {
		free(server);
		return NULL;
	}
	return server;
}

int tcp_server_accept(struct tcp_server* server) {
	while (1) {
		tcpsock as = tcpaccept(server->sk, -1);
    if(!as)
       return -1;
		go(tcp_process_data(server, as));
	}
}


static int process_line(char *item, char **parts, char *sep, int n_parts) {
  int i ;
  char *cursoer = NULL;
  for(i = 0; i < n_parts ; i++) {
    char *str; 
     if( i == 0) {
       str = strtok_r(item, ":", &cursoer);
     } else  {
       str = strtok_r(NULL, ":", &cursoer);
     }
     if(!str) {
       printf("Can't break up string\n");
       break;
     }
     parts[i] = str;
  }
  return i;
}

static int write_data(struct tcp_server* server, char **parts) {
  const short TIME_POS = 1;
  const short NAME_POS = 2;
  const short VALUE_POS = 3;
  time_t m_time;
  float value;
  char *endptr;
  int error;
  m_time = strtol(parts[TIME_POS], &endptr, 10);
  if (endptr && *endptr != '\0') {
    printf("Can't get time %d.\n", *endptr);
    return -1;
  }
  value = strtof(parts[VALUE_POS], &endptr);
  if (endptr && *endptr != '\n') {
    printf("Can't get value %f, '%c'\n", value, *endptr);
    return -1;
  }
  error = store_dp(server->ds, parts[NAME_POS], m_time, value);
  if(error < 0) {
    printf("error storing data");
    return -1;
  }
  return 0;
}

static int read_data(struct tcp_server* server, char **parts, tcpsock sk) {
  const short TIME_START_POS = 1;
  const short TIME_END_POS = 2;
  const short NAME_POS = 3;
  time_t s_time;
  time_t e_time;
  char *endptr;
  int error;
  char *pos;
  struct range_query_result r_result;
  char outbuff[200];
  time_t start_date;
  s_time = strtol(parts[TIME_START_POS], &endptr, 10);
  if (endptr && *endptr != '\0') {
    printf("Can't get time %d.\n", *endptr);
    return -1;
  }
  e_time = strtol(parts[TIME_END_POS], &endptr, 10);
  if (endptr && *endptr != '\0') {
    printf("Can't get time %d.\n", *endptr);
    return -1;
  }

  printf("reading data s_time %ld, e_time %ld \n", s_time, e_time);
  pos = strstr(parts[NAME_POS], "\n");
  if (pos) {
    *pos = '\0';
  }

  r_result = get_range(server->ds, parts[NAME_POS], s_time, e_time, &error);
  if(error < 0) {
    printf("error reading data");
    free_range_query(&r_result); 
    return -1;
  }
  start_date = r_result.start_date;
  for(time_t i = s_time;  i <= e_time; i++)
  {
    sprintf(outbuff, "[%ld, %f],", i,  r_result.points[i - start_date]);
    printf("--- %s, %lu----", outbuff, strlen(outbuff));
    tcpsend(sk, outbuff, strlen(outbuff), -1);
  }
  tcpflush(sk, -1);
  free_range_query(&r_result); 
  return 0;
}


coroutine void tcp_process_data(struct tcp_server* server, tcpsock sk) {
  int64_t deadline = now() + tcp_server_timeout;
	char inbuf[256];
	while (1) {
    char *parts[3];   
    int num_process = 0;
		size_t sz = tcprecvuntil(sk, inbuf, sizeof(inbuf), "\n", 1, deadline);
    printf("buff ---- %s\n", inbuf);
    printf("processing connection\n");
		inbuf[sz] = '\0';
		if(errno != 0)
 	 	  goto cleanup;
    num_process = process_line(inbuf, parts, ":", 4);
    printf("-hjdfiojfdsfoiud %c\n", inbuf[0]);
    switch(inbuf[0]) {
      case 'w':
        if (num_process != 4) {
          printf("String is not  4  parts \n");
          continue;
        }
        if( write_data(server, parts) == -1) { 
          goto cleanup;
        }
      break;
      case 'r':
        printf("start reading");
        if (num_process != 4) {
          printf("Reading, String  is not 4 parts \n");
          continue;
        }
        if (read_data(server, parts, sk) == -1) {
          goto cleanup;
        }
      break;
    }
	}
	cleanup:
    printf("closing connection");
		tcpclose(sk);
}

void tcp_server_set_ds(struct tcp_server *server, struct data_store *ds) {
  server->ds = ds;
}

void tcp_server_cleanup(struct tcp_server *server)
{
	tcpclose(server->sk);
	free(server);
}
