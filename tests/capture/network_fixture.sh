#!/usr/bin/env bash
set -euo pipefail

NS="surma-test"
HOST_IF="surma-host"
TEST_IF="veth-test"

cleanup()
{
    ip netns del "$NS" 2>/dev/null || true
    ip link del "$HOST_IF" 2>/dev/null || true
}

case "${1:-}" in
    setup)
        cleanup

        # Create namespace
        ip netns add "$NS"

        # Create veth pair
        ip link add "$HOST_IF" type veth peer name "$TEST_IF"

        # Move one end into namespace
        ip link set "$TEST_IF" netns "$NS"

        # Configure host side
        ip addr add 10.99.0.1/24 dev "$HOST_IF"
        ip link set "$HOST_IF" up

        # Configure namespace side
        ip netns exec "$NS" ip addr add 10.99.0.2/24 dev "$TEST_IF"
        ip netns exec "$NS" ip link set lo up
        ip netns exec "$NS" ip link set "$TEST_IF" up

        echo "Created $NS with interface $TEST_IF"
        ;;

    teardown)
        cleanup
        echo "Removed $NS"
        ;;

	inject)
		VETH_MAC=$(ip netns exec "$NS" cat /sys/class/net/$TEST_IF/address)
		python3 -c "
		from scapy.all import *
		sendp([Ether(dst='$VETH_MAC')/IP(src='10.99.0.1',dst='10.99.0.2')/
			UDP(sport=9999,dport=9999)/Raw(b'surma integration test')
			for _ in range(10)], iface='$HOST_IF', verbose=False)
		"
		echo "Injected 10 packets to $HOST_IF"
		;;

    *)
        echo "usage: $0 {setup|teardown|inject}"
        exit 1
        ;;
esac
