#pragma once

namespace gxy
{
class ClientServer 
{
public: 	
	ClientServer();

	void setup_client(char *server_host, int port = 1900);
	void setup_server(int port = 1900);

	int get_socket() { return skt; }

private:
	int skt;
};
}
