/*********************************************************************
 *                
 * Filename:      obex_tcp.c
 * Version:       
 * Description:   Do an OBEX PUT over TCP
 * Status:        Experimental.
 * Author:        Dag Brattli <dagb@cs.uit.no>
 * Created at:    Fri Apr 30 19:43:40 1999
 * Modified at:   Sat Nov 20 13:12:54 1999
 * Modified by:   Pontus Fuchs <pontus@tactel.se>
 * 
 *     Copyright (c) 1999 Dag Brattli, All Rights Reserved.
 *     
 *     This program is free software; you can redistribute it and/or 
 *     modify it under the terms of the GNU General Public License as 
 *     published by the Free Software Foundation; either version 2 of 
 *     the License, or (at your option) any later version.
 * 
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *     GNU General Public License for more details.
 * 
 *     You should have received a copy of the GNU General Public License 
 *     along with this program; if not, write to the Free Software 
 *     Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 *     MA 02111-1307 USA
 *     
 ********************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#if _WIN32
#include <winsock.h>
#else

#include <sys/stat.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#endif /* _WIN32 */

#include <obex/obex.h>
#include "obex_put_common.h"
#include "obex_io.h"

obex_t *handle = NULL;
volatile int finished = FALSE;

/*
 * Function get_peer_addr (name, peer)
 *
 *    
 *
 */
gint get_peer_addr(char *name, struct sockaddr_in *peer) 
{
	struct hostent *host;
	u_long inaddr;
        
	/* Is the address in dotted decimal? */
	if ((inaddr = inet_addr(name)) != INADDR_NONE) {
		memcpy((char *) &peer->sin_addr, (char *) &inaddr,
		      sizeof(inaddr));  
	}
	else {
		if ((host = gethostbyname(name)) == NULL) {
			g_error( "Bad host name: ");
			exit(-1);
                }
		memcpy((char *) &peer->sin_addr, host->h_addr,
				host->h_length);
        }
	return 0;
}

/*
 * Function main (argc, )
 *
 *    Starts all the fun!
 *
 */
int main(int argc, char *argv[])
{
	struct sockaddr_in peer;

	obex_object_t *object;
	int ret;

	printf("Send and receive files over TCP OBEX\n");
	if ( ((argc < 3) || (argc > 3)) && (argc != 1) )	{
		printf ("Usage: %s [name] [peer]\n", argv[0]); 
		return -1;
	}

	handle = OBEX_Init(OBEX_TRANS_INET, obex_event, 0);

	if (argc == 1)	{
		printf("Waiting for files\n");
		ret = OBEX_ServerRegister(handle, "OBEX");
		if(ret < 0)
			exit(ret);

		while (!finished) {
			ret = OBEX_HandleInput(handle, 10);
			if (ret < 1) {
				printf("Timeout waiting for connection\n");
				break;
			}
		}
	}
	else {
		/* We are a client */

		get_peer_addr(argv[2], &peer);
		ret = OBEX_TransportConnect(handle, (struct sockaddr *) &peer,
					  sizeof(struct sockaddr_in));

		if (ret < 0) {
			printf("Sorry, unable to connect!\n");
			exit(ret);
		}


		object = OBEX_ObjectNew(handle, OBEX_CMD_CONNECT);
		ret = do_sync_request(handle, object, 0);

		if( (object = build_object_from_file(handle, argv[1])) )	{
			ret = do_sync_request(handle, object, 0);
		}
		else	{
			perror("PUT failed");
		}

		object = OBEX_ObjectNew(handle, OBEX_CMD_DISCONNECT);
		ret = do_sync_request(handle, object, 0);

		printf("PUT successful\n");
	}
	return 0;
}