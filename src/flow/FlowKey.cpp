#include "FlowKey.hpp"
#include <cstring>
#include <highwayhash/instruction_sets.h>

namespace surma::flow
{

using namespace highwayhash;

HHResult64 FlowKey::hash(const HHKey &seed)
{
	HHResult64 result;
	HHStateT<HH_TARGET> state(seed);
	HighwayHashT(
	    &state, reinterpret_cast<const char *>(this), sizeof(*this), &result);
	return result;
}

void FlowKey::normalize(
    struct FlowKey &key,
    const struct FlowKey &raw,
    bool &is_initiator)
{
	if (raw.src_addr < raw.dst_addr ||
	    (raw.src_addr == raw.dst_addr && raw.src_port <= raw.dst_port))
	{
		key = raw;
		is_initiator = true;
	}
	else
	{
		key.src_addr = raw.dst_addr;
		key.dst_addr = raw.src_addr;
		key.src_port = raw.dst_port;
		key.dst_port = raw.src_port;
		key.proto = raw.proto;
		memset(key.pad_.data(), 0, sizeof(key.pad_));
		is_initiator = false;
	}
}

} // namespace surma::flow
