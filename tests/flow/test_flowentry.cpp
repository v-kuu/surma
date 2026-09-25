#include "flow/FlowEntry.hpp"
#include <arpa/inet.h>
#include <catch2/catch_test_macros.hpp>
#include <netinet/ip.h>

using namespace surma::flow;

namespace
{

constexpr uint8_t TH_SYN = 0x02;
constexpr uint8_t TH_ACK = 0x10;
constexpr uint8_t TH_FIN = 0x01;
constexpr uint8_t TH_RST = 0x04;

constexpr uint32_t ADDR_A = 0xc0000201;
constexpr uint32_t ADDR_B = 0xc0000202;
constexpr uint16_t PORT_CLIENT = 1234;
constexpr uint16_t PORT_SERVER = 80;

static FlowEntry make_tcp_entry()
{
	FlowEntry e{};
	e.key.src_addr = htonl(ADDR_A);
	e.key.dst_addr = htonl(ADDR_B);
	e.key.src_port = htons(PORT_CLIENT);
	e.key.dst_port = htons(PORT_SERVER);
	e.key.proto = IPPROTO_TCP;
	e.src.state = TcpState::Closed;
	e.dst.state = TcpState::Closed;
	e.state = SlotState::Occupied;
	e.action = FlowAction::Pass;
	e.created_at = std::chrono::steady_clock::now();
	e.last_seen = e.created_at;
	e.expiry = timeout::tcp_syn_sent;
	return e;
}

} // namespace

TEST_CASE("rst closes both peers from established", "[unit][flowentry][tcp]")
{
	auto e = make_tcp_entry();
	e.src.state = TcpState::Established;
	e.dst.state = TcpState::Established;

	e.update_tcp_state(TH_RST, true);

	REQUIRE(e.src.state == TcpState::Closed);
	REQUIRE(e.dst.state == TcpState::Closed);
	REQUIRE(e.expiry == timeout::tcp_closed);
}

TEST_CASE("rst closes both peers from syn sent", "[unit][flowentry][tcp]")
{
	auto e = make_tcp_entry();
	e.src.state = TcpState::SynSent;

	e.update_tcp_state(TH_RST, true);

	REQUIRE(e.src.state == TcpState::Closed);
	REQUIRE(e.dst.state == TcpState::Closed);
	REQUIRE(e.expiry == timeout::tcp_closed);
}

TEST_CASE("rst from responder closes both peers", "[unit][flowentry][tcp]")
{
	auto e = make_tcp_entry();
	e.src.state = TcpState::Established;
	e.dst.state = TcpState::Established;

	e.update_tcp_state(TH_RST, false); // responder sends RST

	REQUIRE(e.src.state == TcpState::Closed);
	REQUIRE(e.dst.state == TcpState::Closed);
}

TEST_CASE("syn moves initiator to syn sent", "[unit][flowentry][tcp]")
{
	auto e = make_tcp_entry();

	e.update_tcp_state(TH_SYN, true);

	REQUIRE(e.src.state == TcpState::SynSent);
	REQUIRE(e.expiry == timeout::tcp_syn_sent);
}

TEST_CASE("syn ack moves responder to syn rcvd", "[unit][flowentry][tcp]")
{
	auto e = make_tcp_entry();
	e.src.state = TcpState::SynSent;

	e.update_tcp_state(TH_SYN | TH_ACK, false); // responder sends SYN-ACK

	REQUIRE(e.dst.state == TcpState::SynRcvd);
	REQUIRE(e.expiry == timeout::tcp_syn_rcvd);
}

TEST_CASE("ack in syn rcvd moves both to established", "[unit][flowentry][tcp]")
{
	auto e = make_tcp_entry();
	e.src.state = TcpState::SynSent;
	e.dst.state = TcpState::SynRcvd;

	e.update_tcp_state(TH_ACK, false);

	REQUIRE(e.src.state == TcpState::Established);
	REQUIRE(e.dst.state == TcpState::Established);
	REQUIRE(e.expiry == timeout::tcp_established);
}

TEST_CASE("fin in established moves to fin wait 1", "[unit][flowentry][tcp]")
{
	auto e = make_tcp_entry();
	e.src.state = TcpState::Established;
	e.dst.state = TcpState::Established;

	e.update_tcp_state(TH_FIN, true);

	REQUIRE(e.src.state == TcpState::FinWait1);
	REQUIRE(e.expiry == timeout::tcp_fin_wait);
}

TEST_CASE("ack in fin wait 1 moves to fin wait 2", "[unit][flowentry][tcp]")
{
	auto e = make_tcp_entry();
	e.src.state = TcpState::FinWait1;

	e.update_tcp_state(TH_ACK, true);

	REQUIRE(e.src.state == TcpState::FinWait2);
}

TEST_CASE("fin in fin wait 2 moves to time wait", "[unit][flowentry][tcp]")
{
	auto e = make_tcp_entry();
	e.src.state = TcpState::FinWait2;

	e.update_tcp_state(TH_FIN, true);

	REQUIRE(e.src.state == TcpState::TimeWait);
	REQUIRE(e.expiry == timeout::tcp_time_wait);
}

TEST_CASE("time wait state ignores all flags", "[unit][flowentry][tcp]")
{
	auto e = make_tcp_entry();
	e.src.state = TcpState::TimeWait;
	auto expiry_before = e.expiry;

	e.update_tcp_state(TH_FIN | TH_ACK, true);

	REQUIRE(e.src.state == TcpState::TimeWait);
	REQUIRE(e.expiry == expiry_before);
}

TEST_CASE("fin ack in fin wait 1 moves to closing", "[unit][flowentry][tcp]")
{
	auto e = make_tcp_entry();
	e.src.state = TcpState::FinWait1;

	e.update_tcp_state(TH_FIN | TH_ACK, true);

	REQUIRE(e.src.state == TcpState::Closing);
	REQUIRE(e.expiry == timeout::tcp_fin_wait);
}

TEST_CASE("ack in closing moves to time wait", "[unit][flowentry][tcp]")
{
	auto e = make_tcp_entry();
	e.src.state = TcpState::Closing;

	e.update_tcp_state(TH_ACK, true);

	REQUIRE(e.src.state == TcpState::TimeWait);
	REQUIRE(e.expiry == timeout::tcp_time_wait);
}

TEST_CASE("fin in close wait moves to last ack", "[unit][flowentry][tcp]")
{
	auto e = make_tcp_entry();
	e.src.state = TcpState::CloseWait;

	e.update_tcp_state(TH_FIN, true);

	REQUIRE(e.src.state == TcpState::LastAck);
	REQUIRE(e.expiry == timeout::tcp_fin_wait);
}

TEST_CASE("ack in last ack closes both peers", "[unit][flowentry][tcp]")
{
	auto e = make_tcp_entry();
	e.src.state = TcpState::LastAck;
	e.dst.state = TcpState::FinWait2;

	e.update_tcp_state(TH_ACK, true);

	REQUIRE(e.src.state == TcpState::Closed);
	REQUIRE(e.dst.state == TcpState::Closed);
	REQUIRE(e.expiry == timeout::tcp_closed);
}

TEST_CASE(
    "state updates apply to correct peer based on direction",
    "[unit][flowentry][tcp]")
{
	auto e = make_tcp_entry();
	e.src.state = TcpState::Established;
	e.dst.state = TcpState::Established;

	// responder sends FIN. dst should transition, src should stay
	e.update_tcp_state(TH_FIN, false);

	REQUIRE(e.dst.state == TcpState::FinWait1);
	REQUIRE(e.src.state == TcpState::Established);
}

TEST_CASE(
    "select_timeout returns established timeout for established tcp",
    "[unit][flowentry][timeout]")
{
	auto e = make_tcp_entry();
	e.src.state = TcpState::Established;

	REQUIRE(e.select_timeout() == timeout::tcp_established);
}

TEST_CASE(
    "select_timeout returns syn sent timeout",
    "[unit][flowentry][timeout]")
{
	auto e = make_tcp_entry();
	e.src.state = TcpState::SynSent;

	REQUIRE(e.select_timeout() == timeout::tcp_syn_sent);
}

TEST_CASE(
    "select_timeout returns fin wait for unrecognized tcp state",
    "[unit][flowentry][timeout]")
{
	auto e = make_tcp_entry();
	e.src.state = TcpState::FinWait1;

	REQUIRE(e.select_timeout() == timeout::tcp_fin_wait);
}

TEST_CASE(
    "select_timeout returns udp timeout for udp entry",
    "[unit][flowentry][timeout]")
{
	auto e = make_tcp_entry();
	e.key.proto = IPPROTO_UDP;

	REQUIRE(e.select_timeout() == timeout::udp);
}

TEST_CASE(
    "select_timeout returns icmp timeout for icmp entry",
    "[unit][flowentry][timeout]")
{
	auto e = make_tcp_entry();
	e.key.proto = IPPROTO_ICMP;

	REQUIRE(e.select_timeout() == timeout::icmp);
}

TEST_CASE(
    "select_timeout returns other timeout for unknown proto",
    "[unit][flowentry][timeout]")
{
	auto e = make_tcp_entry();
	e.key.proto = 0x99;

	REQUIRE(e.select_timeout() == timeout::other);
}

TEST_CASE("tcp peer default state is closed", "[unit][flowentry][layout]")
{
	TcpPeer peer{};
	REQUIRE(peer.state == TcpState::Closed);
	REQUIRE(peer.seqno == 0);
	REQUIRE(peer.max_win == 0);
	REQUIRE(peer.wscale == 0);
}
