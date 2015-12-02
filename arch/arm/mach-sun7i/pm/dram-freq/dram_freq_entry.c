/*
 * arch/arm/mach-sun7i/dram-freq/mdfs/mdfs_entry.c
 *
 * Copyright (C) 2012 - 2016 Reuuimlla Limited
 * xiechuanrong <xiechr@reuuimllatech.com>
 *
 * SUN7I dram frequency dynamic scaling driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */
#include "../pm_types.h" 
#include "./../pm_config.h"     
#include "../pm.h"

#define SP_IN_SRAM_DRAMFREQ 0xf0007ffc

extern unsigned int save_sp(void);
extern void restore_sp(unsigned int sp);
extern void mem_flush_tlb(void);
extern void mem_preload_tlb(void);
extern char *__bss_start;
extern char *__bss_end;
static void standby_memcpy(void *dest, void *src, int n);

/* parameter for standby, it will be transfered from sys_pwm module */
standy_dram_para_t  g_dram_para;
unsigned char dram_traning_back[DRAM_TRANING_SIZE];
__u32   sp_backup;
     
//#define DRAM_FREQ_CHECKSUM
#ifdef DRAM_FREQ_CHECKSUM
void dump(int *addr, int length);
void checksum(int *addr, int length, int flag, int *pcrc);
int sum[2048];
#endif

/*
*********************************************************************************************************
*                                   STANDBY MAIN PROCESS ENTRY
*
* Description: standby main process entry.
*
* Arguments  : arg  pointer to the parameter that transfered from sys_pwm module.
*
* Returns    : none
*
* Note       :
*********************************************************************************************************
*/
int __attribute__((section(".dram_main"))) dram_freq_main(unsigned int freq_point, 
                    standy_dram_para_t *pdram_para, void * tmp_para) 
{
    char    *tmpPtr;
    int     *penter_addr = tmp_para;
    __u32   sp_ram = SP_IN_SRAM_DRAMFREQ;

    tmpPtr = (char *)&__bss_start;
    dram_suspend_flag = 0;
    //printk("++enter from:%x(%x %x),DMA:%x!\n", penter_addr, *penter_addr, *(penter_addr + 1),*(int*)0xF1c02000);

    if(!pdram_para){
        return -1;
    }

    /* clear bss segment */
    do{*tmpPtr ++ = 0;}while(tmpPtr <= (char *)&__bss_end);

    /* save stack pointer registger, switch stack to sram */
    asm volatile ("mov %0, r13" : "=r"(sp_backup));
    asm volatile ("mov r13, %0" : : "r"(sp_ram));
//    sp_backup = save_sp();
#ifdef DRAM_FREQ_CHECKSUM
    checksum((int *)sp_backup, 0x20, 0, sum);
    checksum((int *)0xc0000000, 0x800000, 0, sum+100);
#endif

    /* copy standby parameter from dram */
    standby_memcpy(&g_dram_para, pdram_para, sizeof(g_dram_para));
    
    /* backup dram traning area */
    standby_memcpy((char *)dram_traning_back, (char *)DRAM_BASE_ADDR, DRAM_TRANING_SIZE);

    /*delay_us init*/
    change_runtime_env(1);

    /* flush data and instruction tlb, there is 32 items of data tlb and 32 items of instruction tlb,
    The TLB is normally allocated on a rotating basis. The oldest entry is always the next allocated */
    mem_flush_tlb();
    /* preload tlb for standby */
    mem_preload_tlb();

    //printk("dram_freq_jum :%d!\n", freq_point);
    dram_freq_jum(freq_point, &g_dram_para);

    /* restore dram traning area */
    standby_memcpy((char *)DRAM_BASE_ADDR, (char *)dram_traning_back, DRAM_TRANING_SIZE);
#ifdef DRAM_FREQ_CHECKSUM
    checksum((int *)sp_backup, 0x20, 1, sum);
    checksum((int *)0xc0000000, 0x800000, 1, sum+100);
#endif
    /* restore stack pointer register, switch stack back to dram */
    restore_sp(sp_backup);
//    printk("restore_sp end:%x!\n", sp_backup);
    
    return 0;
}

/*
*********************************************************************************************************
*                           standby_memcpy
*
*Description: memory copy function for standby.
*
*Arguments  :
*
*Return     :
*
*Notes      :
*
*********************************************************************************************************
*/
static void standby_memcpy(void *dest, void *src, int n)
{
    char    *tmp_src = (char *)src;
    char    *tmp_dst = (char *)dest;

    if(!dest || !src){
        /* parameter is invalid */
        return;
    }

    for( ; n > 0; n--){
        *tmp_dst ++ = *tmp_src ++;
    }

    return;
}
#ifdef DRAM_FREQ_CHECKSUM
void dump(int *addr, int length)
{
    int i = 0;
    int j = 0;
    for (i = 0; i < length; i++){
        if (j==0){
            printk("%8x:  ",addr);
        }
        printk("%8x  ",*addr);
        j++;
        addr++;
        if (j==4){
            j = 0;
            printk("\n");
        }
    }
    printk("\n");
}
void checksum(int *addr, int length, int flag, int *pcrc)
{
    int tmp_length = 0;
    int step = 0;
    int crc = 0;
    int index = 0;
    printk("addr-length(%x-%x)\n",addr,length);
    for(tmp_length = 0; tmp_length < length; tmp_length += 4 ){
        crc += *addr++;
        step++;
        if (step == 256*1024 || tmp_length + 4 == length){
            step = 0;
            if (flag == 0){
                pcrc[index++] = crc;
                //printk("crc check index(%d),%x:%x\n",index-1,(index - 1)*4*256*1024, sum[index - 1]);
            }else{
                if (crc != pcrc[index++]){
                    printk("crc check err(%d) %x:%x-%x\n",index-1,(index - 1)*4*256*1024, crc, pcrc[index - 1]);
                }else{
                    printk("crc check ok(%d),%x:%x\n",index-1,(index - 1)*4*256*1024, pcrc[index - 1]);
                }
            }
            crc = 0;
        }
    }
}
#endif
