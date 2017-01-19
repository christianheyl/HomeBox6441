/*****************************************************************************
 * start multithreading on MIPS34Kc
 * Copyright (c) 2010, Lantiq Inc., All rights reserved
 *****************************************************************************/
#include <linux/autoconf.h>
#include <linux/kernel.h>       //for pr_debug()

#include "startMT.h"

//unsigned long loadELF(char * fileName);

// Global variables
u32 numTC=0,numVPE=0;

void initMT(void)
{
    u32 i;
    unsigned long val, dmt_flag;

    pr_debug("\n initMT start");

    // Check if we are the Master VPE
    val = read_c0_vpeconf0();
    if (!(val & VPECONF0_MVP))
    {
        pr_debug("\n initMT failed! Non VPE0?");
        return;
    }

    // Find the total number of the VPEs and MTs of the current MIPS chip
    val = read_c0_mvpconf0();
    numTC  = val & MVPCONF0_PTC;
    numVPE = (val & MVPCONF0_PVPE) >> MVPCONF0_PVPE_SHIFT;
    pr_debug("\n TC max index=%d", numTC);
    pr_debug("\n VPE max index=%d", numVPE);

    //showMT();

    // Disable multi threading
    dmt_flag = dmt();

    // Disable multi VPE
    dvpe();

    // Put MVPE into 'configuration state'
    set_c0_mvpcontrol(MVPCONTROL_VPC);

//    // Enable the control of the other VPE. After reset this is set to '1' anyway for the Master VPE
//    // Useless because the test above already passed
//    //val = read_c0_vpeconf0();
//    //val = val | (VPECONF0_MVP);
//    //write_vpe_c0_vpeconf0(val);
//    write_vpe_c0_vpeconf0(read_c0_vpeconf0() | (VPECONF0_MVP));

    // Do fix association of the TCs:
    //TC0 assigned to VPE0 is left untouched. TC1,2,3 are set unallocated and halted.
    for (i=1; i<=numTC; i++)
    {
        // Select the TC
        settc(i);

        // Mark the TC not activated, not dynamically allocatable and interrupt exempt
        //val = read_tc_c0_tcstatus();
        //val &= ~TCSTATUS_A;
        //val &= ~TCSTATUS_DA;
        //val |= TCSTATUS_IXMT;
        //write_tc_c0_tcstatus(val);
        write_tc_c0_tcstatus(((read_tc_c0_tcstatus() & ~(TCSTATUS_A)) & ~(TCSTATUS_DA)) | TCSTATUS_IXMT);

        // Mark the TC halted
        write_tc_c0_tchalt(TCHALT_H);

        // Set some invalid restart address
        write_tc_c0_tcrestart((unsigned long)0);

        // Set some invalid context
        write_tc_c0_tccontext((unsigned long)0);

        // Bind it to VPE0
        write_tc_c0_tcbind(read_tc_c0_tcbind() & ~(TCBIND_CURVPE));
    }

    // Take system out of configuration state
    clear_c0_mvpcontrol(MVPCONTROL_VPC);

    // Let the multi-threading disabled
    // Let the multi VPE disabled

    //showMT();

    pr_debug("\n initMT finish");
}

void runMT(unsigned long stackAddr, unsigned long heapAddr, unsigned long globalAddr, unsigned long startAddr)
{
    unsigned long val, dmt_flag;
    u32 tcIndex;

    pr_debug("\n runMT start");

    //showMT();

    // Check if we are the Master VPE
    val = read_c0_vpeconf0();
    if (!(val & VPECONF0_MVP))
    {
        pr_debug("\n runMT failed! Non VPE0?");
        return;
    }

    // Disable multi VPE
    dvpe();

    // Put MVPE into 'configuration state'
    set_c0_mvpcontrol(MVPCONTROL_VPC);

    // Select the TC
    tcIndex = 2;
    settc(tcIndex);

    // Should check if this TC is halted and not activated
    if ((read_tc_c0_tcstatus() & TCSTATUS_A) || !(read_tc_c0_tchalt() & TCHALT_H))
    {
        pr_debug("\n runMT failed! TC%d is already doing something?", tcIndex);
        showMT();

        // Take system out of configuration state
        clear_c0_mvpcontrol(MVPCONTROL_VPC);

        // Re-enable the multi VPE operation mode
        evpe(EVPE_ENABLE);

        return;
    }

    //Disable multi-threaded execution whilst we activate, clear the halt bit and bound the tc to the other VPE...
    dmt_flag = dmt();

    // Write the address we want it to start running from in the TCPC register.
#ifdef PARSE_ELF_AND_LOAD
    write_tc_c0_tcrestart((unsigned long)startAddr);
#else
    write_tc_c0_tcrestart((unsigned long)&mainVPE1);
#endif

    pr_debug("\n milestone 1");
    //TCContext is purely a software read/write register, usable by the operating system as a pointer to thread-specific
    //storage, e.g. a thread context save area.
    write_tc_c0_tccontext((unsigned long)0);

    // Mark the TC as activated, not interrupt exempt and not dynamically allocatable
    //val = read_tc_c0_tcstatus();
    //val = (val & ~(TCSTATUS_DA | TCSTATUS_IXMT)) | TCSTATUS_A;
    //write_tc_c0_tcstatus(val);
    write_tc_c0_tcstatus((read_tc_c0_tcstatus() & ~(TCSTATUS_DA | TCSTATUS_IXMT)) | TCSTATUS_A);

    // Take it out of HALT. It doesn't run yet, and it will wait until the VPE1 will be allow to run.
    write_tc_c0_tchalt(read_tc_c0_tchalt() & ~(TCHALT_H));

    pr_debug("\n milestone 2");
    /*
     * The sde-kit passes 'memsize' to __start in $a3, so set something
     * here...  Or set $a3 to zero and define DFLT_STACK_SIZE and
     * DFLT_HEAP_SIZE when you compile your program
     */
    mttgpr(7, heapAddr);     //start of the heap space
    mttgpr(29, stackAddr);   //set stack pointer SP
    mttgpr(28, globalAddr);  //set global pointer GP

    pr_debug("\n milestone 3");
    // Set up VPE1
    // Bind the TC to VPE 1 as late as possible so we only have the final VPE registers to set up, and so an EJTAG probe can trigger on it.
    //val = read_tc_c0_tcbind();
    //val &= ~TCBIND_CURVPE;
    //val |= v->index;
    //write_tc_c0_tcbind(val);
    write_tc_c0_tcbind((read_tc_c0_tcbind() & ~TCBIND_CURVPE) | 1);     //'1' means VPE1
    //back_to_back_c0_hazard();

    // Be sure that this Virtual Processor is deactivated
    //val = read_vpe_c0_vpeconf0();
    //val &= ~(VPECONF0_VPA);
    //write_vpe_c0_vpeconf0(val);
    write_vpe_c0_vpeconf0(read_vpe_c0_vpeconf0() & ~(VPECONF0_VPA));
    back_to_back_c0_hazard();

    pr_debug("\n milestone 4");
    // Set up the XTC bit in vpeconf0 to point at our tc
    //val = read_vpe_c0_vpeconf0();
    //val &= ~(VPECONF0_XTC);
    //val |= (tcIndex << VPECONF0_XTC_SHIFT);
    //write_vpe_c0_vpeconf0(val);
    write_vpe_c0_vpeconf0((read_vpe_c0_vpeconf0() & ~(VPECONF0_XTC)) | (tcIndex << VPECONF0_XTC_SHIFT));
    back_to_back_c0_hazard();

    // Enable this VPE
    //val = read_vpe_c0_vpeconf0();
    //val |= VPECONF0_VPA;
    //write_vpe_c0_vpeconf0(val);
    write_vpe_c0_vpeconf0(read_vpe_c0_vpeconf0() | VPECONF0_VPA);

    // Clear out any left overs from a previous program
    write_vpe_c0_status(0);
    write_vpe_c0_cause(0);

    pr_debug("\n milestone 5");
    // Take the system out of configuration state
    clear_c0_mvpcontrol(MVPCONTROL_VPC);

    // Now is safe to re-enable the multi-threading
    emt(dmt_flag);

    pr_debug("\n milestone 6");
    showMT();

    // Set it running
    evpe(EVPE_ENABLE);

    pr_debug("\n runMT finish");
}

void stopMT(void)
{
    u32 i;
    unsigned long val, dmt_flag;

    pr_debug("\n stopMT start");

    // Check if we are the Master VPE
    val = read_c0_vpeconf0();
    if (!(val & VPECONF0_MVP))
    {
        pr_debug("\n stopMT failed! Non VPE0?");
        return;
    }

    // Disable multi threading
    dmt_flag = dmt();

    // Disable multi VPE
    dvpe();

    // Put MVPE into 'configuration state'
    set_c0_mvpcontrol(MVPCONTROL_VPC);

    // Do fix association of the TCs:
    //TC0 assigned to VPE0 is left untouched. TC1,2,3 are set unallocated and halted.
    for (i=1; i<=numTC; i++)
    {
        // Select the TC
        settc(i);

        // Mark the TC not activated, not dynamically allocatable and interrupt exempt
        //val = read_tc_c0_tcstatus();
        //val &= ~TCSTATUS_A;
        //val &= ~TCSTATUS_DA;
        //val |= TCSTATUS_IXMT;
        //write_tc_c0_tcstatus(val);
        write_tc_c0_tcstatus(((read_tc_c0_tcstatus() & ~(TCSTATUS_A)) & ~(TCSTATUS_DA)) | TCSTATUS_IXMT);

        // Mark the TC halted
        write_tc_c0_tchalt(TCHALT_H);

        // Set some invalid restart address
        write_tc_c0_tcrestart((unsigned long)0);

        // Set some invalid context
        write_tc_c0_tccontext((unsigned long)0);

        // Bind it to VPE0
        write_tc_c0_tcbind(read_tc_c0_tcbind() & ~(TCBIND_CURVPE));
    }

    // Take system out of configuration state
    clear_c0_mvpcontrol(MVPCONTROL_VPC);

    // Let the multi-threading disabled
    // Let the multi VPE disabled

    pr_debug("\n stopMT finish");

}


void mainVPE1 (void)
// no stack or external APIs used: OK
{

    while(1)
    {
    }
}
//#1 with stack and apis
//#2 with no APIs
//{
//    int i,j;
//    u32 mainVPE1Cntr;
//
//    // Note: since 2 threads will use the 'print' function , the output may come out interleaved :-)
//    //Add little delay
//    for (i=0; i< 0xFFFF; i++)
//        j++;
//
//    //showGPR(2);
//
//    // At this point we can either stay in an infinite loop (doing just a printf to indicate that VPE1 is alive)
//    //or start the ThreadX on this VPE.
//#if 1
//    //Reset mainVPE1Cntr
//    #define CNTR_MASK 0xfffff
//    mainVPE1Cntr = 0;
//
//    while(1)
//    {
//        mainVPE1Cntr++;
//        if ((mainVPE1Cntr & CNTR_MASK) == 0)
//        {
//            pr_debug("+");
//        }
//    }
//#else
//    unsigned long addr;
//    addr = STATIC_PROG1_PTR;
//    pr_debug("\n One way jump to 0x%8lx\n", addr);
//
//    void (*start)(void) = (void *)addr;
//    start();
//#endif
//}

void showMT(void)
{
    u32 tcIndex;

    pr_debug("\n");
    for (tcIndex=0; tcIndex<=numTC; tcIndex++)
    {
        settc(tcIndex);
        pr_debug("\n\nTC=%d", tcIndex);

        pr_debug("\nMVPControl=0x%x", read_c0_mvpcontrol());
        pr_debug("\nMVPConf0=0x%x",   read_c0_mvpconf0());

        pr_debug("\nVPEControl=0x%lx", read_vpe_c0_vpecontrol());
        pr_debug("\nVPEConf0=0x%lx",   read_vpe_c0_vpeconf0());

        pr_debug("\nTCStatus=0x%lx",   read_tc_c0_tcstatus());
        if (((read_tc_c0_tcstatus() >> TCSTATUS_A_SHIFT) & 1) == 1)
            pr_debug(" --> Activated");
        else
            pr_debug(" --> NOT Activated");
        if (((read_tc_c0_tcstatus() >> TCSTATUS_DA_SHIFT) & 1) == 1)
            pr_debug(" --> Dynamicaly Allocatable");
        else
            pr_debug(" --> NOT Dynamicaly Allocatable");
        if (((read_tc_c0_tcstatus() >> TCSTATUS_IXMT_SHIFT) & 1) == 1)
            pr_debug(" --> Interrupt Exempt");
        else
            pr_debug(" --> NOT Interrupt Exempt");

        pr_debug("\nTCBind=0x%lx",     read_tc_c0_tcbind());
        pr_debug(" --> to VPE=0x%lx", (read_tc_c0_tcbind()&0xF));
        pr_debug("\nTCRestart=0x%lx",  read_tc_c0_tcrestart());
        pr_debug("\nTCHalt=0x%lx",     read_tc_c0_tchalt());
        if ((read_tc_c0_tchalt()&1)==1)
            pr_debug(" --> halted");
        else
            pr_debug(" --> NOT halted");
        pr_debug("\nTCContext=0x%lx",  read_tc_c0_tccontext());

        showGPR(tcIndex);
    }
    settc(0);
}

void showGPR(u32 tcIndex)
{
    settc(tcIndex);

    // Display the GPR used by this TC
    pr_debug("\n");
    pr_debug("\n 0     = zero   =");  pr_debug(" 0x%lx", mftgpr(0));
    pr_debug("\n 1     = at     =");  pr_debug(" 0x%lx", mftgpr(1));
    pr_debug("\n 2-3   = v0-v1  =");  pr_debug(" 0x%lx", mftgpr(2)); pr_debug(" 0x%lx", mftgpr(3));
    pr_debug("\n 4-7   = a0-a3  =");  pr_debug(" 0x%lx", mftgpr(4)); pr_debug(" 0x%lx", mftgpr(5)); pr_debug(" 0x%lx", mftgpr(6)); pr_debug(" 0x%lx", mftgpr(7));

    pr_debug("\n 26-27 = k0-k1  =");  pr_debug(" 0x%lx", mftgpr(26)); pr_debug(" 0x%lx", mftgpr(27));
    pr_debug("\n 28    = gp     =");  pr_debug(" 0x%lx", mftgpr(28));
    pr_debug("\n 29    = sp     =");  pr_debug(" 0x%lx", mftgpr(29));
    pr_debug("\n 30    = pf/s8  =");  pr_debug(" 0x%lx", mftgpr(30));
    pr_debug("\n 31    = ra     =");  pr_debug(" 0x%lx", mftgpr(31));

}
