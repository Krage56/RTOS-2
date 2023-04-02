#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <devctl.h>
#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <vector>
#include <signal.h>
#include "bbs.h"

const int vec_len = 1024;

bool stop_signal = false;

struct OutData
{
	OutData():
		out_data(std::vector<uint32_t>(vec_len)),
		current_pos(0)
	{}
	std::vector<uint32_t> out_data;
	int current_pos;
	int push(const uint32_t& num)
	{
		out_data[current_pos] = num;
		current_pos = (current_pos + 1) % vec_len;
		return current_pos;
	}
};

void signalHandler(int signum)
{
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    stop_signal = true;
}


int main( int argc, char **argv )
{
	//set handler for CTRL+C
	signal(SIGINT, signalHandler);
    // open a connection to the server (fd == coid)
    int fd = open("/dev/cryptobbs", O_RDWR);
    if(fd < 0)
    {
        std::cerr << "E: unable to open server connection: " << strerror(errno ) << std::endl;
        return EXIT_FAILURE;
    }

    bbs::BBSParams ser_params;
    ser_params.p = 3;
    ser_params.q = 263;
    ser_params.seed = 866;

    int comma_result = devctl(fd, SET_PARAMS, &ser_params, sizeof(bbs::BBSParams), nullptr);

    if(comma_result != EOK)
    {
            std::cerr << "E: unable to set generator parameters: " << strerror(errno) << std::endl;
            return EXIT_FAILURE;
    }

    uint32_t num;
    OutData data;
    do
    {
    	comma_result = devctl(fd, GET_DATA, &num, sizeof(num), NULL);
    	//there is needn't sleep because qnx blocks client till server's reply will be ready
    	data.push(num);
    } while (comma_result == EOK && !stop_signal);

    std::cout << "Output vector" << std::endl;
    for (auto &_el: data.out_data)
    	std::cout << _el << std::endl;

    close(fd);

    return EXIT_SUCCESS;
}
