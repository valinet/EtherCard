// Simple UDP listening server
//
// Author: Brian Lee
//
// Copyright: GPL V2
// See http://www.gnu.org/licenses/gpl.html

#include "EtherCard.h"
#include "net.h"

#define gPB ether.buffer

#define UDPSERVER_MAXLISTENERS 8    //the maximum number of port listeners.

typedef struct {
    UdpServerCallback callback;
	uint8_t address[IP_LEN];
    uint16_t port;
    bool listening;
	bool acceptAll;
} UdpServerListener;

UdpServerListener listeners[UDPSERVER_MAXLISTENERS];
byte numListeners = 0;

void EtherCard::udpServerListen(UdpServerCallback callback, uint8_t address[IP_LEN], uint16_t port, bool allAddresses) {
    if(numListeners < UDPSERVER_MAXLISTENERS)
    {
        listeners[numListeners] = (UdpServerListener) {
            callback, 
			{address[0], address[1], address[2], address[3]},
			port, 
			true,
			allAddresses
        };
        numListeners++;
    }
}

void EtherCard::udpServerListen(UdpServerCallback callback, uint16_t port, bool allAddresses) {
	uint8_t my_ip[4];
	copyIp(my_ip, myip);
	udpServerListen(callback, my_ip, port, allAddresses);
}

void EtherCard::udpServerListenOnPort(UdpServerCallback callback, uint16_t port) {
	udpServerListen(callback, port, false);
}

void EtherCard::udpServerPauseListen(uint8_t address[IP_LEN], uint16_t port) {
    for(int i = 0; i < numListeners; i++)
    {
		if(listeners[i].port == port && compareAddresses(listeners[i].address, address)) {
        //if(gPB[UDP_DST_PORT_H_P] == (listeners[i].port >> 8) && gPB[UDP_DST_PORT_L_P] == ((byte) listeners[i].port)) {
            listeners[i].listening = false;
        }
    }
}

void EtherCard::udpServerPauseListenOnPort(uint16_t port) {
	udpServerPauseListen(myip, port);
}

void EtherCard::udpServerResumeListen(uint8_t address[IP_LEN], uint16_t port) {
    for(int i = 0; i < numListeners; i++)
    {
		if(listeners[i].port == port && compareAddresses(listeners[i].address, address)) {
        //if(gPB[UDP_DST_PORT_H_P] == (listeners[i].port >> 8) && gPB[UDP_DST_PORT_L_P] == ((byte) listeners[i].port)) {
            listeners[i].listening = true;
        }
    }
}

void EtherCard::udpServerResumeListenOnPort(uint16_t port) {
	udpServerResumeListen(myip, port);
}

bool EtherCard::udpServerListening() {
    return numListeners > 0;
}

bool EtherCard::udpServerHasProcessedPacket(uint16_t plen) {
    bool packetProcessed = false;
    for(int i = 0; i < numListeners; i++)
    {
		if(listeners[i].listening
			&& gPB[UDP_DST_PORT_L_P] == ((byte) listeners[i].port) 
			&& gPB[UDP_DST_PORT_H_P] == (listeners[i].port >> 8)
			&& (listeners[i].acceptAll || memcmp(gPB + IP_DST_P, listeners[i].address, 4) == 0))
        //if(gPB[UDP_DST_PORT_H_P] == (listeners[i].port >> 8) && gPB[UDP_DST_PORT_L_P] == ((byte) listeners[i].port) && listeners[i].listening)
        {
			uint8_t dst_ip[4];
			copyIp(dst_ip, gPB + IP_DST_P);
			
            uint16_t datalen = (uint16_t) (gPB[UDP_LEN_H_P] << 8)  + gPB[UDP_LEN_L_P] - UDP_HEADER_LEN;
            listeners[i].callback(
				dst_ip,
                listeners[i].port,
                gPB + IP_SRC_P,
                (gPB[UDP_SRC_PORT_H_P] << 8) | gPB[UDP_SRC_PORT_L_P],
                (const char *) (gPB + UDP_DATA_P),
                datalen);
            packetProcessed = true;
        }
    }
    return packetProcessed;
}
