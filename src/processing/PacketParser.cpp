#include "PacketParser.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <linux/if_ether.h>
#include <linux/ipv6.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <netinet/icmp6.h>
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
		eth_proto = proto;
		offset += 4;
	}

	parse_result ret = std::unexpected(ParseError::Unsupported);
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

	// IP header length (ihl) is in 32bit words
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

	parse_result ret = std::unexpected(ParseError::Unsupported);
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

parse_result PacketParser::parse_ipv6_(
    const uint8_t *pkt,
    uint32_t len,
    uint32_t offset)
{
	if (len < offset + sizeof(struct ipv6hdr))
		return std::unexpected(ParseError::Truncated);

	auto *ip6 = reinterpret_cast<const struct ipv6hdr *>(pkt + offset);

	uint8_t next_header = ip6->nexthdr;
	uint32_t ext_offset = offset + sizeof(struct ipv6hdr);
	uint32_t remaining = ntohs(ip6->payload_len);

	parse_result ret = std::unexpected(ParseError::Unsupported);

	bool reading = true;
	while (reading)
	{
		switch (next_header)
		{
			case IPPROTO_TCP:
				ret = parse_tcp_(pkt, len, ext_offset, remaining);
				reading = false;
				break;
			case IPPROTO_UDP:
				ret = parse_udp_(pkt, len, ext_offset, remaining);
				reading = false;
				break;
			case IPPROTO_ICMPV6:
				ret = parse_icmpv6_(pkt, len, ext_offset);
				reading = false;
				break;
			case IPPROTO_HOPOPTS:
			case IPPROTO_ROUTING:
			case IPPROTO_DSTOPTS:
			{
				// extension header
				if (len < ext_offset + 2)
					return std::unexpected(ParseError::Truncated);
				uint32_t ext_len = (pkt[ext_offset + 1] + 1) * 8;
				if (remaining < ext_len)
					return std::unexpected(ParseError::Malformed);
				next_header = pkt[ext_offset];
				ext_offset += ext_len;
				remaining -= ext_len;
				break;
			}
			default: return std::unexpected(ParseError::Unsupported);
		}
	}

	if (ret.has_value())
	{
		std::memcpy(std::get<ipv6>(ret->flow.src_addr).data(), &ip6->saddr, 16);
		std::memcpy(std::get<ipv6>(ret->flow.dst_addr).data(), &ip6->daddr, 16);
		ret->ttl = ip6->hop_limit;
		ret->ip_total_len = ntohs(ip6->payload_len) + sizeof(struct ipv6hdr);
	}
	return ret;
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

	ParsedPacket ret{};
	ret.flow = flow;
	ret.tcp_flags = reinterpret_cast<const uint8_t *>(tcp)[13]; // flag trick
	ret.payload = pkt + offset + doff;
	ret.payload_len = static_cast<uint16_t>(remaining - doff);

	return ret;
}

parse_result PacketParser::parse_udp_(
    const uint8_t *pkt,
    uint32_t len,
    uint32_t offset,
    uint32_t remaining)
{
	if (len < offset + sizeof(struct udphdr))
		return std::unexpected(ParseError::Truncated);
	if (remaining < sizeof(struct udphdr))
		return std::unexpected(ParseError::Truncated);

	auto *udp = reinterpret_cast<const struct udphdr *>(pkt + offset);

	uint16_t udp_len = ntohs(udp->len);
	if (udp_len < sizeof(struct udphdr))
		return std::unexpected(ParseError::Malformed);

	// UDP length field includes the header
	if (udp_len > remaining)
		return std::unexpected(ParseError::Malformed);

	FiveTuple flow{};
	flow.src_port = udp->source;
	flow.dst_port = udp->dest;

	ParsedPacket ret{};
	ret.flow = flow;
	ret.tcp_flags = 0;
	ret.payload = pkt + offset + sizeof(struct udphdr);
	ret.payload_len = udp_len - sizeof(struct udphdr);
	return ret;
}

parse_result PacketParser::parse_icmp_(
    const uint8_t *pkt,
    uint32_t len,
    uint32_t offset)
{
	// ICMPv4 header is 8 bytes
	if (len < offset + 8)
		return std::unexpected(ParseError::Truncated);

	// encode type and code into ports magic trick
	FiveTuple flow{};
	flow.src_port = pkt[offset];
	flow.dst_port = pkt[offset + 1];

	ParsedPacket ret{};
	ret.flow = flow;
	ret.tcp_flags = 0;
	ret.payload = pkt + offset + 8;
	ret.payload_len = len - offset - 8;
	return ret;
}

parse_result PacketParser::parse_icmpv6_(
    const uint8_t *pkt,
    uint32_t len,
    uint32_t offset)
{
	// ICMPv6 header is minimum 8 bytes
	if (len < offset + 8)
		return std::unexpected(ParseError::Truncated);

	uint8_t type = pkt[offset];
	uint8_t code = pkt[offset + 1];

	// encode type and code into ports magic trick
	FiveTuple flow{};
	flow.src_port = type;
	flow.dst_port = code;

	ParsedPacket ret{};
	ret.flow = flow;
	ret.tcp_flags = 0;

	uint32_t header_len = 8;
	switch (type)
	{
		case ND_ROUTER_SOLICIT: header_len = 8; break;
		case ND_ROUTER_ADVERT: header_len = 16; break;
		case ND_NEIGHBOR_SOLICIT:
		case ND_NEIGHBOR_ADVERT: header_len = 24; break;
		case ND_REDIRECT: header_len = 40; break;
		default: header_len = 8; break;
	}

	if (len < offset + header_len)
		return std::unexpected(ParseError::Truncated);

	ret.payload = pkt + offset + header_len;
	ret.payload_len = len - offset - header_len;
	return ret;
}

}; // namespace surma::processing
