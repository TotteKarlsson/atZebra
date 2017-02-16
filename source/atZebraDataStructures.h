#ifndef atZebraDataStructuresH
#define atZebraDataStructuresH
#include "Poco/DateTime.h"
#include <System.hpp>

enum ApplicationMessageEnum
{
    atZebraSplashWasClosed = 0,
    atZebraMessage
};


//struct AppMessageStruct;

struct ATWindowStructMessage
{
	Cardinal            Msg;
    					//
	int                 wparam;

    					//This is our data
    void*				lparam;
	LRESULT             Result;
};

//struct AppMessageStruct
//{
//							//The Message enum encodes what is passed by the void pointer
//	ApplicationMessageEnum 	mMessageEnum;
//	void*                   mData;
//
//};
//

#endif
