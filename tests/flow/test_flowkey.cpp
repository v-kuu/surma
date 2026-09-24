#include "flow/FlowKey.hpp"
#include <arpa/inet.h>
#include <catch2/catch_test_macros.hpp>

using namespace surma::flow;

namespace
{

// RFC 5737 documentation range, never routable
constexpr uint32_t ADDR_A = 0xc0000201; // 192.0.2.1
constexpr uint32_t ADDR_B = 0xc0000202; // 192.0.2.2
constexpr uint32_t ADDR_SAME = 0xc0000201;

constexpr uint16_t PORT_HTTP = 80;
constexpr uint16_t PORT_DNS = 53;
constexpr uint16_t PORT_HIGH = 1234;
constexpr uint16_t PORT_ALT = 8080;

constexpr highwayhash::HHKey SEED_A = { 1, 2, 3, 4 };
constexpr highwayhash::HHKey SEED_B = { 5, 6, 7, 8 };

constexpr std::array<std::byte, 3> PAD_DIRTY = { std::byte{ 0xff },
	                                             std::byte{ 0xff },
	                                             std::byte{ 0xff } };
constexpr std::array<std::byte, 3> PAD_ZERO = {};

static FlowKey make_key(
    uint32_t src,
    uint32_t dst,
    uint16_t sport,
    uint16_t dport,
    uint8_t proto)
{
	FlowKey k{};
	k.src_addr = htonl(src);
	k.dst_addr = htonl(dst);
	k.src_port = htons(sport);
	k.dst_port = htons(dport);
	k.proto = proto;
	return k;
}

} // namespace

TEST_CASE(
    "normalization: src < dst address stays as initiator",
    "[unit][flowkey]")
{
	bool is_initiator;
	auto raw = make_key(ADDR_A, ADDR_B, PORT_HIGH, PORT_HTTP, IPPROTO_TCP);
	auto key = FlowKey::normalized(raw, is_initiator);

	REQUIRE(is_initiator);
	REQUIRE(key.src_addr == raw.src_addr);
	REQUIRE(key.dst_addr == raw.dst_addr);
	REQUIRE(key.src_port == raw.src_port);
	REQUIRE(key.dst_port == raw.dst_port);
}

TEST_CASE("normalization: src > dst address swaps direction", "[unit][flowkey]")
{
	bool is_initiator;
	auto raw = make_key(ADDR_B, ADDR_A, PORT_HTTP, PORT_HIGH, IPPROTO_TCP);
	auto key = FlowKey::normalized(raw, is_initiator);

	REQUIRE_FALSE(is_initiator);
	REQUIRE(key.src_addr == htonl(ADDR_A));
	REQUIRE(key.dst_addr == htonl(ADDR_B));
	REQUIRE(key.src_port == htons(PORT_HIGH));
	REQUIRE(key.dst_port == htons(PORT_HTTP));
}

TEST_CASE(
    "normalization: both directions produce identical key",
    "[unit][flowkey]")
{
	bool fwd_initiator;
	bool rev_initiator;

	auto fwd = make_key(ADDR_A, ADDR_B, PORT_HIGH, PORT_HTTP, IPPROTO_TCP);
	auto rev = make_key(ADDR_B, ADDR_A, PORT_HTTP, PORT_HIGH, IPPROTO_TCP);

	auto fwd_key = FlowKey::normalized(fwd, fwd_initiator);
	auto rev_key = FlowKey::normalized(rev, rev_initiator);

	REQUIRE(fwd_initiator);
	REQUIRE_FALSE(rev_initiator);
	REQUIRE(memcmp(&fwd_key, &rev_key, sizeof(FlowKey)) == 0);
}

TEST_CASE(
    "normalization: equal addresses src port <= dst port stays",
    "[unit][flowkey]")
{
	bool is_initiator;
	auto raw = make_key(ADDR_SAME, ADDR_SAME, PORT_HTTP, PORT_ALT, IPPROTO_TCP);
	auto key = FlowKey::normalized(raw, is_initiator);

	REQUIRE(is_initiator);
	REQUIRE(key.src_port == htons(PORT_HTTP));
	REQUIRE(key.dst_port == htons(PORT_ALT));
}

TEST_CASE(
    "normalization: equal addresses src port > dst port swaps",
    "[unit][flowkey]")
{
	bool is_initiator;
	auto raw = make_key(ADDR_SAME, ADDR_SAME, PORT_ALT, PORT_HTTP, IPPROTO_TCP);
	auto key = FlowKey::normalized(raw, is_initiator);

	REQUIRE_FALSE(is_initiator);
	REQUIRE(key.src_port == htons(PORT_HTTP));
	REQUIRE(key.dst_port == htons(PORT_ALT));
}

TEST_CASE("normalization: proto is preserved", "[unit][flowkey]")
{
	bool is_initiator;
	auto raw = make_key(ADDR_A, ADDR_B, PORT_HIGH, PORT_DNS, IPPROTO_UDP);
	auto key = FlowKey::normalized(raw, is_initiator);

	REQUIRE(key.proto == IPPROTO_UDP);
}

TEST_CASE(
    "normalization: padding zeroed in forward direction",
    "[unit][flowkey]")
{
	FlowKey dirty = make_key(ADDR_A, ADDR_B, PORT_HIGH, PORT_HTTP, IPPROTO_TCP);
	dirty.pad_ = PAD_DIRTY;

	bool is_initiator;
	auto key = FlowKey::normalized(dirty, is_initiator);

	REQUIRE(key.pad_ == PAD_ZERO);
}

TEST_CASE(
    "normalization: padding zeroed in swapped direction",
    "[unit][flowkey]")
{
	FlowKey dirty = make_key(ADDR_B, ADDR_A, PORT_HTTP, PORT_HIGH, IPPROTO_TCP);
	dirty.pad_ = PAD_DIRTY;

	bool is_initiator;
	auto key = FlowKey::normalized(dirty, is_initiator);

	REQUIRE_FALSE(is_initiator);
	REQUIRE(key.pad_ == PAD_ZERO);
}

TEST_CASE("hash: same key produces same result", "[unit][flowkey]")
{
	bool is_initiator;
	auto key = FlowKey::normalized(
	    make_key(ADDR_A, ADDR_B, PORT_HIGH, PORT_HTTP, IPPROTO_TCP),
	    is_initiator);

	REQUIRE(key.hash(SEED_A) == key.hash(SEED_A));
}

TEST_CASE("hash: forward and reverse produce same hash", "[unit][flowkey]")
{
	bool fwd_init, rev_init;
	auto fwd_key = FlowKey::normalized(
	    make_key(ADDR_A, ADDR_B, PORT_HIGH, PORT_HTTP, IPPROTO_TCP), fwd_init);
	auto rev_key = FlowKey::normalized(
	    make_key(ADDR_B, ADDR_A, PORT_HTTP, PORT_HIGH, IPPROTO_TCP), rev_init);

	REQUIRE(fwd_key.hash(SEED_A) == rev_key.hash(SEED_A));
}

TEST_CASE("hash: different proto produces different hash", "[unit][flowkey]")
{
	bool i1, i2;
	auto k1 = FlowKey::normalized(
	    make_key(ADDR_A, ADDR_B, PORT_HIGH, PORT_HTTP, IPPROTO_TCP), i1);
	auto k2 = FlowKey::normalized(
	    make_key(ADDR_A, ADDR_B, PORT_HIGH, PORT_HTTP, IPPROTO_UDP), i2);

	REQUIRE(k1.hash(SEED_A) != k2.hash(SEED_A));
}

TEST_CASE("hash: different seeds produce different hashes", "[unit][flowkey]")
{
	bool is_initiator;
	auto key = FlowKey::normalized(
	    make_key(ADDR_A, ADDR_B, PORT_HIGH, PORT_HTTP, IPPROTO_TCP),
	    is_initiator);

	REQUIRE(key.hash(SEED_A) != key.hash(SEED_B));
}

TEST_CASE(
    "hash: dirty padding does not affect hash after normalization",
    "[unit][flowkey]")
{
	FlowKey dirty = make_key(ADDR_A, ADDR_B, PORT_HIGH, PORT_HTTP, IPPROTO_TCP);
	dirty.pad_ = PAD_DIRTY;

	FlowKey clean = make_key(ADDR_A, ADDR_B, PORT_HIGH, PORT_HTTP, IPPROTO_TCP);
	clean.pad_ = PAD_ZERO;

	bool i1, i2;
	auto dirty_norm = FlowKey::normalized(dirty, i1);
	auto clean_norm = FlowKey::normalized(clean, i2);

	REQUIRE(dirty_norm.hash(SEED_A) == clean_norm.hash(SEED_A));
}
