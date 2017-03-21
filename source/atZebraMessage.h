#ifndef atZebraMessageH
#define atZebraMessageH
#include "atABObject.h"
#include <vector>
//---------------------------------------------------------------------------

using std::vector;
//!Simple enum to hold commands
enum ZebraMessageName
				{
                 UNKNOWN
                };

ZebraMessageName toCommandName(const string& cmd, int controllerAddress);
string toShortString(ZebraMessageName cmd);
string toLongString(ZebraMessageName cmd);

class AT_CORE ZebraMessage : public ABObject
{
	public:
					            ZebraMessage(const string& cmd = "", bool hasCS = false);

		string		            command() const;
		string					data() const;
		string		            getFullMessage() const;
        string					checksum() const;
        bool		            check() const;
        string					getMessageNameAsString() const;
        bool					calculateCheckSum();

	private:
        vector<unsigned char>	mData;
        ZebraMessageName	   	mCommandName;
        bool					parse(const string& cmd, bool isResponse = false);
};

#endif
