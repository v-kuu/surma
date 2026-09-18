#include "FlowKey.hpp"
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

} // namespace surma::flow
