"""Helpers for keeping MusE's Pyro bridge on loopback or a private IPv4 network."""

import ipaddress


DEFAULT_BIND_HOST = "127.0.0.1"
_PRIVATE_NETWORKS = tuple(
    ipaddress.ip_network(network)
    for network in ("10.0.0.0/8", "172.16.0.0/12", "192.168.0.0/16")
)


def normalize_bind_host(host):
    """Return a literal loopback/private IPv4 address or reject unsafe hosts."""
    value = (host or "").strip()
    if not value or value.lower() == "localhost":
        return DEFAULT_BIND_HOST

    try:
        address = ipaddress.ip_address(value)
    except ValueError as exc:
        raise ValueError("Pyro host must be a loopback or private IPv4 address") from exc

    if not isinstance(address, ipaddress.IPv4Address):
        raise ValueError("Pyro host must be a loopback or private IPv4 address")
    if address.is_loopback or any(address in network for network in _PRIVATE_NETWORKS):
        return str(address)
    raise ValueError("Pyro host must be a loopback or private IPv4 address")
