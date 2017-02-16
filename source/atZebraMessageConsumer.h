#ifndef atZebraMessageConsumerH
#define atZebraMessageConsumerH
#include "atExporter.h"
#include <string>
#include "mtkThread.h"
#include "atABObject.h"
#include "mtkConstants.h"
#include "mtkStringList.h"
#include "atZebraMessageConsumer.h"

//---------------------------------------------------------------------------
class Zebra;
using mtk::gEmptyString;

typedef void __fastcall (__closure *UICallback)(void);

class AT_CORE ZebraMessageConsumer : public ABObject, public mtk::Thread
{
    public:
                                                    ZebraMessageConsumer(Zebra& list,  HWND__ *h, const string& threadName = gEmptyString);
                                                    ~ZebraMessageConsumer();
        bool                                        openDataBase(const string& db);

                                                    // overridden from Thread
        void                                        run();
        virtual void                                worker();
        void                                        stop();
        bool                                        start(bool in_thread = true);
        void                                        pauseProcessing();
        void                                        resumeProcessing();
        UICallback                                  mNotifyUI;

	protected:
		long                                        mProcessedCount;
		bool                                        mAllowProcessing;
        double										mProcessTimeDelay;
		Zebra&                     					mZebra;

        											//The handle is needed for window messaging
        HWND__*										mHandle;
};

#endif
