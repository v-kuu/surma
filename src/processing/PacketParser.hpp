#pragma once

#include <array>
#include <cstdint>
#include <expected>
#include <variant>

namespace surma::processing
{

using ipv4 = uint32_t;
using ipv6 = std::array<uint8_t, 16>;
using ip_address = std::variant<ipv4, ipv6>;

struct FiveTuple
{
	ip_address src_addr;
	ip_address dst_addr;
	uint16_t src_port;
	uint16_t dst_port;
	uint8_t proto;
};

struct ParsedPacket
{
	struct FiveTuple flow;
	const uint8_t *payload;
	uint16_t payload_len;
	uint16_t eth_proto;

	uint8_t tcp_flags;

	uint8_t ttl;
	uint16_t ip_total_len;
};

enum class ParseError
{
	Truncated,
	Unsupported,
	Malformed,
};

class PacketParser
{
	using parse_result = std::expected<ParsedPacket, ParseError>;

  public:
	static parse_result parse(const uint8_t *pkt, uint32_t len);

  private:
	static parse_result parse_ipv4_(
	    const uint8_t *pkt,
	    uint32_t len,
	    uint32_t offset);
	static parse_result parse_ipv6_(
	    const uint8_t *pkt,
	    uint32_t len,
	    uint32_t offset);
	static parse_result parse_tcp_(
	    const uint8_t *pkt,
	    uint32_t len,
	    uint32_t offset,
	    uint32_t remaining);
	static parse_result parse_udp_(
	    const uint8_t *pkt,
	    uint32_t len,
	    uint32_t offset,
	    uint32_t remaining);
	static parse_result parse_icmp_(
	    const uint8_t *pkt,
	    uint32_t len,
	    uint32_t offset);
};

} // namespace surma::processing
