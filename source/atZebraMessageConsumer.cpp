#pragma hdrstop
#include "atZebraMessageConsumer.h"
#include "mtkStringUtils.h"
#include "Poco/Mutex.h"
#include "mtkLogger.h"
#include "mtkUtils.h"
#include "atZebra.h"
#include "atZebraApplicationMessages.h"
#include "atZebraDataStructures.h"
//---------------------------------------------------------------------------

using Poco::Mutex;
using namespace mtk;

//----------------------------------------------------------------
ZebraMessageConsumer::ZebraMessageConsumer(Zebra& messageContainer,  HWND__ *h, const string& threadName)
:
mtk::Thread(threadName),
mAllowProcessing(true),
mZebra(messageContainer),
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
	mZebra.mNewMessageCondition.signal();
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
			Poco::ScopedLock<Poco::Mutex> lock(mZebra.mBufferMutex);
			if(mZebra.mIncomingMessagesBuffer.size() == 0)
			{
				Log(lDebug3) << "Waiting for Zebra message.";
				mZebra.mNewMessageCondition.wait(mZebra.mBufferMutex);
			}

            while(mZebra.hasMessage() && mIsTimeToDie == false)
            {
				ZebraMessage* msg = new ZebraMessage;
	           	(*msg) = mZebra.mIncomingMessagesBuffer.front();

                mZebra.mIncomingMessagesBuffer.pop_front();
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

    Log(lInfo) << "Zebra Message Processor finished";
	mIsFinished = true;
	mIsRunning = false;
}
