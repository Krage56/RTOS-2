#pragma once
#include <cstdint>
//It is client-RS protocol
namespace bbs {

struct BBSParams
{
    std::uint32_t seed;
    std::uint32_t p;
    std::uint32_t q;
};

//Set parameters on administrator
#define SET_PARAMS __DIOT(_DCMD_MISC, 1, bbs::BBSParams)
//Receive data from administrator
#define GET_DATA __DIOF(_DCMD_MISC, 2, std::uint32_t)
}
