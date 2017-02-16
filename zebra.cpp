#include <vcl.h>
#pragma hdrstop
#include <tchar.h>
//---------------------------------------------------------------------------
#include <Vcl.Styles.hpp>
#include <Vcl.Themes.hpp>
USEFORM("UI\TMainForm.cpp", MainForm);
//---------------------------------------------------------------------------
#include "mtkUtils.h"
#include "mtkVCLUtils.h"
#include "mtkIniKey.h"
#include "mtkRestartApplicationUtils.h"
#include "mtkLogger.h"
#include "mtkMoleculixException.h"
#include "mtkSQLite.h"
#pragma package(smart_init)

using namespace mtk;
using std::string;

extern HWND         gOtherAppWindow             = NULL;
extern bool 		gAppIsStartingUp 			= true;
extern string       gApplicationRegistryRoot    = "\\Software\\Allen Institute\\atUC7";
extern string       gDefaultAppTheme            = "Iceberg Classico";
extern string       gAppMutexName           	= "UC7AppMutex";
extern string       gRestartMutexName           = "UC7RestartMutex";
extern string       gFullDateTimeFormat         = "%Y-%m-%dT%H:%M:%S";
extern string       gDateFormat                 = "%Y-%m-%d";
extern string       gTimeFormat                 = "%H:%M:%S";

extern string       gCommonAppDataLocation      = ""; //Filled out later
extern string       gLogFileLocation            = "";
extern string       gLogFileName                = "atUC7.log";
//extern bool         gIsDevelopmentRelease       = true;
//extern bool         gHideSplash                 = true;
//extern TSplashForm* gSplashForm                 = NULL;

BOOL CALLBACK FindOtherWindow(HWND hwnd, LPARAM lParam) ;

void setupLogging();
//---------------------------------------------------------------------------
int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
    //The app mutex is used to check for already running instances
    try
    {
		// Initialize restart code
		// Check if this instance is restarted and
		// wait while previos instance finish
		if (mtk::checkForCommandLineFlag("--Restart"))
		{
			mtk::WaitForPreviousProcessToFinish(gRestartMutexName);
		}
        else
        {
            //Look at this later... does not work yet
            HANDLE appMutex = ::CreateMutexA(NULL, FALSE, gAppMutexName.c_str());
            int err = GetLastError();
            if( ERROR_ALREADY_EXISTS == err)
            {
                // Program already running somewhere
                ::EnumWindows(FindOtherWindow, NULL);

                if(gOtherAppWindow != NULL)
                {
                    //Send a custom message to restore window here..
                    ::SwitchToThisWindow(gOtherAppWindow, false);
                }

                return(0); // Exit program
            }
        }

        setupLogging();
        Log(lInfo) << "The Logfile was opened..";

        //Setup globals
        gCommonAppDataLocation = joinPath(getSpecialFolder(CSIDL_LOCAL_APPDATA), "atUC7");
        if(!folderExists(gCommonAppDataLocation))
        {
            Log(lError) << "The local app data folder("<<gCommonAppDataLocation<<") don't exists! Catastrophe..";
        }

        Application->Initialize();
        Application->MainFormOnTaskBar = true;

		TStyleManager::TrySetStyle(gDefaultAppTheme.c_str());
		Application->Title = "atUC7";
        Application->ProcessMessages();
		Application->CreateForm(__classid(TMainForm), &MainForm);
		Application->Run();

        // Finish restarting process if needed
        DoRestartProcessFinish();
	}
	catch (Exception &exception)
	{
		Application->ShowException(&exception);
	}
	catch(const MoleculixException &e)
	{
		throw Exception(e.what());
	}

	catch (...)
	{
		try
		{
			throw Exception("There was something bad happening! We have to close atDB.");
		}
		catch (Exception &exception)
		{
			Application->ShowException(&exception);
		}
	}
	return 0;
}

void setupLogging()
{
	//Get Application folder
	string fldr =  joinPath(getSpecialFolder(CSIDL_LOCAL_APPDATA), "atUC7");
	if(!folderExists(fldr))
	{
		createFolder(fldr);
	}

	gLogFileLocation = fldr;
	string fullLogFileName(joinPath(gLogFileLocation, gLogFileName));
	clearFile(fullLogFileName);
	mtk::gLogger.logToFile(fullLogFileName);
	LogOutput::mShowLogLevel = true;
	LogOutput::mShowLogTime = false;
	LogOutput::mUseLogTabs = false;
	Log(lInfo) << "Logger was setup";
}
