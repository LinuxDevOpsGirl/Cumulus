/* 
	Copyright 2010 OpenRTMFP
 
	This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License received along this program for more
	details (or else see http://www.gnu.org/licenses/).

	This file is a part of Cumulus.
*/

#include "LUAInvoker.h"
#include "LUAClients.h"
#include "LUAPublication.h"
#include "LUAPublications.h"
#include "LUAGroups.h"
#include "LUATCPClient.h"
#include "LUATCPServer.h"
#include "LUAMail.h"
#include "Server.h"
#include "Poco/Net/StreamSocket.h"
#include "math.h"
#include "LUAServers.h"

using namespace Cumulus;
using namespace Poco;
using namespace std;

const char*		LUAInvoker::Name="Cumulus::Invoker";

int	LUAInvoker::Publish(lua_State *pState) {
	SCRIPT_CALLBACK(Invoker,LUAInvoker,invoker)
		string name = SCRIPT_READ_STRING("");
		try {
			SCRIPT_WRITE_PERSISTENT_OBJECT(Publication,LUAPublication,invoker.publish(name))
			lua_getmetatable(pState,-1);
			lua_pushlightuserdata(pState,&invoker);
			lua_setfield(pState,-2,"__invoker");
			lua_pop(pState,1);
		} catch(Exception& ex) {
			SCRIPT_ERROR("%s",ex.displayText().c_str())
			SCRIPT_WRITE_NIL
		}
	SCRIPT_CALLBACK_RETURN
}

int	LUAInvoker::AbsolutePath(lua_State *pState) {
	SCRIPT_CALLBACK(Invoker,LUAInvoker,invoker)
		SCRIPT_WRITE_STRING((Server::WWWPath+SCRIPT_READ_STRING("")+"/").c_str())
	SCRIPT_CALLBACK_RETURN
}

int	LUAInvoker::CreateTCPClient(lua_State *pState) {
	SCRIPT_CALLBACK(Invoker,LUAInvoker,invoker)
		SCRIPT_WRITE_OBJECT(LUATCPClient,LUATCPClient,*(new LUATCPClient(invoker.sockets,pState)))
		SCRIPT_ADD_DESTRUCTOR(&LUATCPClient::Destroy);
	SCRIPT_CALLBACK_RETURN
}

int	LUAInvoker::CreateTCPServer(lua_State *pState) {
	SCRIPT_CALLBACK(Invoker,LUAInvoker,invoker)
		SCRIPT_WRITE_OBJECT(LUATCPServer,LUATCPServer,*(new LUATCPServer(invoker.sockets,pState)))
		SCRIPT_ADD_DESTRUCTOR(&LUATCPServer::Destroy);
	SCRIPT_CALLBACK_RETURN
}

int	LUAInvoker::ToAMF(lua_State *pState) {
	SCRIPT_CALLBACK(Invoker,LUAInvoker,invoker)
		BinaryStream stream;
		Cumulus::BinaryWriter rawWriter(stream);
		AMFWriter writer(rawWriter);
		SCRIPT_READ_AMF(writer)
		SCRIPT_WRITE_BINARY(stream.data(),stream.size())
	SCRIPT_CALLBACK_RETURN
}

int	LUAInvoker::ToAMF0(lua_State *pState) {
	SCRIPT_CALLBACK(Invoker,LUAInvoker,invoker)
		BinaryStream stream;
		Cumulus::BinaryWriter rawWriter(stream);
		AMFWriter writer(rawWriter);
		writer.amf0Preference=true;
		SCRIPT_READ_AMF(writer)
		SCRIPT_WRITE_BINARY(stream.data(),stream.size())
	SCRIPT_CALLBACK_RETURN
}

int	LUAInvoker::FromAMF(lua_State *pState) {
	SCRIPT_CALLBACK(Invoker,LUAInvoker,invoker)
		SCRIPT_READ_BINARY(data,size)
		PacketReader packet(data,size);
		AMFReader reader(packet);
		SCRIPT_WRITE_AMF(reader,0)
	SCRIPT_CALLBACK_RETURN
}

int	LUAInvoker::SendMail(lua_State* pState) {
	SCRIPT_CALLBACK(Invoker,LUAInvoker,invoker)
		string sender = SCRIPT_READ_STRING("");
		string subject = SCRIPT_READ_STRING("");
		string content = SCRIPT_READ_STRING("");
		list<string> recipients;
		while(SCRIPT_CAN_READ)
			recipients.push_back(SCRIPT_READ_STRING(""));

		LUAMail* pMail = new LUAMail(pState);
		((Server&)invoker).mails.send(sender,recipients,subject,content,pMail);

		SCRIPT_WRITE_PERSISTENT_OBJECT(LUAMail,LUAMail,*pMail)

	SCRIPT_CALLBACK_RETURN
}


int LUAInvoker::Get(lua_State *pState) {
	SCRIPT_CALLBACK(Invoker,LUAInvoker,invoker)
		string name = SCRIPT_READ_STRING("");
		if(name=="clients") {
			SCRIPT_WRITE_PERSISTENT_OBJECT(Entities<Client>,LUAClients,invoker.clients)
		} else if(name=="publicAddress") {
			SCRIPT_WRITE_STRING(((ServerHandler&)invoker).publicAddress().c_str())
		} else if(name=="groups") {
			SCRIPT_WRITE_PERSISTENT_OBJECT(Entities<Group>,LUAGroups,invoker.groups)
		} else if(name=="publications") {
			SCRIPT_WRITE_PERSISTENT_OBJECT(Publications,LUAPublications,invoker.publications)
		} else if(name=="servers") {
			SCRIPT_WRITE_PERSISTENT_OBJECT(Servers,LUAServers,((Server&)invoker).servers)
		} else if(name=="publish") {
			SCRIPT_WRITE_FUNCTION(&LUAInvoker::Publish)
		} else if(name=="toAMF") {
			SCRIPT_WRITE_FUNCTION(&LUAInvoker::ToAMF)
		} else if(name=="toAMF0") {
			SCRIPT_WRITE_FUNCTION(&LUAInvoker::ToAMF0)
		} else if(name=="fromAMF") {
			SCRIPT_WRITE_FUNCTION(&LUAInvoker::FromAMF)
		} else if(name=="absolutePath") {
			SCRIPT_WRITE_FUNCTION(&LUAInvoker::AbsolutePath)
		} else if(name=="epochTime") {
			SCRIPT_WRITE_NUMBER(ROUND(Timestamp().epochMicroseconds()/1000))
		} else if(name=="createTCPClient") {
			SCRIPT_WRITE_FUNCTION(&LUAInvoker::CreateTCPClient)
		} else if(name=="createTCPServer") {
			SCRIPT_WRITE_FUNCTION(&LUAInvoker::CreateTCPServer)
		} else if(name=="configs") {
			lua_getglobal(pState,"cumulus.configs");
		} else if(name=="sendMail") {
			SCRIPT_WRITE_FUNCTION(&LUAInvoker::SendMail)
		}
	SCRIPT_CALLBACK_RETURN
}

int LUAInvoker::Set(lua_State *pState) {
	SCRIPT_CALLBACK(Invoker,LUAInvoker,invoker)
		string name = SCRIPT_READ_STRING("");
		lua_rawset(pState,1); // consumes key and value
	SCRIPT_CALLBACK_RETURN
}

