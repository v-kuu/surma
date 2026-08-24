#pragma once
#include "capture/Platform.hpp"

#include <memory>

namespace surma
{

struct Config
{
	std::unique_ptr<capture::Platform> platform;
	std::string iface;
	uint32_t queue_id;
};

} // namespace surma
