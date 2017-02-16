#ifndef atZebraMessageH
#define atZebraMessageH
#include "atABObject.h"
//---------------------------------------------------------------------------

//!Simple enum to hold commands
enum ZebraMessageName
				{SOFTWARE_RESET                             = 0,
				 GET_PART_ID 	                            ,
                 LOGIN			                            ,
                 COMMAND_TRANSMISSION_ERROR                 ,
                 GET_VERSION	                            ,

                 FEEDRATE_MOTOR_CONTROL						,
                 SEND_POSITION_AT_MOTION                    ,
                 FEED                                       ,

                 NORTH_SOUTH_MOTOR_MOVEMENT                 ,
                 SEND_POSITION_AT_MOVEMENT_NORTH_SOUTH      ,

                 EAST_WEST_MOTOR_MOVEMENT                   ,
				 SEND_POSITION_AT_MOVEMENT_EAST_WEST        ,

                 CUTTING_MOTOR_CONTROL                      ,
                 CUTTING_SPEED                              ,
                 RETURN_SPEED                               ,
                 HANDWHEEL_POSITION							,
                 UNKNOWN
                };

ZebraMessageName toCommandName(const string& cmd, int controllerAddress);
string toShortString(ZebraMessageName cmd);
string toLongString(ZebraMessageName cmd);

class AT_CORE ZebraMessage : public ABObject
{
	public:
					            ZebraMessage(const string& cmd = "", bool hasCS = false);

        string		            receiver() const;
        string		            sender() const;
		string		            command() const;
		string					data() const;
		string		            getFullMessage() const;
        string					checksum() const;
        bool		            check() const;
        string					getMessageNameAsString() const;
        bool					calculateCheckSum();

	private:
    	string 					mReceiver;
        string 					mSender;
        string					mData;
        string					mCheckSum;
        string 					mCommandString;
//        bool					mIsResponse;
        ZebraMessageName			mCommandName;
        bool					parse(const string& cmd, bool isResponse = false);
};

#endif
