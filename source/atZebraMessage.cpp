#pragma hdrstop
#include "atZebraMessage.h"
#include "mtkLogger.h"
//---------------------------------------------------------------------------
using namespace mtk;

int hexToDec(const string& hex);
int addUpDataToInt(const string& mData);

ZebraMessage::ZebraMessage(const string& cmd, bool hasCS)
:
mReceiver(""),
mSender(""),
mData(""),
mCheckSum(""),
mCommandString("")//,
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
	//A command has to be at least 4 chars long
    int minCmdSize = 4;

    if(cmd.size() < minCmdSize)
    {
    	Log(lError) << "Zebra command is to short";
        return false;
    }

    //First letter is the receiver
    mReceiver = cmd[0];

    //Second letter is the sender
    mSender = cmd[1];

    //Next two chars is the command
    mCommandString.push_back(cmd[2]);
    mCommandString.push_back(cmd[3]);
    mCommandName = toCommandName(mCommandString, toInt(mSender));

    //The data is everything after the command and before the checksum
    if(cmd.size() == minCmdSize)
    {
    	mData = "";
    }
    else
    {
        int lengthOfData;
        if(hasCS)
        {
	    	lengthOfData = cmd.size() - (minCmdSize + 2); //check sum is two bytes
        }
        else
        {
	    	lengthOfData = cmd.size() - (minCmdSize);
        }

    	mData = cmd.substr(4, lengthOfData);
    }


    if(hasCS)
    {
    	mCheckSum.push_back(cmd[cmd.size() - 2]);
	    mCheckSum.push_back(cmd[cmd.size() - 1]);
    }
    else
    {
    	calculateCheckSum();
    }
    return true;
}

string ZebraMessage::receiver() const
{
	return mReceiver;
}

string ZebraMessage::sender() const
{
	return mSender;
}

string ZebraMessage::command()  const
{
	return mCommandString;
}

string ZebraMessage::data()  const
{
	return mData;
}

string ZebraMessage::checksum() const
{
	return mCheckSum;
}

bool ZebraMessage::check() const
{
	//Check command against checksum for integrity
    string recSndrSum(mReceiver + mSender);
    unsigned char sum = hexToDec(recSndrSum) + hexToDec(mCommandString);

	int data = addUpDataToInt(mData);
    sum += data;

    //take two's complement, of a two byte variable
    sum = ~sum + 1;

    unsigned int cs ( hexToDec(mCheckSum));
    unsigned int test = sum;
    return (test == cs ) ? true : false;
}

bool ZebraMessage::calculateCheckSum()
{
    //Calculate Checksum
    string recSndrSum(mReceiver + mSender);
    unsigned char sum = hexToDec(recSndrSum) + hexToDec(mCommandString);

    int data = addUpDataToInt(mData);
    sum += data;

    //take two's complement, of a two byte variable
    sum = ~sum + 1;

    mCheckSum = mtk::toString((int) sum, "", 16);
    return true;
}

string ZebraMessage::getFullMessage() const
{
	return '!' + mReceiver + mSender + mCommandString + mData + mCheckSum + 'D';
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
	switch(controllerAddress)
    {
    	case 1: 		return UNKNOWN;
        case 2:			return UNKNOWN;
        case 3:			return UNKNOWN;
        case 4:
        {
        	if(cmd == "20")
            {
            	return FEEDRATE_MOTOR_CONTROL;
            }
            else if(cmd == "21")
            {
            	return SEND_POSITION_AT_MOTION;
            }
            else if(cmd == "23")
            {
            	return FEED;
            }
            else if(cmd == "30")
            {
            	return NORTH_SOUTH_MOTOR_MOVEMENT;
            }
            else if(cmd == "31")
            {
            	return SEND_POSITION_AT_MOVEMENT_NORTH_SOUTH;
            }
            else if(cmd == "40")
            {
            	return EAST_WEST_MOTOR_MOVEMENT;
            }
            else if(cmd == "41")
            {
            	return SEND_POSITION_AT_MOVEMENT_EAST_WEST;
            }
			else
            {
            	return UNKNOWN;
            }
        }
		case 5:
        {
        	if(cmd == "20")
            {
            	return CUTTING_MOTOR_CONTROL;
            }
        	else if(cmd == "30")
            {
            	return CUTTING_SPEED;
            }
        	else if(cmd == "31")
            {
            	return RETURN_SPEED;
            }
        	else if(cmd == "40")
            {
            	return HANDWHEEL_POSITION;
            }
            else
            {
            	return UNKNOWN;
            }
        }

        default: return UNKNOWN;
    }
}

string toShortString(ZebraMessageName cmd)
{
    switch(cmd)
    {
        case SOFTWARE_RESET:		                        return "F0";
        case GET_PART_ID:			                        return "F1";
        case LOGIN:					                        return "F2";
        case COMMAND_TRANSMISSION_ERROR:                    return "F3";
        case GET_VERSION:                                   return "F5";

        //Controller #4
        case FEEDRATE_MOTOR_CONTROL:                        return "20";
        case SEND_POSITION_AT_MOTION:                       return "21";
        case FEED:                                          return "23";
        case NORTH_SOUTH_MOTOR_MOVEMENT:                    return "30";
        case SEND_POSITION_AT_MOVEMENT_NORTH_SOUTH:         return "31";
        case EAST_WEST_MOTOR_MOVEMENT:                      return "40";
        case SEND_POSITION_AT_MOVEMENT_EAST_WEST:           return "41";

        //Controller #5
        case CUTTING_MOTOR_CONTROL:                         return "20";
        case CUTTING_SPEED:                                 return "30";
        case RETURN_SPEED:                                  return "31";
        case HANDWHEEL_POSITION:                            return "40";

        default:
        	Log(lError) << "UNKNOWN Zebra message: "<<cmd;    return "UNKNOWN";
    }
}

string toLongString(ZebraMessageName cmd)
{
    switch(cmd)
    {
        case SOFTWARE_RESET:		                        return "Software Reset";
        case GET_PART_ID:			                        return "Get part ID (WKZ)";
        case LOGIN:					                        return "Login";
        case COMMAND_TRANSMISSION_ERROR:                    return "Command-/Transmission error";
        case GET_VERSION:                                   return "Get version";

        //Controller #4
        case FEEDRATE_MOTOR_CONTROL:                        return "Feedrate motoro control";
        case SEND_POSITION_AT_MOTION:                       return "Send position at motion";
        case FEED:                                          return "Feed";
        case NORTH_SOUTH_MOTOR_MOVEMENT:                    return "North/South Motor movement";
        case SEND_POSITION_AT_MOVEMENT_NORTH_SOUTH:         return "Send Position at movement (North/South)";
        case EAST_WEST_MOTOR_MOVEMENT:                      return "East/West motor movement";
        case SEND_POSITION_AT_MOVEMENT_EAST_WEST:           return "Send position at movement East/West";

        //Controller #5
        case CUTTING_MOTOR_CONTROL:                         return "Cutting motor control";
        case CUTTING_SPEED:                                 return "Cutting Speed";
        case RETURN_SPEED:                                  return "Return Speed";
        case HANDWHEEL_POSITION:                            return "Handwheel position";

        default:
        	Log(lError) << "UNKNOWN Zebra message: "<<cmd;	return "UNKNOWN";
    }
}

int hexToDec(const string& hex)
{
	stringstream convert(hex);
    int val;
    convert >> std::hex >> val;
    return val;
}

