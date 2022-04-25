#ifndef _IRCClient_IRCClient_h_
#define _IRCClient_IRCClient_h_
#include <queue>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include "Socket.h"

namespace irc{
		
	class IRCClient{
		public:
			IRCClient(const char* serveur, int port, const char* pseudo);
			
			std::string ADMIN(bool use_prefix=false);
			std::string WHO(bool use_prefix=false);
			std::string USER(const char* username, const char* hostname, const char* servername, const char* realname, bool use_prefix=false);
			std::string NICK(const char* nickname, bool use_prefix=false);
			std::string PRIVMSG(const char* target, const char* message, bool use_prefix=false);
			std::string ReceiveCommand(int timeout_ms = 3000);
			
		private:
			std::unique_ptr<std::thread> rx_thread;
			std::queue<std::string> fifo;

			void RxRoutine();
			std::string PrepareCommand(const std::string& cmd, const std::vector<std::string>& args = std::vector<std::string>(), bool use_prefix=false);
			bool SendCommand(const std::string& cmd);
			
			winsock::SocketClient client;
			const char* pseudo;
			
			const int MAX_SIZE_CMD = 512;
			const char* CMD_TERMINAISON = "\r\n";
			const char* CMD_SPACE = " ";
	};
	
}

#endif
