CFLAGS=-g -I.
LDFLAGS=-lm
PLUGIN_OBJS=wsaapi.o wsddapi.o
onvif_discovery: onvif_discovery.o soapC.o stdsoap2.o soapServer.o soapClient.o $(PLUGIN_OBJS) 

clean:
	rm onvif_discovery
	rm *.o
 

