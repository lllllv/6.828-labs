#include "ns.h"

#include <inc/lib.h>

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver

	
	while(1)
	{
		int r = ipc_recv(0, &nsipcbuf, 0);
		if(r < 0)
			panic("output env ipc recv error:%e\n", r);

		do
		{
			r = sys_packet_send((void*)nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len);
			sys_yield();
		} while (r < 0);
	}
}
