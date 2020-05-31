const uint16_t portnum = ((std::random_device()()) % 50000) + 1025;

// create a UDP socket and bind it to a local address
UDPSocket sock1{};
sock1.bind(Address("127.0.0.1", portnum));
std::cout<<"example1.1 \n";
// create another UDP socket and send a datagram to the first socket without connecting
UDPSocket sock2{};
sock2.sendto(Address("127.0.0.1", portnum), "hi there");
std::cout<<"example1.2 \n";
// receive sent datagram, connect the socket to the peer's address, and send a response
auto recvd = sock1.recv();
std::cout<<"example1.3 \n";
sock1.connect(recvd.source_address);
std::cout<<"example1.4 \n";
sock1.send("hi yourself");
std::cout<<"example1.5 \n";
auto recvd2 = sock2.recv();

if (recvd.payload != "hi there" || recvd2.payload != "hi yourself") {
    throw std::runtime_error("wrong data received");
}
