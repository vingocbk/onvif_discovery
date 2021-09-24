#include <stdio.h>
#include <stdlib.h>
// #include "soapStub.h"
#include "soapH.h"
#include "RemoteDiscoveryBinding.nsmap"
#include "wsddapi.h"
#define HTTPSERVER_IMPL
#include "httpserver.h"
#include "http-client-c.h"

#include <signal.h>
#include <stdbool.h>
#include <unistd.h>  //Header file for sleep(). man 3 sleep for details.
#include <string.h>	//memset
#include <errno.h>	//errno
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <unistd.h> 
#include <math.h>
// #include<mathcalls.h>

/*WS-Discovery specialization address and port of UDP*/
#define MULTICAST_GROUP ("239.255.255.250")  
#define PORT (3702) 

#define RESPONSE "Hello, World!"
// #define DISCOVERY_ON "Hello, World!"
// #define RESPONSE "Hello, World!"

pthread_t thread_http, thread_onvif;
struct http_server_s* server;

char* get_address();
char* toArray(int number);
void* http_server(void *vargp);
void* onvif_discovery_server(void *vargp);
int port_http = 8001;

char  g_scopes[] = "onvif://www.onvif.org/Profile/Streaming \
				onvif://www.onvif.org/model/C5F0S7Z0N1P0L0V0 \
				onvif://www.onvif.org/name/ELCOM_IPCAM \
				onvif://www.onvif.org/location/country/Vietnam"; 

// char  g_scopes[] = "onvif://www.onvif.org/name/IPCAM";


const char* host = "239.255.255.250";	
int port = 3702;
int port_onvif;
// bool is_turn_on_discovery = true;

int request_target_is(struct http_request_s* request, char const * target) {
	http_string_t url = http_request_target(request);
	int len = strlen(target);
	return len == url.len && memcmp(url.buf, target, url.len) == 0;
}

void handle_request(struct http_request_s* request) {
	http_request_connection(request, HTTP_AUTOMATIC);
	struct http_response_s* response = http_response_init();
	http_response_status(response, 200);
	if (request_target_is(request, "/discovery/on")) {
		printf("on\n");
		pthread_create(&thread_onvif, NULL, onvif_discovery_server, NULL);
		pthread_join(thread_onvif, NULL);
		http_response_header(response, "Content-Type", "text/plain");
		http_response_body(response, RESPONSE, sizeof(RESPONSE) - 1);
	}
	else if (request_target_is(request, "/discovery/off")) {
		printf("off\n");
		pthread_cancel(thread_onvif);
		http_response_header(response, "Content-Type", "text/plain");
		http_response_body(response, RESPONSE, sizeof(RESPONSE) - 1);
	}
	else {
		http_response_header(response, "Content-Type", "text/plain");
		http_response_body(response, RESPONSE, sizeof(RESPONSE) - 1);
	}
	http_respond(request, response);
}

void handle_sigterm(int signum) {
	(void)signum;
	free(server);
	exit(0);
}


int main(int argc, char** argv)
{
	if(argc >= 2)
	{
		port_onvif = atoi(argv[1]);
	}else{
		port_onvif = 8000;
	}
	printf("port onvif: %d\n",port_onvif);
	printf("port http: %d\n",port_http);
	
	pthread_create(&thread_http, NULL, http_server, NULL);

	struct http_response *hrep = http_get("http://localhost:8200/dvr/v1.0/GetDiscoveryMode", "User-agent:MyUserAgent\r\n"); 
	if(hrep)
	{
		printf("http code: %d\n",hrep->status_code_int);
		printf("http body: %s\n",hrep->body);
		if(!strstr(hrep->body, "NonDiscoverable"))
		{
			pthread_create(&thread_onvif, NULL, onvif_discovery_server, NULL);
			pthread_join(thread_onvif, NULL);
		}
	}
	else{
		printf("\nCant connect to uri -> Auto turn on thread discovery\n");
		pthread_create(&thread_onvif, NULL, onvif_discovery_server, NULL);
		pthread_join(thread_onvif, NULL);
	}
	
    
    pthread_join(thread_http, NULL);
	

	return 0;
}





void* http_server(void *vargp)
{
	signal(SIGTERM, handle_sigterm);
	server = http_server_init(port_http, handle_request);
	http_server_listen(server);
}

void* onvif_discovery_server(void *vargp)
{
	struct soap* serv = soap_new1(SOAP_IO_UDP | SOAP_IO_KEEPALIVE); 
	serv->bind_flags=SO_REUSEADDR;
	serv->connect_flags = SO_BROADCAST; 
	if (!soap_valid_socket(soap_bind(serv, NULL, port, 1000)))
	{
		soap_print_fault(serv, stderr);
		exit(1);
	}	

	if (MULTICAST_GROUP) { 
		struct ip_mreq mreq;
		mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_GROUP);
		mreq.imr_interface.s_addr = htonl(INADDR_ANY);
		if (setsockopt(serv->socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
			printf("setsockopt failed\n");
	      		exit(-1);
		}
	}

	// signal(SIGINT, &sighandler);
	while (1)
	{
		soap_wsdd_listen(serv, 1000);
	}
}



SOAP_FMAC5 int SOAP_FMAC6 SOAP_ENV__Fault(struct soap* soap, char *faultcode, char *faultstring, char *faultactor, struct SOAP_ENV__Detail *detail, struct SOAP_ENV__Code *SOAP_ENV__Code, struct SOAP_ENV__Reason *SOAP_ENV__Reason, char *SOAP_ENV__Node, char *SOAP_ENV__Role, struct SOAP_ENV__Detail *SOAP_ENV__Detail) {
	printf("%s,%d\n",__FUNCTION__, __LINE__);
	return 0;
}


SOAP_FMAC5 int SOAP_FMAC6 __tdn__Hello(struct soap* soap, struct wsdd__HelloType tdn__Hello, struct wsdd__ResolveType *tdn__HelloResponse) {
	printf("%s,%d\n",__FUNCTION__, __LINE__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tdn__Bye(struct soap* soap, struct wsdd__ByeType tdn__Bye, struct wsdd__ResolveType *tdn__ByeResponse) {
	printf("%s,%d\n",__FUNCTION__, __LINE__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tdn__Probe(struct soap* soap, struct wsdd__ProbeType tdn__Probe, struct wsdd__ProbeMatchesType *tdn__ProbeResponse) {
	printf("%s,%d\n",__FUNCTION__, __LINE__);
	return 0;
}

void wsdd_event_Hello(struct soap *soap, unsigned int InstanceId, const char *SequenceId, unsigned int MessageNumber, const char *MessageID, const char *RelatesTo, const char *EndpointReference, const char *Types, const char *Scopes, const char *MatchBy, const char *XAddrs, unsigned int MetadataVersion)
{
    printf("%s,%d\n",__FUNCTION__, __LINE__);
}
 

void wsdd_event_Bye(struct soap *soap, unsigned int InstanceId, const char *SequenceId, unsigned int MessageNumber, const char *MessageID, const char *RelatesTo, const char *EndpointReference, const char *Types, const char *Scopes, const char *MatchBy, const char *XAddrs, unsigned int *MetadataVersion)
{
    printf("%s,%d\n",__FUNCTION__, __LINE__);
}
soap_wsdd_mode wsdd_event_Probe(struct soap *soap, const char *MessageID,
		const char *ReplyTo, const char *Types, const char *Scopes,
		const char *MatchBy, struct wsdd__ProbeMatchesType *matches) {
	printf("%s,%d\n", __FUNCTION__, __LINE__);
	printf("MessageID:%s\n", MessageID);
	printf("ReplyTo:%s\n", ReplyTo);
	printf("Types:%s\n", Types);
	printf("Scopes:%s\n", Scopes);
	printf("MatchBy:%s\n", MatchBy);
	printf("\n");


	soap->header->wsa__RelatesTo = (struct wsa__Relationship*) soap_malloc(
			soap, sizeof(struct wsa__Relationship));

	if (soap->header->wsa__ReplyTo) {
		soap->header->wsa__To = soap->header->wsa__ReplyTo->Address;
	} else {
		soap->header->wsa__To = NULL;
	}
	soap->header->wsa__RelatesTo->__item = soap->header->wsa__MessageID;
	soap->header->wsa__RelatesTo->RelationshipType = NULL;
	soap->header->wsa__RelatesTo->__anyAttribute = NULL;
	soap->header->wsa__Action = "http://schemas.xmlsoap.org/ws/2005/04/discovery/ProbeMatches";
	soap->header->wsa__ReplyTo = NULL;

	char* address = get_address();
	soap_wsdd_init_ProbeMatches(soap, matches);
	soap_wsdd_add_ProbeMatch(soap, matches,
				soap_wsa_rand_uuid(soap),
				"tdn:NetworkVideoTransmitter", g_scopes,
				NULL,
				address,
				10);

	return SOAP_WSDD_MANAGED;
}
 
void wsdd_event_ProbeMatches(struct soap *soap, unsigned int InstanceId, const char *SequenceId, unsigned int MessageNumber, const char *MessageID, const char *RelatesTo, struct wsdd__ProbeMatchesType *matches)
{
    printf("*****************88%s,%d\n",__FUNCTION__, __LINE__);
}

soap_wsdd_mode wsdd_event_Resolve(struct soap *soap, const char *MessageID, const char *ReplyTo, const char *EndpointReference, struct wsdd__ResolveMatchType *match)
{
    printf("%s,%d\n",__FUNCTION__, __LINE__);
    return 0;
}
 
void wsdd_event_ResolveMatches(struct soap *soap, unsigned int InstanceId, const char *SequenceId, unsigned int MessageNumber, const char *MessageID, const char *RelatesTo, struct wsdd__ResolveMatchType *match)
{
    printf("%s,%d\n",__FUNCTION__, __LINE__);
 }


char* get_address(){
	FILE *f;
    char line[100] , *p , *c;
    f = fopen("/proc/net/route" , "r");
    while(fgets(line , 100 , f))
    {
		p = strtok(line , " \t");
		c = strtok(NULL , " \t");
		if(p!=NULL && c!=NULL)
		{
			if(strcmp(c , "00000000") == 0)
			{
				printf("Default interface is : %s \n" , p);
				break;
			}
		}
	}
    //which family do we require , AF_INET or AF_INET6
    int fm = AF_INET;
    struct ifaddrs *ifaddr, *ifa;
	int family , s;
	char host[NI_MAXHOST];

	if (getifaddrs(&ifaddr) == -1) 
	{
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}
	//Walk through linked list, maintaining head pointer so we can free list later
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
	{
		if (ifa->ifa_addr == NULL)
		{
			continue;
		}
		family = ifa->ifa_addr->sa_family;
		if(strcmp( ifa->ifa_name , p) == 0)
		{
			if (family == fm) 
			{
				s = getnameinfo( ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6) , host , NI_MAXHOST , NULL , 0 , NI_NUMERICHOST);
				if (s != 0) 
				{
					printf("getnameinfo() failed: %s\n", gai_strerror(s));
					exit(EXIT_FAILURE);
				}
				printf("address: %s", host);
			}
			printf("\n");
		}
	}
	freeifaddrs(ifaddr);

    int x = port_onvif;
    int size = 0;
    while ( x ) {
		x=x/10;
		size++;
    }
	x = port_onvif;
    char port_array[size];
    for (int i = size - 1; x ; i--) {
		// *port_array[i] = char(x % 10 + '0');
		port_array[i] = x % 10 + '0';
		x = x/10;
	//    printf("address:%d, %c, %d, %d \n",i, port_array[i], size, x);
    }
	port_array[size] = '\0';
	// port_array[3] = '0';
	// port_array[2] = '0';
	// port_array[1] = '0';
	// port_array[0] = '8';
	
	char* local_ip = malloc(100);
	strcpy(local_ip, (char*)"http://");
	strcat(local_ip, (char*)host);
	strcat(local_ip, (char*)":");
	// strcat(local_ip, (char*)"8000");
	strcat(local_ip, (char*)port_array);
	strcat(local_ip, (char*)"/onvif/device_service");
	printf("address: %s \n", local_ip);
	return local_ip;
}
