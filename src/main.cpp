// wxWidgets "Hello World" Program

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/taskbar.h>
#include <wx/wxprec.h>
#include <wx/app.h>

enum  {
  // Main menu actions
  ID_START = 0,
  ID_STOP = 2,
  ID_EXIT = 1,
  ID_OPENCONFIG = 3,

  // Cursor submenu actions
  ID_CURSOR_OFF = 4,
  ID_CURSOR_TOUCHPAD_MODE = 5,
  ID_CURSOR_FUSION_MODE = 6,

  // Button submenu actions
  ID_BUTTON_OFF = 7,
  ID_BUTTON_SIMPLE_MODE = 8,
  ID_BUTTON_DPAD_MODE = 9,
};


class TrayApp : public wxApp {
public:
  virtual bool OnInit() override;
  virtual int onExit();
};

DECLARE_APP(TrayApp);

class TrayIcon : public wxTaskBarIcon {
public:
  TrayIcon(){
    std::wstring r = L"config.ini";
    wchar_t a[MAX_PATH];
    GetFullPathNameW(r.c_str(), MAX_PATH, a, NULL);
    this->configFilePath = std::wstring(a);
  };
  std::wstring configFilePath;
  int cursorMode = 0;
  int buttonMode = 0;
protected:
  wxMenu *CreatePopupMenu() override {
    wxMenu *mainMenu = new wxMenu();
    wxMenu* confgSubmenu = new wxMenu();
    wxMenu *cursorSubmenu = new wxMenu();
    wxMenu *buttonSubmenu = new wxMenu();

    auto cursorOffItem = cursorSubmenu->AppendRadioItem(ID_CURSOR_OFF, "OFF");
    cursorOffItem->Check(cursorMode == 0);
    auto cursorTouchItem = cursorSubmenu->AppendRadioItem(
        ID_CURSOR_TOUCHPAD_MODE, "Touchpad Mode");
    cursorTouchItem->Check(cursorMode == 1);
    auto cursorFusionItem =
        cursorSubmenu->AppendRadioItem(ID_CURSOR_FUSION_MODE, "Fusion Mode");
    cursorFusionItem->Check(cursorMode == 2);

    Bind(
        wxEVT_COMMAND_MENU_SELECTED,
        [=, this](wxCommandEvent &e) {
          cursorMode = 0;
        },
        ID_CURSOR_OFF);
    Bind(
        wxEVT_COMMAND_MENU_SELECTED,
        [=](wxCommandEvent &e) {
          cursorMode = 1;
        },
        ID_CURSOR_TOUCHPAD_MODE);

    Bind(
        wxEVT_COMMAND_MENU_SELECTED,
        [=](wxCommandEvent &e) {
          cursorMode = 2;
        },
        ID_CURSOR_FUSION_MODE);


    auto buttonOffItem = buttonSubmenu->AppendRadioItem(ID_BUTTON_OFF, "OFF");
    buttonOffItem->Check(buttonMode == 0);

    auto buttonSimpleItem =
        buttonSubmenu->AppendRadioItem(ID_BUTTON_SIMPLE_MODE, "Simple Mode");
    buttonSimpleItem->Check(buttonMode == 1);

    auto buttonDpadItem =
        buttonSubmenu->AppendRadioItem(ID_BUTTON_DPAD_MODE, "D-Pad Mode");
    buttonDpadItem->Check(buttonMode == 2);

    Bind(
        wxEVT_COMMAND_MENU_SELECTED,
        [=, this](wxCommandEvent &e) { buttonMode = 0; }, ID_BUTTON_OFF);

    Bind(
        wxEVT_COMMAND_MENU_SELECTED,
        [=, this](wxCommandEvent &e) { buttonMode = 1; },
        ID_BUTTON_SIMPLE_MODE);

    confgSubmenu->Append(ID_OPENCONFIG, "Open config.ini");
    Bind(
        wxEVT_MENU,
        [=, this](wxCommandEvent &e) { buttonMode = 2; }, ID_BUTTON_DPAD_MODE);

        Bind(
        wxEVT_COMMAND_MENU_SELECTED,
        [=](wxCommandEvent &) {
          ShellExecute(0, 0, configFilePath.c_str(), 0, 0, SW_SHOW);
        },
        ID_OPENCONFIG);

    confgSubmenu->AppendSubMenu(cursorSubmenu, "Cursor Mode");
    confgSubmenu->AppendSubMenu(buttonSubmenu, "Button Mode");


    mainMenu->Append(ID_START, "Start");
    mainMenu->Append(ID_STOP, "Stop");
    mainMenu->AppendSubMenu(confgSubmenu, "Config");
    mainMenu->Append(wxID_EXIT, "Exit");

    Bind(
        wxEVT_MENU, [=](wxCommandEvent &) { wxGetApp().Exit(); },
        wxID_EXIT);
    return mainMenu;
  }
};

bool TrayApp::OnInit() {
  if (!wxApp::OnInit())
    return false;

  // Create the tray icon
  TrayIcon *trayIcon = new TrayIcon();

  // Load an icon (replace with your own .xpm or .ico file)
  wxIcon icon("./resources/test.ico", wxBITMAP_TYPE_ICO);
  if (icon.IsOk()) {
    trayIcon->SetIcon(icon, "My Tray Application");
  }

  return true;
}

int TrayApp::onExit() { return 1; }

wxIMPLEMENT_APP(TrayApp);