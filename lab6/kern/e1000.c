#include <kern/e1000.h>
#include <kern/pmap.h>

// LAB 6: Your driver code here

int attachfn(struct pci_func *pcif)
{
    pci_func_enable(pcif);

    location = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);

    // trans init
    struct PageInfo * p = page_alloc(1);
    if(!p)
        panic("trans des array page alloc failed in attachfn\n");

    uint32_t trans_des_paddr = page2pa(p);
    trans_des = page2kva(p);
    for(int i = 0; i < 64; i ++)
    {
        p = page_alloc(1);
        if(!p)
            panic("trans buffer page alloc failed in attachfn\n");
        trans_des[i].buffer_addr = page2pa(p);
        trans_des[i].upper.data |= E1000_TXD_STAT_DD;
    }

    
    *(location + E1000_TDBAL/4) = trans_des_paddr;
    *(location + E1000_TDBAH/4) = 0;
    *(location + E1000_TDLEN/4) = 64 * sizeof(struct e1000_tx_desc);
    *(location + E1000_TDT/4) = 0;
    *(location + E1000_TDH/4) = 0;

    *(location + E1000_TCTL/4) = E1000_TCTL_EN | E1000_TCTL_PSP | 0x40100;

    *(location + E1000_TIPG/4) = 10 | 10 << 10 | 10 << 20;
    

    // recv init

    *(location + E1000_RA/4) = 0x12005452;
    *(location + E1000_RA/4 + 1) = 0x00005634;
    *(location + E1000_RA/4 + 1) |= E1000_RAH_AV;
    *(location + E1000_MTA/4) = 0;
    *(location + E1000_IMS/4) = 0;

    p = page_alloc(1);
    if(!p)
        panic("recv des array page alloc failed in attachfn\n");

    uint32_t recv_des_paddr = page2pa(p);
    recv_des = page2kva(p);

    *(location + E1000_RDBAL/4) = recv_des_paddr;
    *(location + E1000_RDBAH/4) = 0;
    *(location + E1000_RDLEN/4) = 128 * sizeof(struct e1000_rx_desc);
    for(int i = 0; i < 128; i++)
    {
        p = page_alloc(1);
        if(!p)
            panic("recv buffer page alloc failed in attachfn\n");
        recv_des[i].buffer_addr = page2pa(p);
        recv_des[i].length = 4096;
    }

    *(location + E1000_RDH/4) = 0;
    *(location + E1000_RDT/4) = 127;

    *(location + E1000_RCTL/4) |= E1000_RCTL_EN;
    *(location + E1000_RCTL/4) ^= E1000_RCTL_LPE;
    *(location + E1000_RCTL/4) |= E1000_RCTL_LBM_NO;
    *(location + E1000_RCTL/4) |= E1000_RCTL_RDMTS_HALF;
    // MO ignored
    *(location + E1000_RCTL/4) |= E1000_RCTL_BAM;
    *(location + E1000_RCTL/4) |= E1000_RCTL_BSEX;
    *(location + E1000_RCTL/4) |= E1000_RCTL_SZ_4096;
    *(location + E1000_RCTL/4) |= E1000_RCTL_SECRC;

    return 0;
}


int send_packet(void* content, uint32_t size)
{
    if(trans_des[*(location + E1000_TDT/4)].upper.data & E1000_TXD_STAT_DD)
    {
        
        trans_des[*(location + E1000_TDT/4)].upper.data ^= E1000_TXD_STAT_DD;

        memcpy(page2kva(pa2page(trans_des[*(location + E1000_TDT/4)].buffer_addr)), content, size);
        trans_des[*(location + E1000_TDT/4)].lower.data |= E1000_TXD_CMD_RS;
        trans_des[*(location + E1000_TDT/4)].lower.data |= E1000_TXD_CMD_EOP;
        trans_des[*(location + E1000_TDT/4)].lower.flags.length = size;

        *(location + E1000_TDT/4) = (*(location + E1000_TDT/4) + 1) % 64;

        return 0;
    }
    else
    {
        return -1;
    }
    
}

int recv_packet(void* buf, uint32_t* size)
{
    if(recv_des[(*(location + E1000_RDT/4) + 1) % 128].status & E1000_RXD_STAT_DD)
    {
        *(location + E1000_RDT/4) = (*(location + E1000_RDT/4) + 1) % 128;
        recv_des[*(location + E1000_RDT/4)].status ^= E1000_RXD_STAT_DD;

        if(!size)
            panic("nullptr cannot be used in fn-recv_packet\n");
        *size = recv_des[*(location + E1000_RDT/4)].length;
        memcpy(buf, page2kva(pa2page(recv_des[*(location + E1000_RDT/4)].buffer_addr)), *size);

        return 0;
    }
    else 
        return -1;
}