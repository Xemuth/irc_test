#include "IRCClient.h"
#include <assert.h>
#include <iostream>
#include <chrono>
#include <algorithm>

std::mutex lock;

namespace irc{

IRCClient::IRCClient(const char* serveur, int port, const char* pseudo) : client(serveur, port), pseudo(pseudo){
	client.Connect();
	rx_thread = std::unique_ptr<std::thread>(new std::thread(&IRCClient::RxRoutine, this));
}

std::string IRCClient::ADMIN(bool use_prefix){
	if(SendCommand(PrepareCommand("ADMIN", {}, use_prefix)))
		return ReceiveCommand();
	return std::string();
}

std::string IRCClient::WHO(bool use_prefix){
	if(SendCommand(PrepareCommand("WHO", {}, use_prefix)))
		return ReceiveCommand();
	return std::string();
}

std::string IRCClient::USER(const char* username, const char* hostname, const char* servername, const char* realname, bool use_prefix){
	if(SendCommand(PrepareCommand("USER", {username, hostname, servername, realname}, use_prefix)))
		return ReceiveCommand();
	return std::string();
}

std::string IRCClient::NICK(const char* nickname, bool use_prefix){
	if(SendCommand(PrepareCommand("NICK", {nickname}, use_prefix)))
		return ReceiveCommand();
	return std::string();
}

std::string IRCClient::PRIVMSG(const char* target, const char* message, bool use_prefix){
	if(SendCommand(PrepareCommand("PRIVMSG", {target, message}, use_prefix)))
		return ReceiveCommand();
	return std::string();
}

std::string IRCClient::PrepareCommand(const std::string& cmd, const std::vector<std::string>& args, bool use_prefix){
	std::string final_cmd;
	if(use_prefix){
		final_cmd.append(":");
		final_cmd.append(pseudo);
		final_cmd.append(CMD_SPACE);
	}
	final_cmd.append(cmd);
	final_cmd.append(CMD_SPACE);
	for(const std::string& str: args){
		final_cmd.append(str);
		final_cmd.append(CMD_SPACE);
	}
	final_cmd.append(CMD_TERMINAISON);
	assert(final_cmd.length() < MAX_SIZE_CMD);
	return final_cmd;
}

std::string IRCClient::ReceiveCommand(int timeout_ms){
	for(int ratio = 0; ratio < timeout_ms; ratio++){
		if(fifo.size() > 0){
			std::unique_lock<std::mutex> lk(lock);
			std::string str = std::move(fifo.front());
			fifo.pop();
			return std::move(str);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(ratio));
	}
	return std::string();
}

bool IRCClient::SendCommand(const std::string& cmd){
	if(client.IsConnected()){
		std::cout << "Sending command: " << cmd;
		std::unique_lock<std::mutex> lk(lock);
		int data_sent = client.SendData(cmd.c_str(), cmd.length());
		return data_sent;
	}
	return false;
}

void IRCClient::RxRoutine(){
	std::cout << "entering thread" << std::endl;
	static char buffer[512];
	for(;;){
		if(client.IsError() || !client.IsConnected())
			break;
		if(client.HasData(1000)){
			std::unique_lock<std::mutex> lk(lock);
			memset(buffer, '\0', 512);
			client.ReceiveData(buffer, 512);
			std::string received(buffer);
			std::cout <<  "Received : " << received << std::endl;
			fifo.push(std::move(received));
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	std::cout << "exiting thread" << std::endl;
}

}