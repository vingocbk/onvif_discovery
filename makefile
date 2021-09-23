CFLAGS=-g -I.
LDFLAGS=-lm -lpthread
PLUGIN_OBJS=wsaapi.o wsddapi.o
onvif_discovery: onvif_discovery.o soapC.o stdsoap2.o soapServer.o soapClient.o $(PLUGIN_OBJS) -lpthread

clean:
	rm *.o
	rm onvif_discovery
	
 

