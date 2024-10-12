import socket
import pytest

UDP_IP = "255.255.255.255"
UDP_PORT = 1884
MESSAGE = "search"

@pytest.mark.timeout(5)
def test_udp_server():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.sendto(bytes(MESSAGE, "utf-8"), (UDP_IP, UDP_PORT))
    recv_data, addr = sock.recvfrom(1024)
    assert recv_data.decode("utf-8") == "found"

