#ifndef atZebraH
#define atZebraH
#include "atABObject.h"
#include "serial/atSerial.h"
#include "atZebraMessage.h"
#include <deque>
#include "Poco/Mutex.h"
//---------------------------------------------------------------------------

using mtk::gEmptyString;
using std::deque;
class ZebraMessageConsumer;

class AT_CORE Zebra : public ABObject
{
	friend ZebraMessageConsumer;
	public:
										Zebra();
										~Zebra();

		bool							connect(int comPort);
        bool							disConnect();
        bool							isConnected();
        bool							getVersion();
        bool							sendRawMessage(const string& msg);

        bool							startCutter();
        bool							stopCutter();
        bool							getCutterStatus();
        int								calculateCheckSum(const string& cmd);
        bool							hasMessage();

    protected:
        string							mINIFileSection;
        int								mCOMPort;
    	Serial							mSerial;
        void							onSerialMessage(const string& msg);
        bool							sendZebraMessage(const ZebraMessageName& uc, const string& data1 = gEmptyString, const string& data2 = gEmptyString);

        Poco::Mutex						mBufferMutex;
        Poco::Condition					mNewMessageCondition;
        deque<ZebraMessage> 				mIncomingMessagesBuffer;


};

#endif
