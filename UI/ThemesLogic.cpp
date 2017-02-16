#include <vcl.h>
#pragma hdrstop
#include "TMainForm.h"
#include "mtkRestartApplicationUtils.h"
#include "mtkVCLUtils.h"

using namespace mtk;
extern string gRestartMutexName ;
extern string gDefaultAppTheme;
//---------------------------------------------------------------------------
void __fastcall TMainForm::PopulateStyleMenu()
{
    //Populate styles menu
    string themeFolder("styles");
    themeFolder = joinPath(getCWD(), themeFolder);

    //Populate menu with styles in the style manager
    //Add to menu
    System::DynamicArray<System::UnicodeString> aList = TStyleManager::StyleNames;
    String activeStyle = TStyleManager::ActiveStyle->Name;
    for(int i = 0; i < aList.Length; i++)
    {
        String name = TStyleManager::StyleNames[i];

//        TMenuItem *Item = new TMenuItem(ThemesMenu);
//        Item->Caption = name;
//        Item->OnClick = ThemesMenuClick;
//        ThemesMenu->Add(Item);
//        if(Item->Caption == activeStyle)
//        {
//            Item->Checked = true;
//        }
    }
}

void __fastcall TMainForm::ThemesMenuClick(TObject *Sender)
{
    TMenuItem* anItem = dynamic_cast<TMenuItem*>(Sender);
    if(!anItem)
    {
        return;
    }

    TReplaceFlags rFlags(rfIgnoreCase|rfReplaceAll);
    String styleName = StringReplace(anItem->Caption, "&", "", rFlags);

    int mrResult = MessageDlg("Changing theme require restart of the Application.\nRestart?", mtCustom, TMsgDlgButtons() << mbOK<<mbCancel, 0);

    if(mrResult == mrOk)
    {
        if (!ActivateApplicationStyleChange(gRestartMutexName, stdstr(styleName)))
        {
            ::MessageBox(NULL, TEXT("Something Went Wrong!"),
                         TEXT("Restart App"),
                         MB_OK|MB_ICONEXCLAMATION);
            return ;
        }

        gDefaultAppTheme = stdstr(styleName);
        //Write to registry
        writeStringToRegistry(gApplicationRegistryRoot, "", "Theme", gDefaultAppTheme);

        // Terminate application.
        Close();
    }
}

