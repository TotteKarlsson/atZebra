#pragma hdrstop
#include "atZebraMessage.h"
#include "mtkLogger.h"
//---------------------------------------------------------------------------
using namespace mtk;


int addUpDataToInt(const string& mData);

ZebraMessage::ZebraMessage(const string& cmd, bool hasCS)
:
mData()
//mIsResponse(isResponse)
{
	if(cmd.size())
    {
        if(!parse(cmd, hasCS))
        {
            Log(lError) << "The Zebra command: " <<cmd<<" failed to parse";
        }
    }
}

bool ZebraMessage::parse(const string& cmd, bool hasCS)
{
    return true;
}

string ZebraMessage::checksum() const
{
}

bool ZebraMessage::check() const
{
	//Check command against checksum for integrity
}

bool ZebraMessage::calculateCheckSum()
{
    //Calculate Checksum
//    string recSndrSum(mReceiver + mSender);
//    unsigned char sum = hexToDec(recSndrSum) + hexToDec(mCommandString);
//
//    int data = addUpDataToInt(mData);
//    sum += data;
//
//    //take two's complement, of a two byte variable
//    sum = ~sum + 1;
//
//    mCheckSum = mtk::toString((int) sum, "", 16);
    return true;
}

string ZebraMessage::getFullMessage() const
{

}

int addUpDataToInt(const string& mData)
{
	int sum(0);

    for(int i = 0; i < mData.size(); i += 2)
    {
        string val;
        val.push_back(mData[i]);
        val.push_back(mData[i+1]);
        sum += hexToDec(val);
    }
    return sum;
}

string ZebraMessage::getMessageNameAsString() const
{
	return toLongString(mCommandName);
}

ZebraMessageName toCommandName(const string& cmd, int controllerAddress)
{
}

string toShortString(ZebraMessageName cmd)
{
}

string toLongString(ZebraMessageName cmd)
{
}
