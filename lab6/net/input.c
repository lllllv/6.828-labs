#include "ns.h"
#include <inc/lib.h>
extern union Nsipc nsipcbuf;

#define PKTMAP0		0x10000000
#define PKTMAP1		(0x10000000 + PGSIZE)

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.

	int r = sys_page_alloc(0, (void *)PKTMAP0, PTE_U|PTE_W|PTE_P);
    if (r < 0)
		panic("fn-input: could not allocate page of memory");

	r = sys_page_alloc(0, (void *)PKTMAP1, PTE_U|PTE_W|PTE_P);
	if (r < 0)
		panic("fn-input: could not allocate page of memory");

    struct jif_pkt *pkt0 = (struct jif_pkt *)PKTMAP0;
	struct jif_pkt *pkt1 = (struct jif_pkt *)PKTMAP1;
	uint32_t size0, size1, t = 0;

	
	while(1)
	{
		if(t++ % 2)
		{
			r = sys_packet_recv(pkt0->jp_data, &size0);
			if(!r)
			{
				pkt0->jp_len = size0;
				ipc_send(ns_envid, NSREQ_INPUT, (void*)pkt0, PTE_P|PTE_W|PTE_U);
			}
		}
		else
		{
			r = sys_packet_recv(pkt1->jp_data, &size1);
			if(!r)
			{
				pkt1->jp_len = size1;
				ipc_send(ns_envid, NSREQ_INPUT, (void*)pkt1, PTE_P|PTE_W|PTE_U);
			}
		}
	}
}
