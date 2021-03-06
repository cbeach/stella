//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef OPTIONS_DIALOG_HXX
#define OPTIONS_DIALOG_HXX

class CommandSender;
class DialogContainer;
class GuiObject;
class OSystem;

#include "Dialog.hxx"
#include "VideoDialog.hxx"
#include "AudioDialog.hxx"
#include "InputDialog.hxx"
#include "UIDialog.hxx"
#include "SnapshotDialog.hxx"
#include "ConfigPathDialog.hxx"
#include "RomAuditDialog.hxx"
#include "GameInfoDialog.hxx"
#ifdef CHEATCODE_SUPPORT
#include "CheatCodeDialog.hxx"
#endif
#include "HelpDialog.hxx"
#include "AboutDialog.hxx"
#include "LoggerDialog.hxx"
#include "bspf.hxx"

class OptionsDialog : public Dialog
{
  public:
    OptionsDialog(OSystem& osystem, DialogContainer& parent, GuiObject* boss,
                  int max_w, int max_h, bool global);
    virtual ~OptionsDialog() = default;

  private:
    void loadConfig() override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    unique_ptr<VideoDialog>      myVideoDialog;
    unique_ptr<AudioDialog>      myAudioDialog;
    unique_ptr<InputDialog>      myInputDialog;
    unique_ptr<UIDialog>         myUIDialog;
    unique_ptr<SnapshotDialog>   mySnapshotDialog;
    unique_ptr<ConfigPathDialog> myConfigPathDialog;
    unique_ptr<RomAuditDialog>   myRomAuditDialog;
    unique_ptr<GameInfoDialog>   myGameInfoDialog;
  #ifdef CHEATCODE_SUPPORT
    unique_ptr<CheatCodeDialog>  myCheatCodeDialog;
  #endif
    unique_ptr<LoggerDialog>     myLoggerDialog;
    unique_ptr<HelpDialog>       myHelpDialog;
    unique_ptr<AboutDialog>      myAboutDialog;

    ButtonWidget* myRomAuditButton;
    ButtonWidget* myGameInfoButton;
    ButtonWidget* myCheatCodeButton;

    // Indicates if this dialog is used for global (vs. in-game) settings
    bool myIsGlobal;

    enum {
      kVidCmd      = 'VIDO',
      kAudCmd      = 'AUDO',
      kInptCmd     = 'INPT',
      kUsrIfaceCmd = 'URIF',
      kSnapCmd     = 'SNAP',
      kCfgPathsCmd = 'CFGP',
      kAuditCmd    = 'RAUD',
      kInfoCmd     = 'INFO',
      kCheatCmd    = 'CHET',
      kLoggerCmd   = 'LOGG',
      kHelpCmd     = 'HELP',
      kAboutCmd    = 'ABOU',
      kExitCmd     = 'EXIM'
    };

  private:
    // Following constructors and assignment operators not supported
    OptionsDialog() = delete;
    OptionsDialog(const OptionsDialog&) = delete;
    OptionsDialog(OptionsDialog&&) = delete;
    OptionsDialog& operator=(const OptionsDialog&) = delete;
    OptionsDialog& operator=(OptionsDialog&&) = delete;
};

#endif
