#include "PacketParser.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <linux/if_ether.h>
#include <linux/ipv6.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <netinet/ip.h>

namespace surma::processing
{

using parse_result = std::expected<ParsedPacket, ParseError>;

parse_result PacketParser::parse(const uint8_t *pkt, uint32_t len)
{
	if (len < ETH_HLEN)
		return std::unexpected(ParseError::Truncated);

	const auto *eth = reinterpret_cast<const struct ethhdr *>(pkt);
	uint16_t proto = ntohs(eth->h_proto);
	uint16_t eth_proto = proto;

	// double tagging out of scope
	uint32_t offset = ETH_HLEN;
	if (proto == ETH_P_8021Q)
	{
		if (len < ETH_HLEN + 4)
			return std::unexpected(ParseError::Truncated);

		uint16_t vlan_proto;
		std::memcpy(&vlan_proto, pkt + ETH_HLEN + 2, sizeof(vlan_proto));
		proto = ntohs(vlan_proto);
		offset += 4;
	}

	parse_result ret;
	switch (proto)
	{
		case ETH_P_IP: ret = parse_ipv4_(pkt, len, offset); break;
		case ETH_P_IPV6: ret = parse_ipv6_(pkt, len, offset); break;
		default: return std::unexpected(ParseError::Unsupported);
	}
	if (ret.has_value())
		ret->eth_proto = eth_proto;
	return ret;
}

parse_result PacketParser::parse_ipv4_(
    const uint8_t *pkt,
    uint32_t len,
    uint32_t offset)
{
	if (len < offset + sizeof(struct iphdr))
		return std::unexpected(ParseError::Truncated);

	auto *ip = reinterpret_cast<const struct iphdr *>(pkt + offset);

	// IP header length (ihl) is in bits
	uint32_t ihl = ip->ihl * 4;
	if (ihl < sizeof(struct iphdr))
		return std::unexpected(ParseError::Malformed);

	if (len < offset + ihl)
		return std::unexpected(ParseError::Truncated);

	uint16_t total_len = ntohs(ip->tot_len);
	if (total_len < ihl)
		return std::unexpected(ParseError::Malformed);

	// fragmented packet reassembly out of scope
	if (ntohs(ip->frag_off) & (IP_MF | IP_OFFMASK))
		return std::unexpected(ParseError::Unsupported);

	uint32_t remaining = total_len - ihl;
	offset += ihl;

	parse_result ret;
	switch (ip->protocol)
	{
		case IPPROTO_TCP: ret = parse_tcp_(pkt, len, offset, remaining); break;
		case IPPROTO_UDP: ret = parse_udp_(pkt, len, offset, remaining); break;
		case IPPROTO_ICMP: ret = parse_icmp_(pkt, len, offset); break;
		default: return std::unexpected(ParseError::Unsupported);
	}

	if (ret.has_value())
	{
		ret->flow.src_addr = ip->saddr;
		ret->flow.dst_addr = ip->daddr;
		ret->flow.proto = ip->protocol;
		ret->ttl = ip->ttl;
		ret->ip_total_len = total_len;
	}

	return ret;
}

// TODO
parse_result PacketParser::parse_ipv6_(
    const uint8_t *pkt,
    uint32_t len,
    uint32_t offset)
{
	(void)pkt;
	(void)len;
	(void)offset;
	return std::unexpected(ParseError::Unsupported);
}

parse_result PacketParser::parse_tcp_(
    const uint8_t *pkt,
    uint32_t len,
    uint32_t offset,
    uint32_t remaining)
{
	if (len < offset + sizeof(struct tcphdr))
		return std::unexpected(ParseError::Truncated);
	if (remaining < sizeof(struct tcphdr))
		return std::unexpected(ParseError::Truncated);

	auto *tcp = reinterpret_cast<const struct tcphdr *>(pkt + offset);

	// tcp data offset is in bits
	uint32_t doff = tcp->doff * 4;
	if (doff < sizeof(struct tcphdr))
		return std::unexpected(ParseError::Malformed);

	if (len < offset + doff)
		return std::unexpected(ParseError::Truncated);

	FiveTuple flow{};
	flow.src_port = tcp->source;
	flow.dst_port = tcp->dest;
	parse_result ret{};
	ret->flow = flow;
	ret->tcp_flags = reinterpret_cast<const uint8_t *>(tcp)[13]; // flag trick
	ret->payload = pkt + offset + doff;
	ret->payload_len = static_cast<uint16_t>(remaining - doff);

	return ret;
}

// TODO
parse_result PacketParser::parse_udp_(
    const uint8_t *pkt,
    uint32_t len,
    uint32_t offset,
    uint32_t remaining)
{
	(void)pkt;
	(void)len;
	(void)offset;
	(void)remaining;
	return std::unexpected(ParseError::Unsupported);
}

// TODO
parse_result PacketParser::parse_icmp_(
    const uint8_t *pkt,
    uint32_t len,
    uint32_t offset)
{
	(void)pkt;
	(void)len;
	(void)offset;
	return std::unexpected(ParseError::Unsupported);
}

}; // namespace surma::processing
