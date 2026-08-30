#pragma once
#include <cstdint>

namespace surma::capture
{

struct PacketDescriptor
{
	uint64_t addr;
	uint32_t len;
};

} // namespace surma::capture
