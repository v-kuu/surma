#include "FlowKey.hpp"
#include <cstring>
#include <highwayhash/highwayhash_target.h>
#include <highwayhash/instruction_sets.h>

namespace surma::flow
{

using namespace highwayhash;

HHResult64 FlowKey::hash(const HHKey &seed) const
{
	HHResult64 result;
	InstructionSets::Run<HighwayHash>(
	    seed, reinterpret_cast<const char *>(this), sizeof(*this), &result);
	return result;
}

FlowKey FlowKey::normalized(const FlowKey &raw, bool &is_initiator)
{
	FlowKey key{};
	if (raw.src_addr < raw.dst_addr ||
	    (raw.src_addr == raw.dst_addr && raw.src_port <= raw.dst_port))
	{
		key = raw;
		key.pad_ = {};
		is_initiator = true;
	}
	else
	{
		key.src_addr = raw.dst_addr;
		key.dst_addr = raw.src_addr;
		key.src_port = raw.dst_port;
		key.dst_port = raw.src_port;
		key.proto = raw.proto;
		is_initiator = false;
	}
	return key;
}

} // namespace surma::flow
