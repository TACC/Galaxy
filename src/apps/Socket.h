class Socket 
{
public: 
	Socket(int port);
	Socket(char* host, int port);
	void Send(char*   buf, int  size);
	void SendV(char**  buf, int* size);
	void Recv(char*&  buf, int& size);

private:
	int sockfd;
};
