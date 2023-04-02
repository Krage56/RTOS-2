#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <string.h>
#include <cmath>
#include <iostream>
#include "bbc.h"

static resmgr_connect_funcs_t    connect_funcs;
static resmgr_io_funcs_t         io_funcs;
static iofunc_attr_t             attr;

bbs::BBSParams global_params;
//x_n := x_{n-1}^2 mod M
uint32_t last_x;

uint32_t BBS()
{
	int N = sizeof(uint32_t) * 8;
	uint32_t result = 0;
	uint32_t M = global_params.p * global_params.q;
	for (int i = 0; i < N; ++i)
	{
		result = result | (last_x % 2);
		result = result << (i == N - 1? 0: 1);
		std::cout << "result: " << result << ", prev_x: " << last_x;
		last_x = (last_x * last_x) % M;
		std::cout << "cur_x: " << last_x << std::endl;
	}
	return result;
}

int io_devctl(resmgr_context_t *ctp, io_devctl_t *msg,
              RESMGR_OCB_T *ocb)
{
    int     status, nbytes;

    union
    {
    	bbs::BBSParams  params;
        uint32_t     data32;
    } *rx_data;

    /*
     * Check if there is system message
    */
    if ((status = iofunc_devctl_default(ctp, msg, ocb)) !=
         _RESMGR_DEFAULT) {
        return(status);
    }
    status = nbytes = 0;
    bbs::BBSParams* tmp_params;
    switch (msg->i.dcmd)
    {
    case SET_PARAMS:
    	tmp_params = reinterpret_cast<bbs::BBSParams*>(_DEVCTL_DATA(msg->i));
    	//validation
    	if (!(tmp_params->p % 4 == 3 && tmp_params->q % 4 == 3))
    	{
    		fprintf(stderr,
    		        "%d %d: Invalid numbers.\n",
    		        tmp_params->p, tmp_params->q);
    	}
    	else
    	{
    		global_params = *tmp_params;
    		last_x = global_params.seed;
    	}
    	nbytes = 0;
        break;

    case GET_DATA:
    	*(reinterpret_cast<uint32_t*>(_DEVCTL_DATA(msg->i))) = BBS();
    	nbytes = sizeof(uint32_t);
    	break;

    default:
        return(ENOSYS);
    }

    memset(&msg->o, 0, sizeof(msg->o));
    /* Indicate the number of bytes and return the message */
    msg->o.nbytes = nbytes;
    return(_RESMGR_PTR(ctp, &msg->o, sizeof(msg->o) + nbytes));
}




int main(int argc, char **argv)
{
    /* declare variables we'll be using */
    resmgr_attr_t        resmgr_attr;
    dispatch_t           *dpp;
    dispatch_context_t   *ctp;
    int                  id;

    /* initialize dispatch interface */
    if((dpp = dispatch_create()) == NULL) {
        fprintf(stderr,
                "%s: Unable to allocate dispatch handle.\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    /* initialize resource manager attributes */
    memset(&resmgr_attr, 0, sizeof(resmgr_attr));
    resmgr_attr.nparts_max = 1;
    resmgr_attr.msg_max_size = 2048;

    /* initialize functions for handling messages */
    iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs,
                     _RESMGR_IO_NFUNCS, &io_funcs);

    /* initialize attribute structure used by the device */
    iofunc_attr_init(&attr, S_IFNAM | 0666, 0, 0);

    /* For handling _IO_DEVCTL, sent by devctl() */
    io_funcs.devctl = io_devctl;

    /* attach our device name */
    id = resmgr_attach(
            dpp,            /* dispatch handle        */
            &resmgr_attr,   /* resource manager attrs */
            "/dev/cryptobbs",  /* device name            */
            _FTYPE_ANY,     /* open type              */
            0,              /* flags                  */
            &connect_funcs, /* connect routines       */
            &io_funcs,      /* I/O routines           */
            &attr);         /* handle                 */
    if(id == -1) {
        fprintf(stderr, "%s: Unable to attach name.\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* allocate a context structure */
    ctp = dispatch_context_alloc(dpp);

    /* start the resource manager message loop */
    while(1) {
        if((ctp = dispatch_block(ctp)) == NULL) {
            fprintf(stderr, "block error\n");
            return EXIT_FAILURE;
        }
        dispatch_handler(ctp);
    }
    return EXIT_SUCCESS; // never go here
}
