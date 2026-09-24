#pragma once

#include "FlowKey.hpp"
#include <array>
#include <chrono>
#include <cstdint>
#include <highwayhash/highwayhash.h>

namespace surma::flow
{

namespace timeout
{
using namespace std::chrono_literals;

constexpr auto udp = 1min;
constexpr auto icmp = 20s;
constexpr auto other = 1min;
constexpr auto tcp_established = 24h;
constexpr auto tcp_syn_sent = 2min;
constexpr auto tcp_syn_rcvd = 1min;
constexpr auto tcp_fin_wait = 2min;
constexpr auto tcp_time_wait = 90s;
constexpr auto tcp_closed = 90s;
} // namespace timeout

enum class TcpState : uint8_t
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

enum class FlowAction : uint8_t
{
	Pass,
	Drop,
};

enum class SlotState : uint8_t
{
	Empty,
	Occupied,
};

struct TcpPeer
{
	uint32_t seqno = 0;
	uint32_t max_win = 0;
	TcpState state = TcpState::Closed;
	uint8_t wscale = 0;
	std::array<std::byte, 2> pad_ = {};
};

using std::chrono::steady_clock;

struct FlowEntry
{
	FlowKey key;
	highwayhash::HHResult64 hash;

	TcpPeer src;
	TcpPeer dst;

	steady_clock::time_point created_at;
	steady_clock::time_point last_seen;
	std::chrono::seconds expiry;

	uint64_t packets;
	uint64_t bytes;

	FlowAction action;
	SlotState state;

	std::array<std::byte, 6> pad_;

	void update_tcp_state(uint8_t flags, bool is_initiator);
	std::chrono::seconds select_timeout() const;
};

// 64 bytes would be ideal but 96 still keeps two entries per cache line
static_assert(sizeof(FlowEntry) == 96);

} // namespace surma::flow
