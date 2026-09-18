#include "FlowEntry.hpp"

namespace surma::flow
{

constexpr uint8_t TH_SYN = 0x02;
constexpr uint8_t TH_ACK = 0x10;
constexpr uint8_t TH_FIN = 0x01;
constexpr uint8_t TH_RST = 0x04;

void FlowEntry::update_tcp_state(uint8_t flags, bool is_initiator)
{
	struct TcpPeer *source = is_initiator ? &src : &dst;
	struct TcpPeer *dest = is_initiator ? &dst : &src;

	if (flags & TH_RST)
	{
		source->state = TcpState::Closed;
		dest->state = TcpState::Closed;
		timeout = timeout::tcp_closed;
		return;
	}

	switch (source->state)
	{
		case TcpState::Closed:
		case TcpState::SynSent:
			if ((flags & (TH_SYN | TH_ACK)) == TH_SYN)
			{
				source->state = TcpState::SynSent;
				timeout = timeout::tcp_syn_sent;
			}
			else if ((flags & (TH_SYN | TH_ACK)) == (TH_SYN | TH_ACK))
			{
				source->state = TcpState::SynRcvd;
				timeout = timeout::tcp_syn_rcvd;
			}
			break;

		case TcpState::SynRcvd:
			if (flags & TH_ACK)
			{
				source->state = TcpState::Established;
				dest->state = TcpState::Established;
				timeout = timeout::tcp_established;
			}
			break;

		case TcpState::Established:
			if (flags & TH_FIN)
			{
				source->state = TcpState::FinWait1;
				timeout = timeout::tcp_fin_wait;
			}
			break;

		case TcpState::FinWait1:
			if (flags & TH_ACK)
				source->state = TcpState::FinWait2;
			if (flags & TH_FIN)
			{
				source->state = TcpState::TimeWait;
				timeout = timeout::tcp_time_wait;
			}
			break;

		case TcpState::FinWait2:
			if (flags & TH_FIN)
			{
				source->state = TcpState::TimeWait;
				timeout = timeout::tcp_time_wait;
			}
			break;

		case TcpState::Closing:
			if (flags & TH_ACK)
			{
				source->state = TcpState::TimeWait;
				timeout = timeout::tcp_time_wait;
			}
			break;

		case TcpState::TimeWait: break;

		case TcpState::CloseWait:
			if (flags & TH_FIN)
			{
				source->state = TcpState::LastAck;
				timeout = timeout::tcp_fin_wait;
			}
			break;

		case TcpState::LastAck:
			if (flags & TH_ACK)
			{
				source->state = TcpState::Closed;
				dest->state = TcpState::Closed;
				timeout = timeout::tcp_closed;
			}
			break;
	}
}

} // namespace surma::flow
