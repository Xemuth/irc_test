#ifndef _IRCClient_Socket_h_
#define _IRCClient_Socket_h_
#include <winsock2.h>

namespace winsock{

	struct WSAJanitor{
		// Janitor partern to handle Winsock initialisation
		WSAJanitor();
		~WSAJanitor();
		WSAData data;
	};
	
	class SocketClient{
		public:
			SocketClient(const char* inet, int port);
			~SocketClient();
			
			void Connect();
			void Disconnect();
			
			int SendData(const char* buffer, int size); //Send the data provided as buffer
			int ReceiveData(char* buffer, int maxSize, int timeout_ms = 1000); // Return the amount of data readen
			
			bool HasData(int timeout = 0); // When true, then their is something to read in the buffer
			bool ReadyToSend(int timeout = 0); // When True, then socket is ready to send data
			bool IsError();
			bool IsConnected();
			
		private:
			int PollSocketState(int timeout);
			
			struct hostent * ResolveHost(const char* inet);
			bool connected = false;
			WSAPOLLFD poll;
			SOCKET socket_client;
			SOCKADDR_IN socket_address;
	};

}
#endif
