#include "Socket.h"
#include <assert.h>
#include <iostream>
#include <string>
#include <system_error>

//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
std::string GetLastErrorAsString()
{
    //Get the error message ID, if any.
    DWORD errorMessageID = ::GetLastError();
    if(errorMessageID == 0) {
        return std::string(); //No error message has been recorded
    }
    
    LPSTR messageBuffer = nullptr;

    //Ask Win32 to give us the string version of that message ID.
    //The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
    
    //Copy the error message into a std::string.
    std::string message(messageBuffer, size);
    
    //Free the Win32's string's buffer.
    LocalFree(messageBuffer);
            
    return message;
}

namespace winsock{
	static WSAJanitor janitor = WSAJanitor(); // by initialising it as static we ensure Winsock is enable until the end of the program
	
	// Janitor partern to handle Winsock initialisation
	WSAJanitor::WSAJanitor(){
		static bool has_been_init = false;
		if(!has_been_init){
			std::cout << "Init WSA" << std::endl;
			has_been_init = true;
		    WSAStartup(MAKEWORD(2,0), &data);
		}else{
			// "Trying to reinit WSA via WSAJanitor()"
			assert(false);
		}
	}
	
	WSAJanitor::~WSAJanitor(){
		std::cout << "Cleanup WSA" << std::endl;
		WSACleanup();
	}

	SocketClient::SocketClient(const char* inet, int port){
		struct hostent *remoteHost;
		struct in_addr addr;
		remoteHost = ResolveHost(inet);
		assert(remoteHost);
		addr.s_addr = *(u_long *) remoteHost->h_addr_list[0];
		std::cout << "Connecting to " << inet_ntoa(addr) << ":" << port << std::endl;
		socket_address.sin_addr.s_addr = inet_addr(inet_ntoa(addr));
		socket_address.sin_family = AF_INET;
		socket_address.sin_port = htons(port);
		socket_client = socket(AF_INET, SOCK_STREAM,0);
		assert(socket_client != SOCKET_ERROR);
	}
	
	SocketClient::~SocketClient(){
		if(connected)Disconnect();
	}
	
	struct hostent * SocketClient::ResolveHost(const char* inet){
		struct hostent *remoteHost;
		struct in_addr addr;
		remoteHost = gethostbyname(inet);
		if(remoteHost == NULL){
	        std::cout << "Error occured during ResolveHost connect: " << GetLastErrorAsString() << std::endl;
	        return nullptr;
	    }else{
	        return remoteHost;
	    }
	}
	
	void SocketClient::Connect(){
		if(connect(socket_client, (SOCKADDR *)&socket_address, sizeof(socket_address)) == 0){
			connected = true;
			std::cout << "Connect to host" << std::endl;
		}else{
			std::cout << "Error occured during connect: " << GetLastErrorAsString() << std::endl;
		}
	}
			
	void SocketClient::Disconnect(){
		if(connected){
			if( closesocket(socket_client) == 0){
				std::cout << "Disconnect from host" << std::endl;
			}else{
				std::cout << "Error occured during closesocket: " << GetLastErrorAsString() << std::endl;
			}
			
		}else{
			std::cout << "Cannot disconnect a not connected socket" << std::endl;
		}
		connected = false;
	}
						
	bool SocketClient::HasData(int timeout){
		std::cout << "Checking for data" << std::endl;
		int result = PollSocketState(timeout);
		if(connected){
			if(result > 0 && poll.revents & POLLRDNORM){
				return true;
			}
		}
		return false;
	}
	
	bool SocketClient::ReadyToSend(int timeout){
		int result = PollSocketState(timeout);
		if(connected){
			if(result > 0 && poll.revents & POLLWRNORM){
				return true;
			}
		}
		return false;
	}
	
	bool SocketClient::IsError(){
		int result = PollSocketState(0);
		if( result == -1){
			return true;
		}
		return false;
	}
	
	bool SocketClient::IsConnected(){
		return connected;
	}
	
	int SocketClient::PollSocketState(int timeout){
		// This is the only function with disconnect() which is capable of closing the connection
		if(connected){
			poll.fd = socket_client;
			poll.events = POLLRDNORM | POLLWRNORM;
			int result = WSAPoll(&poll, 1, timeout);
			if(result == SOCKET_ERROR){
				std::cout << "Error occured during receiveData: " << GetLastErrorAsString() << std::endl;
				Disconnect();
				return -1;
			}else if(poll.revents & (POLLERR | POLLHUP | POLLNVAL)){
				Disconnect();
				return -1;
			}
			return result;
		}
		return 0;
	}
	
	int SocketClient::SendData(const char* buffer, int size){
		int byte_send = 0;
		if((byte_send = send(socket_client, buffer, size, 0)) == SOCKET_ERROR){
			std::cout << "Error occured during SendData: " << GetLastErrorAsString() << std::endl;
		}else{
			std::cout << "Data sent " << byte_send << std::endl;
		}
		return byte_send;
	}
	
	int SocketClient::ReceiveData(char* buffer, int maxSize, int timeout_ms){
		//TODO: Add HAS DATA BEFORE TRYING TO READ
		if(HasData(timeout_ms)){
			std::cout << "Receiving data" << std::endl;
			int recv_byte = 0;
			if((recv_byte = recv(socket_client, buffer, maxSize, 0)) == SOCKET_ERROR){
				std::cout << "Error occured during receiveData: " << GetLastErrorAsString() << std::endl;
			}
			std::cout << "Receiving end" << std::endl;
			return recv_byte;
		}
		return 0;
	}

}