#include "FlowEntry.hpp"
#include <netinet/ip.h>

namespace surma::flow
{

constexpr uint8_t TH_SYN = 0x02;
constexpr uint8_t TH_ACK = 0x10;
constexpr uint8_t TH_FIN = 0x01;
constexpr uint8_t TH_RST = 0x04;

void FlowEntry::update_tcp_state(uint8_t flags, bool is_initiator)
{
	TcpPeer *source = is_initiator ? &src : &dst;
	TcpPeer *dest = is_initiator ? &dst : &src;

	if (flags & TH_RST)
	{
		source->state = TcpState::Closed;
		dest->state = TcpState::Closed;
		expiry = timeout::tcp_closed;
		return;
	}

	switch (source->state)
	{
		case TcpState::Closed:
		case TcpState::SynSent:
			if ((flags & (TH_SYN | TH_ACK)) == TH_SYN)
			{
				source->state = TcpState::SynSent;
				expiry = timeout::tcp_syn_sent;
			}
			else if ((flags & (TH_SYN | TH_ACK)) == (TH_SYN | TH_ACK))
			{
				source->state = TcpState::SynRcvd;
				expiry = timeout::tcp_syn_rcvd;
			}
			break;

		case TcpState::SynRcvd:
			if (flags & TH_ACK)
			{
				source->state = TcpState::Established;
				dest->state = TcpState::Established;
				expiry = timeout::tcp_established;
			}
			break;

		case TcpState::Established:
			if (flags & TH_FIN)
			{
				source->state = TcpState::FinWait1;
				expiry = timeout::tcp_fin_wait;
			}
			break;

		case TcpState::FinWait1:
			if ((flags & (TH_FIN | TH_ACK)) == (TH_FIN | TH_ACK))
			{
				source->state = TcpState::Closing;
				expiry = timeout::tcp_fin_wait;
			}
			else if (flags & TH_ACK)
			{
				source->state = TcpState::FinWait2;
			}
			else if (flags & TH_FIN)
			{
				source->state = TcpState::TimeWait;
				expiry = timeout::tcp_time_wait;
			}
			break;

		case TcpState::FinWait2:
			if (flags & TH_FIN)
			{
				source->state = TcpState::TimeWait;
				expiry = timeout::tcp_time_wait;
			}
			break;

		case TcpState::Closing:
			if (flags & TH_ACK)
			{
				source->state = TcpState::TimeWait;
				expiry = timeout::tcp_time_wait;
			}
			break;

		case TcpState::TimeWait: break;

		case TcpState::CloseWait:
			if (flags & TH_FIN)
			{
				source->state = TcpState::LastAck;
				expiry = timeout::tcp_fin_wait;
			}
			break;

		case TcpState::LastAck:
			if (flags & TH_ACK)
			{
				source->state = TcpState::Closed;
				dest->state = TcpState::Closed;
				expiry = timeout::tcp_closed;
			}
			break;
	}
}

std::chrono::seconds FlowEntry::select_timeout() const
{
	switch (key.proto)
	{
		case IPPROTO_TCP:
			switch (src.state)
			{
				case TcpState::Established: return timeout::tcp_established;
				case TcpState::SynSent: return timeout::tcp_syn_sent;
				case TcpState::SynRcvd: return timeout::tcp_syn_rcvd;
				case TcpState::TimeWait: return timeout::tcp_time_wait;
				case TcpState::Closed: return timeout::tcp_closed;
				default: return timeout::tcp_fin_wait;
			}
		case IPPROTO_UDP: return timeout::udp;
		case IPPROTO_ICMP: return timeout::icmp;
		default: return timeout::other;
	}
}

} // namespace surma::flow
