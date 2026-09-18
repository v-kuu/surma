#pragma once

#include <array>
#include <highwayhash/highwayhash.h>
#include <highwayhash/instruction_sets.h>

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

	HHResult64 hash(HH_ALIGNAS(32) const HHKey seed)
	{
		HHResult64 result;
		HHStateT<HH_TARGET> state(seed);
		HighwayHashT(
		    &state,
		    reinterpret_cast<const char *>(this),
		    sizeof(*this),
		    &result);
		return result;
	}
};

} // namespace surma::flow
