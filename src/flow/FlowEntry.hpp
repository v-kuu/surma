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

struct TcpPeer
{
	uint32_t seqno;
	uint32_t max_win;
	TcpState state;
	uint8_t wscale;
	std::array<std::byte, 2> pad_;
};

using std::chrono::steady_clock;

struct FlowEntry
{
	struct FlowKey key;
	highwayhash::HHResult64 hash;

	struct TcpPeer src;
	struct TcpPeer dst;

	steady_clock::time_point created_at;
	steady_clock::time_point last_seen;
	std::chrono::seconds timeout;

	uint64_t packets;
	uint64_t bytes;

	FlowAction action;
	bool occupied;

	std::array<std::byte, 6> pad_;

	void update_tcp_state(uint8_t flags, bool is_initiator);
	std::chrono::seconds select_timeout() const;
};
static_assert(sizeof(FlowEntry) == 96);

} // namespace surma::flow
