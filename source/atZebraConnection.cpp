#pragma hdrstop
#include "atZebraConnection.h"
#include "mtkIniFile.h"
#include "mtkLogger.h"
//---------------------------------------------------------------------------

using namespace mtk;

ZebraConnection::ZebraConnection()
:
	mINIFileSection("Zebra"),
    mCOMPort(-1),
    mSerial(-1, 19200, '[', ']', SerialPort::EHandshake::EHandshakeHardware)
{
	mSerial.assignMessageReceivedCallBack(onSerialMessage);
}

ZebraConnection::~ZebraConnection()
{
	disConnect();
}

bool ZebraConnection::connect(int com)
{
	Log(lInfo) << "Connecting Zebra client on COM"<<com;
    return mSerial.connect(com, 9600);
}

bool ZebraConnection::disConnect()
{
	return mSerial.disConnect();
}

bool ZebraConnection::isConnected()
{
	return mSerial.isConnected();
}

//This returns the checksum as a HEX number
int ZebraConnection::calculateCheckSum(const string& cmd)
{
	int cs = 0xFF;
	return cs;
}

bool ZebraConnection::getVersion()
{
	return "";
}

bool ZebraConnection::hasMessage()
{
	return mIncomingMessagesBuffer.size() ? true : false;
}

void ZebraConnection::onSerialMessage(const string& msg)
{
	Log(lDebug4) << "Decoding Zebra message: "<<msg;
    ZebraMessage cmd(msg, true);
}

bool ZebraConnection::sendRawMessage(const string& msg)
{
	Log(lInfo) << "Sending raw message: "<<msg;
	return mSerial.send(msg);
}

bool ZebraConnection::sendZebraMessage(const ZebraMessageName& msgName, const string& data1, const string& data2)
{
	stringstream cmd;

	//This function constructs a proper command to send to the Zebra
    switch(msgName)
    {

        default: break;

    }

    Log(lDebug4) << "Sending Zebra command: "<<cmd.str();
    return true;
}


