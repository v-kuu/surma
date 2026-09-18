#pragma once

#include "FlowKey.hpp"
#include <array>
#include <chrono>
#include <cstdint>

namespace surma::flow
{

enum class TcpState
{
	Closed,
	SynSent,
	SynRcvd,
	Established,
	FinWait1,
	FinWait2,
	Closing,
	TimeWait,
	CloseWait,
	LastAck,
};

struct TcpPeer
{
	uint32_t seqno;
	uint32_t max_win;
	uint8_t state;
	uint8_t wscale;
	std::array<std::byte, 2> pad_;
};

using std::chrono::steady_clock;
struct FlowEntry
{
	struct FlowKey key;
	uint64_t hash;

	struct TcpPeer src;
	struct TcpPeer dst;

	steady_clock::time_point created_at;
	steady_clock::time_point last_seen;
	std::chrono::seconds timeout;

	uint64_t packets;
	uint64_t bytes;

	uint8_t action;
	bool occupied;

	std::array<std::byte, 6> pad_;
};
static_assert(sizeof(FlowEntry) == 96);

} // namespace surma::flow
