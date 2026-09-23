#pragma once

#include <array>
#include <highwayhash/highwayhash.h>

namespace surma::flow
{

struct FlowKey
{
	uint32_t src_addr;
	uint32_t dst_addr;
	uint16_t src_port;
	uint16_t dst_port;
	uint8_t proto;
	std::array<std::byte, 3> pad_;

	highwayhash::HHResult64 hash(const highwayhash::HHKey &seed) const;
	static FlowKey normalized(const FlowKey &raw, bool &is_initiator);
};

} // namespace surma::flow
