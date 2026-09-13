#include "PacketBuilder.hpp"
#include "processing/PacketParser.hpp"
#include <catch2/catch_test_macros.hpp>
#include <linux/if_ether.h>
#include <netinet/ip.h>

using namespace surma::processing;
using namespace surma::test;

TEST_CASE("truncated below ethernet minimum", "[unit][parser][ethernet]")
{
	auto pkt = PacketBuilder::ethernet().build();
	auto result = PacketParser::parse(pkt.data(), ETH_HLEN - 1);
	REQUIRE_FALSE(result.has_value());
	REQUIRE(result.error() == ParseError::Truncated);
}

TEST_CASE(
    "unsupported ethertype returns unsupported",
    "[unit][parser][ethernet]")
{
	auto pkt = PacketBuilder::ethernet(0x9999).build();
	auto result = PacketParser::parse(pkt.data(), pkt.size());
	REQUIRE_FALSE(result.has_value());
	REQUIRE(result.error() == ParseError::Unsupported);
}

TEST_CASE(
    "VLAN tagged packet parsed correctly",
    "[unit][parser][ethernet][vlan]")
{
	auto pkt = PacketBuilder::ethernet()
	               .vlan(100, ETH_P_IP)
	               .ipv4(0x0a000001, 0x0a000002, IPPROTO_UDP)
	               .udp()
	               .build();
	auto result = PacketParser::parse(pkt.data(), pkt.size());
	REQUIRE(result.has_value());
	REQUIRE(result->eth_proto == ETH_P_IP);
}

TEST_CASE("VLAN tag truncated", "[unit][parser][ethernet][vlan]")
{
	auto pkt = PacketBuilder::ethernet().vlan(100, ETH_P_IP).build();
	;
	auto result = PacketParser::parse(pkt.data(), ETH_HLEN + 2);
	REQUIRE_FALSE(result.has_value());
	REQUIRE(result.error() == ParseError::Truncated);
}

TEST_CASE("truncated IPv4 header", "[unit][parser][ipv4]")
{
	auto pkt = PacketBuilder::ethernet().ipv4().tcp().build();
	auto result = PacketParser::parse(pkt.data(), ETH_HLEN + sizeof(iphdr) - 1);
	REQUIRE_FALSE(result.has_value());
	REQUIRE(result.error() == ParseError::Truncated);
}

TEST_CASE("IPv4 IHL less than minimum rejected", "[unit][parser][ipv4]")
{
	auto pkt = PacketBuilder::ethernet().ipv4().tcp().build();
	pkt[ETH_HLEN] = (pkt[ETH_HLEN] & 0xf0) | 4;
	auto result = PacketParser::parse(pkt.data(), pkt.size());
	REQUIRE_FALSE(result.has_value());
	REQUIRE(result.error() == ParseError::Malformed);
}

TEST_CASE("fragmented IPv4 packet rejected", "[unit][parser][ipv4]")
{
	auto pkt = PacketBuilder::ethernet().ipv4().tcp().build();
	uint16_t frag_off = htons(IP_MF);
	memcpy(pkt.data() + ETH_HLEN + 6, &frag_off, 2);
	auto result = PacketParser::parse(pkt.data(), pkt.size());
	REQUIRE_FALSE(result.has_value());
	REQUIRE(result.error() == ParseError::Unsupported);
}

TEST_CASE("IPv4 tot_len less than IHL rejected", "[unit][parser][ipv4]")
{
	auto pkt = PacketBuilder::ethernet().ipv4().tcp().build();
	uint16_t bad_len = htons(10);
	memcpy(pkt.data() + ETH_HLEN + 2, &bad_len, 2);
	auto result = PacketParser::parse(pkt.data(), pkt.size());
	REQUIRE_FALSE(result.has_value());
	REQUIRE(result.error() == ParseError::Malformed);
}

TEST_CASE(
    "unsupported IPv4 protocol returns unsupported",
    "[unit][parser][ipv4]")
{
	auto pkt =
	    PacketBuilder::ethernet().ipv4(0x0a000001, 0x0a000002, 0x99).build();
	auto result = PacketParser::parse(pkt.data(), pkt.size());
	REQUIRE_FALSE(result.has_value());
	REQUIRE(result.error() == ParseError::Unsupported);
}

TEST_CASE("IPv4 addresses parsed correctly", "[unit][parser][ipv4]")
{
	auto pkt = PacketBuilder::ethernet()
	               .ipv4(0x0a000001, 0x0a000002, IPPROTO_UDP)
	               .udp()
	               .build();
	auto result = PacketParser::parse(pkt.data(), pkt.size());
	REQUIRE(result.has_value());
	REQUIRE(std::get<uint32_t>(result->flow.src_addr) == htonl(0x0a000001));
	REQUIRE(std::get<uint32_t>(result->flow.dst_addr) == htonl(0x0a000002));
}

TEST_CASE("IPv4 TTL parsed correctly", "[unit][parser][ipv4]")
{
	auto pkt = PacketBuilder::ethernet()
	               .ipv4(0x0a000001, 0x0a000002, IPPROTO_UDP, 128)
	               .udp()
	               .build();
	auto result = PacketParser::parse(pkt.data(), pkt.size());
	REQUIRE(result.has_value());
	REQUIRE(result->ttl == 128);
}

TEST_CASE("truncated TCP header", "[unit][parser][tcp]")
{
	auto pkt = PacketBuilder::ethernet().ipv4().tcp().build();
	auto result = PacketParser::parse(
	    pkt.data(), ETH_HLEN + sizeof(iphdr) + sizeof(tcphdr) - 1);
	REQUIRE_FALSE(result.has_value());
	REQUIRE(result.error() == ParseError::Truncated);
}

TEST_CASE("TCP doff less than minimum rejected", "[unit][parser][tcp]")
{
	auto pkt = PacketBuilder::ethernet().ipv4().tcp().build();
	size_t doff_offset = ETH_HLEN + sizeof(iphdr) + 12;
	pkt[doff_offset] = (pkt[doff_offset] & 0x0f) | (4 << 4);
	auto result = PacketParser::parse(pkt.data(), pkt.size());
	REQUIRE_FALSE(result.has_value());
	REQUIRE(result.error() == ParseError::Malformed);
}

TEST_CASE("TCP ports and flags parsed correctly", "[unit][parser][tcp]")
{
	auto pkt =
	    PacketBuilder::ethernet().ipv4().tcp(9999, 80, 0x02 /* SYN */).build();
	auto result = PacketParser::parse(pkt.data(), pkt.size());
	REQUIRE(result.has_value());
	REQUIRE(ntohs(result->flow.src_port) == 9999);
	REQUIRE(ntohs(result->flow.dst_port) == 80);
	REQUIRE(result->tcp_flags == 0x02);
}

TEST_CASE("truncated UDP header", "[unit][parser][udp]")
{
	auto pkt = PacketBuilder::ethernet()
	               .ipv4(0x0a000001, 0x0a000002, IPPROTO_UDP)
	               .udp()
	               .build();
	auto result = PacketParser::parse(
	    pkt.data(), ETH_HLEN + sizeof(iphdr) + sizeof(udphdr) - 1);
	REQUIRE_FALSE(result.has_value());
	REQUIRE(result.error() == ParseError::Truncated);
}

TEST_CASE("UDP length less than header size rejected", "[unit][parser][udp]")
{
	auto pkt = PacketBuilder::ethernet()
	               .ipv4(0x0a000001, 0x0a000002, IPPROTO_UDP)
	               .udp()
	               .build();
	uint16_t bad_len = htons(4);
	memcpy(pkt.data() + ETH_HLEN + sizeof(iphdr) + 4, &bad_len, 2);
	auto result = PacketParser::parse(pkt.data(), pkt.size());
	REQUIRE_FALSE(result.has_value());
	REQUIRE(result.error() == ParseError::Malformed);
}

TEST_CASE(
    "UDP length greater than IP remaining rejected",
    "[unit][parser][udp]")
{
	auto pkt = PacketBuilder::ethernet()
	               .ipv4(0x0a000001, 0x0a000002, IPPROTO_UDP)
	               .udp()
	               .build();
	uint16_t bad_len = htons(9999);
	memcpy(pkt.data() + ETH_HLEN + sizeof(iphdr) + 4, &bad_len, 2);
	auto result = PacketParser::parse(pkt.data(), pkt.size());
	REQUIRE_FALSE(result.has_value());
	REQUIRE(result.error() == ParseError::Malformed);
}

TEST_CASE("UDP ports parsed correctly", "[unit][parser][udp]")
{
	auto pkt = PacketBuilder::ethernet()
	               .ipv4(0x0a000001, 0x0a000002, IPPROTO_UDP)
	               .udp(1234, 5678)
	               .build();
	auto result = PacketParser::parse(pkt.data(), pkt.size());
	REQUIRE(result.has_value());
	REQUIRE(ntohs(result->flow.src_port) == 1234);
	REQUIRE(ntohs(result->flow.dst_port) == 5678);
	REQUIRE(result->tcp_flags == 0);
}

TEST_CASE("truncated ICMP header", "[unit][parser][icmp]")
{
	auto pkt = PacketBuilder::ethernet()
	               .ipv4(0x0a000001, 0x0a000002, IPPROTO_ICMP)
	               .build();
	std::vector<uint8_t> icmp(7, 0);
	pkt.insert(pkt.end(), icmp.begin(), icmp.end());
	uint16_t tot = htons(pkt.size() - ETH_HLEN);
	memcpy(pkt.data() + ETH_HLEN + 2, &tot, 2);
	auto result = PacketParser::parse(pkt.data(), pkt.size());
	REQUIRE_FALSE(result.has_value());
	REQUIRE(result.error() == ParseError::Truncated);
}

TEST_CASE("ICMP type and code encoded as ports", "[unit][parser][icmp]")
{
	auto pkt = PacketBuilder::ethernet()
	               .ipv4(0x0a000001, 0x0a000002, IPPROTO_ICMP)
	               .build();
	std::vector<uint8_t> icmp = {
		8, 0,      // type=8 (echo), code=0
		0, 0,      // checksum
		0, 1, 0, 1 // id=1, seq=1
	};
	pkt.insert(pkt.end(), icmp.begin(), icmp.end());
	uint16_t tot = htons(pkt.size() - ETH_HLEN);
	memcpy(pkt.data() + ETH_HLEN + 2, &tot, 2);
	auto result = PacketParser::parse(pkt.data(), pkt.size());
	REQUIRE(result.has_value());
	REQUIRE(result->flow.src_port == 8); // type
	REQUIRE(result->flow.dst_port == 0); // code
}

TEST_CASE("proto field set correctly for TCP", "[unit][parser][proto]")
{
	auto pkt = PacketBuilder::ethernet().ipv4().tcp().build();
	auto result = PacketParser::parse(pkt.data(), pkt.size());
	REQUIRE(result.has_value());
	REQUIRE(result->flow.proto == IPPROTO_TCP);
}

TEST_CASE("proto field set correctly for UDP", "[unit][parser][proto]")
{
	auto pkt = PacketBuilder::ethernet()
	               .ipv4(0x0a000001, 0x0a000002, IPPROTO_UDP)
	               .udp()
	               .build();
	auto result = PacketParser::parse(pkt.data(), pkt.size());
	REQUIRE(result.has_value());
	REQUIRE(result->flow.proto == IPPROTO_UDP);
}
