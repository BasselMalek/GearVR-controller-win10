// DO NOT CHANGE INCLUDE ORDER
#include <wx/app.h>
#include <wx/taskbar.h>
#include <wx/wxprec.h>
#include <GearVRController.h>
#include <thread>

enum {
  ID_START = 0,
  ID_STOP = 2,
  ID_EXIT = 1,
  ID_OPENCONFIG = 3,
  ID_CURSOR_OFF = 4,
  ID_CURSOR_TOUCHPAD_MODE = 5,
  ID_CURSOR_FUSION_MODE = 6,
  ID_BUTTON_OFF = 7,
  ID_BUTTON_SIMPLE_MODE = 8,
  ID_BUTTON_DPAD_MODE = 9,
  ID_ABOUT = 10,
};

class TrayApp : public wxApp {
public:
  std::jthread *controllerWorker;
  GearVRController *controllerObject;
  virtual bool OnInit() override;
  virtual int OnExit() override;
};

DECLARE_APP(TrayApp);

class TrayIcon : public wxTaskBarIcon {
public:
  TrayIcon(GearVRController *object, std::jthread *worker)
      : controllerObjectRef(object), controllerWorkerRef(worker) {
    GetFullPathNameW(L"config.ini", MAX_PATH, this->configFilePath, NULL);
  };
  wchar_t configFilePath[MAX_PATH];
  GearVRController *controllerObjectRef;
  std::jthread *controllerWorkerRef;
  int cursorMode = 0;
  int buttonMode = 0;
  bool isRunning = 0;

protected:
  wxMenu *CreatePopupMenu() override {
    wxMenu *mainMenu = new wxMenu();
    wxMenu *confgSubmenu = new wxMenu();
    wxMenu *cursorSubmenu = new wxMenu();
    wxMenu *buttonSubmenu = new wxMenu();

    cursorSubmenu->AppendRadioItem(ID_CURSOR_OFF, "OFF");
    cursorSubmenu->Check(ID_CURSOR_OFF, cursorMode == 0);
    cursorSubmenu->Enable(ID_CURSOR_OFF, !isRunning);
    cursorSubmenu->AppendRadioItem(ID_CURSOR_TOUCHPAD_MODE, "Touchpad Mode");
    cursorSubmenu->Check(ID_CURSOR_TOUCHPAD_MODE, cursorMode == 1);
    cursorSubmenu->Enable(ID_CURSOR_TOUCHPAD_MODE, !isRunning);
    cursorSubmenu->AppendRadioItem(ID_CURSOR_FUSION_MODE, "Fusion Mode");
    cursorSubmenu->Check(ID_CURSOR_FUSION_MODE, cursorMode == 2);
    cursorSubmenu->Enable(ID_CURSOR_FUSION_MODE, !isRunning);

    Bind(
        wxEVT_COMMAND_MENU_SELECTED,
        [=, this](wxCommandEvent &e) { cursorMode = 0; }, ID_CURSOR_OFF);
    Bind(
        wxEVT_COMMAND_MENU_SELECTED, [=](wxCommandEvent &e) { cursorMode = 1; },
        ID_CURSOR_TOUCHPAD_MODE);

    Bind(
        wxEVT_COMMAND_MENU_SELECTED, [=](wxCommandEvent &e) { cursorMode = 2; },
        ID_CURSOR_FUSION_MODE);

    buttonSubmenu->AppendRadioItem(ID_BUTTON_OFF, "OFF");
    buttonSubmenu->Check(ID_BUTTON_OFF, buttonMode == 0);
    buttonSubmenu->Enable(ID_BUTTON_OFF, !isRunning);

    buttonSubmenu->AppendRadioItem(ID_BUTTON_SIMPLE_MODE, "Simple Mode");
    buttonSubmenu->Check(ID_BUTTON_SIMPLE_MODE, buttonMode == 1);
    buttonSubmenu->Enable(ID_BUTTON_SIMPLE_MODE, !isRunning);

    buttonSubmenu->AppendRadioItem(ID_BUTTON_DPAD_MODE, "D-Pad Mode");
    buttonSubmenu->Check(ID_BUTTON_DPAD_MODE, buttonMode == 2);
    buttonSubmenu->Enable(ID_BUTTON_DPAD_MODE, !isRunning);

    Bind(
        wxEVT_COMMAND_MENU_SELECTED,
        [=, this](wxCommandEvent &e) { buttonMode = 0; }, ID_BUTTON_OFF);

    Bind(
        wxEVT_COMMAND_MENU_SELECTED,
        [=, this](wxCommandEvent &e) { buttonMode = 1; },
        ID_BUTTON_SIMPLE_MODE);

    confgSubmenu->Append(ID_OPENCONFIG, "Open config.ini");
    Bind(
        wxEVT_MENU, [=, this](wxCommandEvent &e) { buttonMode = 2; },
        ID_BUTTON_DPAD_MODE);

    Bind(
        wxEVT_COMMAND_MENU_SELECTED,
        [=](wxCommandEvent &) {
          ShellExecute(0, 0, configFilePath, 0, 0, SW_SHOW);
        },
        ID_OPENCONFIG);

    confgSubmenu->AppendSubMenu(cursorSubmenu, "Cursor Mode");
    confgSubmenu->AppendSubMenu(buttonSubmenu, "Button Mode");

    mainMenu->Append(ID_START, "Start");
    mainMenu->Enable(ID_START, !isRunning);
    mainMenu->Append(ID_STOP, "Stop");
    mainMenu->Enable(ID_STOP, isRunning);
    mainMenu->AppendSeparator();
    mainMenu->AppendSubMenu(confgSubmenu, "Config");
    mainMenu->Append(ID_ABOUT, "About");
    mainMenu->Append(wxID_EXIT, "Exit");

    Bind(
        wxEVT_MENU,
        [=](wxCommandEvent &) {
          if (this->controllerWorkerRef == nullptr) {
            ControllerSettings loadedFromIni = GearVRController::loadFromIni();
            if (loadedFromIni.macAddress == 1) {
              wxMessageBox(
                  wxString::Format("No config.ini was found. Fresh one created "
                                   "at root directory, please fill in the MAC "
                                   "address and run start again."));
            } else if (loadedFromIni.macAddress == 0) {
              wxMessageBox(
                  wxString::Format("Invalid or non-existant config.ini. "
                                   "Creation of new one failed."));
            } else {
              this->isRunning = 1;
              this->controllerWorkerRef =
                  new std::jthread([this, loadedFromIni](std::stop_token stop) {
                    if (this->controllerObjectRef == nullptr) {
                      this->controllerObjectRef =
                          new GearVRController(loadedFromIni);
                    } else {
                      this->controllerObjectRef->initializeSettings(
                          loadedFromIni);
                    }
                    this->controllerObjectRef->startOperation(
                        std::bitset<4>((this->cursorMode == 2) |
                                       ((this->cursorMode == 1) << 1) |
                                       ((this->buttonMode > 0) << 2) |
                                       ((this->buttonMode == 2) << 3)));
                    while (!stop.stop_requested()) {
                    };
                    this->controllerObjectRef->pauseOperation();
                    return;
                  });
            }
          }
        },
        ID_START);
    Bind(
        wxEVT_MENU,
        [=](wxCommandEvent &) {
          this->controllerWorkerRef->request_stop();
          this->controllerWorkerRef->join();
          this->controllerWorkerRef = nullptr;
          this->isRunning = 0;
        },
        ID_STOP);
    Bind(
        wxEVT_MENU, [=](wxCommandEvent &) { wxGetApp().ExitMainLoop(); },
        wxID_EXIT);
    Bind(
        wxEVT_MENU,
        [=](wxCommandEvent &) {
          wxMessageBox(
              wxString::Format(
                  "Made by Basel W. Ibrahim under the MiT license.\nTip: "
                  "pressing trigger + touchpad while using the motion cursor "
                  "will re-center the on-screen cursor.\nCredits to \n-@jsyang "
                  "for their comprehensive reverse engineering documentation. "
                  "\n-@xioTechnologies for the Madgwick fusion algorithm "
                  "implementation.\n-@metayeti for the INI file parsing "
                  "library "),
              wxString::Format("About"));
        },
        ID_ABOUT);

    return mainMenu;
  }
};

bool TrayApp::OnInit() {
  if (!wxApp::OnInit())
    return false;

  this->controllerObject = nullptr;
  this->controllerWorker = nullptr;
  TrayIcon *trayIcon =
      new TrayIcon(this->controllerObject, this->controllerWorker);

  wxIcon icon("./resources/icon.ico", wxBITMAP_TYPE_ICO);
  if (icon.IsOk()) {
    trayIcon->SetIcon(icon, "GearVR Controller");
  }

  return true;
}

int TrayApp::OnExit() {
  if (this->controllerWorker != nullptr) {
    this->controllerWorker->request_stop();
    this->controllerWorker->join();
    delete this->controllerWorker;
    this->controllerWorker = nullptr;
  }
  if (this->controllerObject != nullptr) {
    delete this->controllerObject;
    this->controllerObject = nullptr;
  }
  return 1;
}

wxIMPLEMENT_APP(TrayApp);