#pragma hdrstop
#include "atZebraMessageConsumer.h"
#include "mtkStringUtils.h"
#include "Poco/Mutex.h"
#include "mtkLogger.h"
#include "mtkUtils.h"
#include "atUC7.h"
#include "atUC7ApplicationMessages.h"
#include "atUC7DataStructures.h"
//---------------------------------------------------------------------------

using Poco::Mutex;
using namespace mtk;

//----------------------------------------------------------------
ZebraMessageConsumer::ZebraMessageConsumer(UC7& messageContainer,  HWND__ *h, const string& threadName)
:
mtk::Thread(threadName),
mAllowProcessing(true),
mUC7(messageContainer),
mProcessedCount(0),
mNotifyUI(NULL),
mProcessTimeDelay(1),
mHandle(h)
{}

//----------------------------------------------------------------
ZebraMessageConsumer::~ZebraMessageConsumer()
{
	if(mIsRunning)
    {
		stop();
    }
}

bool ZebraMessageConsumer::start(bool inThread)
{
	if(isRunning())
	{
		Log(lWarning) << "Tried to start a runnning thread";
		return true;
	}
	if(inThread)
	{
		return mtk::Thread::start();
	}
	else
	{
		worker();
		return true;
	}
}

void ZebraMessageConsumer::pauseProcessing()
{
	mAllowProcessing = false;
}

void ZebraMessageConsumer::resumeProcessing()
{
	mAllowProcessing = true;
}

void ZebraMessageConsumer::stop()
{
	//Sets time to die to true
	mtk::Thread::stop();

	//If thread is waiting.. get it out of wait state
	mUC7.mNewMessageCondition.signal();
}

void ZebraMessageConsumer::run()
{
	worker();
}

void ZebraMessageConsumer::worker()
{
	Log(lDebug)<<"Entering Command Processor Worker Function.";
	while(mIsTimeToDie == false)
	{
		{
			Poco::ScopedLock<Poco::Mutex> lock(mUC7.mBufferMutex);
			if(mUC7.mIncomingMessagesBuffer.size() == 0)
			{
				Log(lDebug3) << "Waiting for UC7 message.";
				mUC7.mNewMessageCondition.wait(mUC7.mBufferMutex);
			}

            while(mUC7.hasMessage() && mIsTimeToDie == false)
            {
				UC7Message* msg = new UC7Message;
	           	(*msg) = mUC7.mIncomingMessagesBuffer.front();

                mUC7.mIncomingMessagesBuffer.pop_front();
                if(!msg->check())
                {
                	Log(lError) << "Corrupted message";
                }
                else
                {
                    //Send windows message and let UI handle the message
                    int ret = PostMessage(mHandle, UWM_MESSAGE, 1, (long) msg);
                    if(!ret)
                    {
                        Log(lError) << "Post message failed..";
                    }
                }

                sleep(mProcessTimeDelay);
            }

		}//scoped lock
	}

    Log(lInfo) << "UC7 Message Processor finished";
	mIsFinished = true;
	mIsRunning = false;
}
