#include "FlowTable.hpp"
#include <sys/random.h>

namespace surma::flow
{

std::expected<FlowTable, FlowError> FlowTable::init(
    uint32_t capacity,
    uint32_t limit)
{
	if (capacity & (capacity - 1))
		return std::unexpected(FlowError::CapacityNotP2);

	FlowTable ret(capacity, limit);

	// allocate flowentry array

	ret.capacity_ = capacity;
	ret.mask_ = capacity - 1;
	ret.limit_ = limit ? limit : (capacity * FLOW_TABLE_MAX_LOAD / 100);

	getrandom(seed_, sizeof(highwayhash::HHKey), 0); // randomizes every boot

	return ret;
}

} // namespace surma::flow
