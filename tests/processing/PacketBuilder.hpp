#pragma once
#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <linux/if_ether.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <vector>

namespace surma::test
{

class PacketBuilder
{
  public:
	static PacketBuilder ethernet(uint16_t proto = ETH_P_IP)
	{
		PacketBuilder b;
		struct ethhdr eth{};
		memset(eth.h_dest, 0xaa, ETH_ALEN);
		memset(eth.h_source, 0xbb, ETH_ALEN);
		eth.h_proto = htons(proto);
		b.append(&eth, sizeof(eth));
		return b;
	}

	PacketBuilder &vlan(uint16_t vid, uint16_t inner_proto)
	{
		uint16_t vlan_tpid = htons(ETH_P_8021Q);
		memcpy(data_.data() + 12, &vlan_tpid, 2);
		uint16_t tci = htons(vid & 0xfff);
		uint16_t inner = htons(inner_proto);
		append(&tci, 2);
		append(&inner, 2);
		return *this;
	}

	PacketBuilder &ipv4(
	    uint32_t src = 0x0a000001,
	    uint32_t dst = 0x0a000002,
	    uint8_t proto = IPPROTO_TCP,
	    uint8_t ttl = 64)
	{
		struct iphdr ip{};
		ip.ihl = 5;
		ip.version = 4;
		ip.ttl = ttl;
		ip.protocol = proto;
		ip.saddr = htonl(src);
		ip.daddr = htonl(dst);
		ip_offset_ = data_.size();
		append(&ip, sizeof(ip));
		return *this;
	}

	PacketBuilder &tcp(
	    uint16_t sport = 12345,
	    uint16_t dport = 80,
	    uint8_t flags = 0x02 /* SYN */)
	{
		struct tcphdr tcp{};
		tcp.source = htons(sport);
		tcp.dest = htons(dport);
		tcp.doff = 5;
		reinterpret_cast<uint8_t *>(&tcp)[13] = flags;
		append(&tcp, sizeof(tcp));
		return *this;
	}

	PacketBuilder &udp(uint16_t sport = 12345, uint16_t dport = 53)
	{
		struct udphdr udp{};
		udp.source = htons(sport);
		udp.dest = htons(dport);
		udp.len = htons(sizeof(udp));
		udp_offset_ = data_.size();
		append(&udp, sizeof(udp));
		return *this;
	}

	PacketBuilder &payload(const uint8_t *data, size_t len)
	{
		append(data, len);
		return *this;
	}

	PacketBuilder &truncate(size_t to)
	{
		if (to < data_.size())
			data_.resize(to);
		return *this;
	}

	std::vector<uint8_t> build()
	{
		if (ip_offset_ > 0)
		{
			auto *ip =
			    reinterpret_cast<struct iphdr *>(data_.data() + ip_offset_);
			ip->tot_len = htons(data_.size() - ip_offset_);
		}
		if (udp_offset_ > 0)
		{
			auto *udp =
			    reinterpret_cast<struct udphdr *>(data_.data() + udp_offset_);
			udp->len = htons(data_.size() - udp_offset_);
		}
		return data_;
	}

	const uint8_t *data() const { return data_.data(); }
	size_t size() const { return data_.size(); }

  private:
	void append(const void *src, size_t n)
	{
		const auto *p = static_cast<const uint8_t *>(src);
		data_.insert(data_.end(), p, p + n);
	}

	std::vector<uint8_t> data_;
	size_t ip_offset_ = 0;
	size_t udp_offset_ = 0;
};

} // namespace surma::test
