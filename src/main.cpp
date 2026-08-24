#include "surma.hpp"
#include <cerrno>
#include <spdlog/spdlog.h>

int main()
{
	try
	{
		surma::run();
		return 0;
	}
	catch (std::exception &e)
	{
		spdlog::error(
		    "Uncaught exeption \"{}\", errno={}, {}",
		    e.what(),
		    errno,
		    std::strerror(errno));
		return 1;
	}
}
