#pragma once
#include <array>
#include <chrono>
#include <cstdint>

namespace surma::flow
{

namespace timeout
{
using namespace std::chrono_literals;

constexpr auto tcp_established = 24h;
constexpr auto tcp_syn_sent = 2min;
constexpr auto tcp_syn_rcvd = 1min;
constexpr auto tcp_fin_wait = 2min;
constexpr auto tcp_time_wait = 90s;
constexpr auto tcp_closed = 90s;
constexpr auto udp = 1min;
constexpr auto icmp = 20s;
constexpr auto other = 1min;
} // namespace timeout

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

struct FlowKey
{
	uint32_t src_addr;
	uint32_t dst_addr;
	uint16_t src_port;
	uint16_t dst_port;
	uint8_t proto;
	std::array<std::byte, 3> pad_;
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
	uint8_t occupied;

	std::array<std::byte, 6> pad_;
};

static_assert(sizeof(FlowEntry) == 96);

enum class FlowAction
{
	Pass,
	Drop,
};

} // namespace surma::flow
