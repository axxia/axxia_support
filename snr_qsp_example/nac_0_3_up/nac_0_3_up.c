/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2018 Intel Corporation. All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/* Simple test program to put NAC0-3 ports in up state. Also disables MAC learning
   main() calls function setup_hlp contains a call to ies_initialize to instantiate the
   IES API
      The program stays active until Ctrl-C is pressed
   On exit, ies_terminate is called to shut down IES API gracefully

   If program is ran in background  (./jltest &), it will print text when switch events occur


   To build, set up your enviroment for DPDK/RTE build, then type 'make all'

*/ 


#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>


#include <ies_sdk.h>



int quiet = 0;
uint32_t port_up_event_count = 0;


#define debug_print(format, args...)\
    if (!quiet) {\
        printf(format, ## args);\
    }

static void print_switch_state(enum ies_switch_state ies_sw_state)
{
    switch (ies_sw_state)
    {
    case IES_SWITCH_STATE_DOWN:
        debug_print("Switch state is IES_SWITCH_STATE_DOWN\n");
        break;
    case IES_SWITCH_STATE_HOLD:
        debug_print("Switch state is IES_SWITCH_STATE_HOLD\n");
        break;
    case IES_SWITCH_STATE_UP:
        debug_print("Switch state is IES_SWITCH_STATE_UP\n");
        break;
    default:
        debug_print("Invalid Switch state\n");
        break;
    }
}


static void ies_event_handler_func(const struct ies_event *ies_event)
{
    const char *ies_event_name;

    if (ies_event != NULL)
    {
        ies_event_name = ies_event_to_str(ies_event->type);
        debug_print("IES_EVENT : %s\n", ies_event_name);
        /**
        * Now handle the event itself...
        * Place holder for now with just print statements
        */
        switch (ies_event->type)
        {
        case IES_EVENT_ADDR_UPDATE:
            debug_print("IES_EVENT : MAC Address Update\n");
            break;
        case IES_EVENT_ERROR:
            debug_print("IES_EVENT : Error\n");
            break;
        case IES_EVENT_PORT_STATE:
            debug_print("IES_EVENT : Port State\n");
            if( ies_event->info.port_state.status == IES_PORT_STATUS_LINK_UP )
	    {
		port_up_event_count += 1;
		debug_print("Link up\n");
            }
	    else if(ies_event->info.port_state.status == IES_PORT_STATUS_LINK_DOWN )
	    {
		debug_print("Link down\n");
	    }
	    else
	    {
		debug_print("Invalid link status\n");
	    }
            break;
        case IES_EVENT_SWITCH_STATE:
            debug_print("IES_EVENT : Switch State\n");
            break;
        case IES_EVENT_TIMEOUT:
            debug_print("IES_EVENT : Timeout\n");
            break;
        default:
            break;
        }
    }
}


int setup_hlp(void)
{
    int sw = 0;
    int port, ret;
    enum ies_switch_state ies_sw_state;
    enum ies_port_mode port_mode;
    enum ies_port_phy_type phy_type;
    uint32_t event_mask;
 
    event_mask = IES_EVENT_SWITCH_STATE |
                 IES_EVENT_PORT_STATE |
                 IES_EVENT_ADDR_UPDATE |
                 IES_EVENT_ERROR |
                 IES_EVENT_TIMEOUT;

    /*Initialize IES*/ 
    debug_print("Initializing IES \n");
    ret = ies_initialize(ies_event_handler_func, event_mask);
    if(ret != IES_OK)
    {
       debug_print("Error in ies_initialize...errno = %d\n", ret);
       return ret;
    }

    /*Get switch state*/
    debug_print("Fetching Switch State\n");
    do
    {
        usleep(100);
        ret = ies_switch_get_state(sw, &ies_sw_state);
        if(ret != IES_OK)
        {
            debug_print("Error in ies_initialize\n");
            return ret;
        }
    }while(ies_sw_state != IES_SWITCH_STATE_UP);

    print_switch_state(ies_sw_state);

    /*Get port information*/
    /* Only enabling ports 0 - 3 in this example*/


   /* Reset port_up_event_count */
   port_up_event_count = 0;

   
    for(port = 0; port < 4; port++)
    {
        ret = ies_port_get_mode(sw, port, &port_mode);
        if(ret != IES_OK)
        {
            debug_print("Error in ies_port_get_mode() for port = %d\n", port);
            return ret;
        }

	/* Take port DOWN */
        debug_print("Setting port %d to DOWN\n",port);
        port_mode = IES_PORT_MODE_ADMIN_DOWN;
        ret = ies_port_set_mode(sw, port, port_mode);
        if(ret != IES_OK)
        {
            debug_print("ies_port_set_mode() failed to take port DOWN. errno = %d\n", ret);
            return ret;
        }	

        /*Set the phy type for each port*/
	/* For this test, set ports to 25GBASE_CR  */

	
        phy_type = IES_PORT_PHY_TYPE_25GBASE_CR;
           

        ret = ies_port_set_attr(sw, port, IES_PA_PHY_TYPE, &phy_type);
        if(ret != IES_OK)
        {
            debug_print("ies_port_set_attr() failed. errno = %d\n", ret);
            return ret;
        }

	/* Take port UP*/
	debug_print("Setting port %d to UP\n",port);
        port_mode = IES_PORT_MODE_ADMIN_UP;

        ret = ies_port_set_mode(sw, port, port_mode);
        if(ret != IES_OK)
        {
            debug_print("ies_port_set_mode() failed. errno = %d\n", ret);
            return ret;
        }
    }
    
    /*Wait for port up events for all ports*/
    debug_print("Waiting for port UP events\n");
    uint32_t timeout = 120;
    while((port_up_event_count < 4) && (timeout != 0))
    {
        timeout -= 1;
        sleep(1);
    }

    if((port_up_event_count < 4) && (timeout == 0))
    {
        debug_print("Timed out waiting for port UP events. Waited 2 minutes\n");
        return -1;
    }

    debug_print("HLP ports are UP\n");
    return 0;
}



unsigned char quitFlag=0;

void quit_fn(int sig)
{
   quitFlag = 1;
}

int main(void)
{
  int ret; 
  int macLearn = 0;
  unsigned int i;


  /* Register SIGINT  (Ctrl-C), used to terminate program */
   signal(SIGINT, quit_fn);
 


   /* function to setup hlp (includes a call to ies_initialize)*/
   setup_hlp();


    /* Turn off MAC learning for nac-eth0 -4 */
   for (i=0; i<4; i++)
   {    
	    ret = ies_port_set_attr(0,i,IES_PA_LEARNING,&macLearn);
	    if (ret != IES_OK)
	    {
		printf("Error disabling MAC learning on port %d\n",i);
	    }
   }

#if 0   
   /* Dump port settings for nac-eth0 - 3  */
    for (i=0; i<4; i++)
    {
	    ret = ies_port_dbg_dump_attributes(0,i);
	    if (ret != IES_OK)
	    {
		printf("Error dumping attributes on port %d\n",i);
	    }
    }
#endif 


   while(quitFlag == 0)
   {}


    printf("Exiting nac_0_3_up. Terminating connection to ies api\n");

    /*Terminate ies apis*/
    ret = ies_terminate(false);
    if (ret != IES_OK)
    {
	printf("Error ies_terminate\n");
    }

}

