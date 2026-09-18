#pragma once

#include <array>
#include <highwayhash/highwayhash.h>

namespace surma::flow
{

using namespace highwayhash;

struct FlowKey
{
	uint32_t src_addr;
	uint32_t dst_addr;
	uint16_t src_port;
	uint16_t dst_port;
	uint8_t proto;
	std::array<std::byte, 3> pad_;

	// TODO: HH_ALIGNAS(32) the seed at caller site
	HHResult64 hash(const HHKey &seed);
};

} // namespace surma::flow
