/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
 * $Revision: 12699 $
 * $Id: environmentsettingsdlg.cpp 12699 2022-02-03 19:20:37Z wh11204 $
 * $HeadURL: svn://svn.code.sf.net/p/codeblocks/code/trunk/src/src/environmentsettingsdlg.cpp $
 */

#include <sdk.h>

#ifndef CB_PRECOMP
    #include <wx/button.h>
    #include <wx/menu.h>
    #include <wx/radiobut.h>
    #include <wx/xrc/xmlres.h>
    #include <wx/intl.h>
    #include <wx/listctrl.h>
    #include <wx/combobox.h>
    #include <wx/choice.h>
    #include <wx/checkbox.h>
    #include <wx/checklst.h>
    #include <wx/radiobox.h>
    #include <wx/spinctrl.h>
    #include <wx/msgdlg.h>
    #include <wx/imaglist.h>
    #include <wx/settings.h>
    #include <wx/statbmp.h>
    #include <wx/stattext.h>
    #include <wx/dcmemory.h>

    #include <manager.h>
    #include <configmanager.h>
    #include <editormanager.h>
    #include <pluginmanager.h>
    #include <logmanager.h>
    #include "appglobals.h"
    #include "globals.h"
    #include "associations.h"
    #include "cbauibook.h"
#endif

#include <wx/aui/aui.h>
#include <wx/clrpicker.h>
#include <wx/listbook.h>

#include "annoyingdialog.h"
#include "appglobals.h"
#include "configurationpanel.h"
#include "environmentsettingsdlg.h"
#include "cbcolourmanager.h"

#ifdef __WXMSW__
    #include "associations.h"
#endif

#ifndef CB_PRECOMP
    #include <wx/dir.h>
    #include "cbplugin.h" // cgCompiler...
#endif

// images by order of pages
const wxString base_imgs[] =
{
    _T("general-prefs"),
    _T("view"),
    _T("notebook-appearance"),
    _T("colours"),
    _T("colours"),
    _T("dialogs"),
    _T("net")
};
const int IMAGES_COUNT = sizeof(base_imgs) / sizeof(wxString);

const int iconSizes[] = { 16, 24, 32, 64 };

BEGIN_EVENT_TABLE(EnvironmentSettingsDlg, wxScrollingDialog)
    EVT_BUTTON(XRCID("btnSetAssocs"), EnvironmentSettingsDlg::OnSetAssocs)
    EVT_BUTTON(XRCID("btnManageAssocs"), EnvironmentSettingsDlg::OnManageAssocs)
    EVT_BUTTON(XRCID("btnResetDefaultColours"), EnvironmentSettingsDlg::OnResetDefaultColours)
    EVT_CHECKBOX(XRCID("chkUseIPC"), EnvironmentSettingsDlg::OnUseIpcCheck)
    EVT_CHOICE(XRCID("chChildWindowPlace"), EnvironmentSettingsDlg::OnPlaceCheck)
    EVT_CHECKBOX(XRCID("chkPlaceHead"), EnvironmentSettingsDlg::OnHeadCheck)
    EVT_CHECKBOX(XRCID("chkAutoHideMessages"), EnvironmentSettingsDlg::OnAutoHide)
    EVT_CHECKBOX(XRCID("chkI18N"), EnvironmentSettingsDlg::OnI18NCheck)
    EVT_CHECKBOX(XRCID("chkDblClkMaximizes"), EnvironmentSettingsDlg::OnDblClickMaximizes)
    EVT_CHECKBOX(XRCID("chkNBUseMousewheel"), EnvironmentSettingsDlg::OnUseTabMousewheel)

    EVT_CHOICE(XRCID("chCategory"), EnvironmentSettingsDlg::OnChooseAppColourCategory)
    EVT_CHOICE(XRCID("chSettingsIconsSize"), EnvironmentSettingsDlg::OnSettingsIconsSize)
    EVT_LISTBOX(XRCID("lstColours"), EnvironmentSettingsDlg::OnChooseAppColourItem)
    EVT_COLOURPICKER_CHANGED(XRCID("colourPicker"), EnvironmentSettingsDlg::OnClickAppColour)
    EVT_BUTTON(XRCID("btnDefaultColour"), EnvironmentSettingsDlg::OnClickAppColourDefault)
    EVT_BUTTON(XRCID("btnResetAll"), EnvironmentSettingsDlg::OnClickAppResetAll)
END_EVENT_TABLE()

EnvironmentSettingsDlg::EnvironmentSettingsDlg(wxWindow* parent, wxAuiDockArt* art)
    : m_pArt(art),
    m_pImageList(nullptr)
{
    ConfigManager *cfg = Manager::Get()->GetConfigManager(_T("app"));
    ConfigManager *pcfg = Manager::Get()->GetConfigManager(_T("project_manager"));
    ConfigManager *mcfg = Manager::Get()->GetConfigManager(_T("message_manager"));
    ConfigManager *acfg = Manager::Get()->GetConfigManager(_T("an_dlg"));

    wxXmlResource::Get()->LoadObject(this, parent, _T("dlgEnvironmentSettings"),_T("wxScrollingDialog"));
    XRCCTRL(*this, "wxID_OK", wxButton)->SetDefault();

    LoadListbookImages();

    Connect(XRCID("nbMain"),wxEVT_COMMAND_LISTBOOK_PAGE_CHANGING,wxListbookEventHandler(EnvironmentSettingsDlg::OnPageChanging));
    Connect(XRCID("nbMain"),wxEVT_COMMAND_LISTBOOK_PAGE_CHANGED, wxListbookEventHandler(EnvironmentSettingsDlg::OnPageChanged ));

    // tab "General"
    XRCCTRL(*this, "chkShowSplash", wxCheckBox)->SetValue(cfg->ReadBool(_T("/environment/show_splash"), true));
    XRCCTRL(*this, "chkSingleInstance", wxCheckBox)->SetValue(cfg->ReadBool(_T("/environment/single_instance"), true));
#ifdef __WXMSW__
    static_cast<wxStaticBoxSizer*>(XRCCTRL(*this, "chkUseIPC", wxCheckBox)->GetContainingSizer())->GetStaticBox()->SetLabel(_("Dynamic Data Exchange (will take place after restart)"));
#endif
    bool useIpc = cfg->ReadBool(_T("/environment/use_ipc"), true);
    XRCCTRL(*this, "chkUseIPC",      wxCheckBox)->SetValue(useIpc);
    XRCCTRL(*this, "chkRaiseViaIPC", wxCheckBox)->SetValue(cfg->ReadBool(_T("/environment/raise_via_ipc"), true));
    XRCCTRL(*this, "chkRaiseViaIPC", wxCheckBox)->Enable(useIpc);

    XRCCTRL(*this, "chkAssociations",       wxCheckBox)->SetValue(cfg->ReadBool(_T("/environment/check_associations"),     true));
    XRCCTRL(*this, "chkModifiedFiles",      wxCheckBox)->SetValue(cfg->ReadBool(_T("/environment/check_modified_files"),   true));
    XRCCTRL(*this, "chkInvalidTargets",     wxCheckBox)->SetValue(cfg->ReadBool(_T("/environment/ignore_invalid_targets"), true));
    XRCCTRL(*this, "chkRobustSave",         wxCheckBox)->SetValue(cfg->ReadBool(_T("/environment/robust_save"), true));
    XRCCTRL(*this, "rbAppStart", wxRadioBox)->SetSelection(cfg->ReadBool(_T("/environment/blank_workspace"), true) ? 1 : 0);
    XRCCTRL(*this, "chkProjectLayout",      wxCheckBox)->SetValue(cfg->ReadBool(_T("/environment/enable_project_layout"),  true));
    XRCCTRL(*this, "chkEditorLayout",       wxCheckBox)->SetValue(cfg->ReadBool(_T("/environment/enable_editor_layout"),   false));

    wxTextCtrl* txt = XRCCTRL(*this, "txtConsoleShell", wxTextCtrl);
    txt->SetValue(cfg->Read(_T("/console_shell"), DEFAULT_CONSOLE_SHELL));
#ifdef __WXMSW__
    // under win32, this option is not needed, so disable it
    txt->Enable(false);
#endif

    wxComboBox *combo = XRCCTRL(*this, "cbConsoleTerm", wxComboBox);
    combo->Append(DEFAULT_CONSOLE_TERM);

    if (platform::windows)
        combo->Enable(false);
    else
    {
        if (!platform::macosx && !platform::darwin)
        {
            combo->Append(wxT("gnome-terminal --wait -t $TITLE -x "));
            combo->Append(wxT("konsole -e "));
            combo->Append(wxT("xfce4-terminal --disable-server -T $TITLE -x "));
            combo->Append(wxT("terminology -M -T $TITLE -e "));
        }
        wxString terminal = cfg->Read(wxT("/console_terminal"), DEFAULT_CONSOLE_TERM);
        if (!combo->SetStringSelection(terminal))
        {
            combo->Insert(terminal, 0);
            combo->SetStringSelection(terminal);
        }
    }

    const wxString &openFolderCommand = cfg->Read(_T("/open_containing_folder"), cbDEFAULT_OPEN_FOLDER_CMD);
    XRCCTRL(*this, "txtOpenFolder", wxTextCtrl)->SetValue(openFolderCommand);

    // tab "View"
    const cbChildWindowPlacement childWindowPlacement = cbGetChildWindowPlacement(*cfg);
    XRCCTRL(*this, "chChildWindowPlace", wxChoice)->SetSelection(int(childWindowPlacement));
    XRCCTRL(*this, "chkPlaceHead", wxCheckBox)->SetValue(cfg->ReadInt(_T("/dialog_placement/dialog_position"), 0) == pdlHead ? 1 : 0);
    XRCCTRL(*this, "chkPlaceHead", wxCheckBox)->Enable(childWindowPlacement == cbChildWindowPlacement::CenterOnDisplay);

    XRCCTRL(*this, "rbProjectOpen", wxRadioBox)->SetSelection(pcfg->ReadInt(_T("/open_files"), 1));

    {
        const int size = cbHelpers::ReadToolbarSizeFromConfig();

        int selection = -1;
        for (int ii = 0; ii < cbCountOf(iconSizes); ++ii)
        {
            if (size == iconSizes[ii])
            {
                selection = ii;
                break;
            }
        }

        wxChoice *control = XRCCTRL(*this, "chToolbarIconSize", wxChoice);
        const double actualScaleFactor = cbGetActualContentScaleFactor(*Manager::Get()->GetAppFrame());
        int scaledSize;

        scaledSize = cbFindMinSize16to64(iconSizes[0] * actualScaleFactor);
        control->Append(_("Normal") + wxString::Format(wxT(" (%dx%d)"), scaledSize, scaledSize));

        scaledSize = cbFindMinSize16to64(iconSizes[1] * actualScaleFactor);
        control->Append(_("Large") + wxString::Format(wxT(" (%dx%d)"), scaledSize, scaledSize));

        scaledSize = cbFindMinSize16to64(iconSizes[2] * actualScaleFactor);
        control->Append(_("Larger") + wxString::Format(wxT(" (%dx%d)"), scaledSize, scaledSize));

        scaledSize = cbFindMinSize16to64(iconSizes[3] * actualScaleFactor);
        control->Append(_("Largest") + wxString::Format(wxT(" (%dx%d)"), scaledSize, scaledSize));

        control->SetSelection(selection);
    }

    XRCCTRL(*this, "chSettingsIconsSize", wxChoice)->SetSelection(cfg->ReadInt(_T("/environment/settings_size"), 0));
    XRCCTRL(*this, "chkShowStartPage",    wxCheckBox)->SetValue(cfg->ReadBool(_T("/environment/start_here_page"), true));
    XRCCTRL(*this, "spnLogFontSize",      wxSpinCtrl)->SetValue(mcfg->ReadInt(_T("/log_font_size"), (platform::macosx ? 10 : 8)));


    bool en = mcfg->ReadBool(_T("/auto_hide"), false);
    XRCCTRL(*this, "chkAutoHideMessages",         wxCheckBox)->SetValue(en);
    XRCCTRL(*this, "chkAutoShowMessagesOnSearch", wxCheckBox)->SetValue(mcfg->ReadBool(_T("/auto_show_search"), true));
    XRCCTRL(*this, "chkAutoShowMessagesOnWarn",   wxCheckBox)->SetValue(mcfg->ReadBool(_T("/auto_show_build_warnings"), true));
    XRCCTRL(*this, "chkAutoShowMessagesOnErr",    wxCheckBox)->SetValue(mcfg->ReadBool(_T("/auto_show_build_errors"), true));
    XRCCTRL(*this, "chkAutoShowMessagesOnSearch", wxCheckBox)->Enable(en);
    XRCCTRL(*this, "chkAutoShowMessagesOnWarn",   wxCheckBox)->Enable(en);
    XRCCTRL(*this, "chkAutoShowMessagesOnErr",    wxCheckBox)->Enable(en);

    XRCCTRL(*this, "chkAutoFocusMessagesOnErr",    wxCheckBox)->SetValue(mcfg->ReadBool(_T("/auto_focus_build_errors"), true));
    XRCCTRL(*this, "chkSaveSelectionChangeInMP", wxCheckBox)->SetValue(mcfg->ReadBool(_T("/save_selection_change_in_mp"), true));

    en = cfg->ReadBool(_T("/environment/view/dbl_clk_maximize"), true);
     XRCCTRL(*this, "chkDblClkMaximizes", wxCheckBox)->SetValue(en);
    int idx = Manager::Get()->GetAppFrame()->GetMenuBar()->FindMenu(_("&View"));
    if (idx != wxNOT_FOUND)
    {
        wxMenu* menuView = Manager::Get()->GetAppFrame()->GetMenuBar()->GetMenu(idx);
        int sub_idx = menuView->FindItem(_("Perspectives"));
        if (sub_idx != wxNOT_FOUND)
        {
            wxMenu* menuLayouts = menuView->FindItem(sub_idx)->GetSubMenu();
            if (menuLayouts)
            {
                wxMenuItemList& items = menuLayouts->GetMenuItems();
                for (size_t i = 0; i < items.GetCount() && ! items[i]->IsSeparator() ; ++i)
                {
                    XRCCTRL(*this, "choLayoutToToggle", wxChoice)->Append(items[i]->GetLabelText(items[i]->GetItemLabelText()));
                }
            }
        }
    }

    int sel = XRCCTRL(*this, "choLayoutToToggle", wxChoice)->FindString( cfg->Read(_T("/environment/view/layout_to_toggle"),cfg->Read(_T("/main_frame/layout/default"))));
    XRCCTRL(*this, "choLayoutToToggle", wxChoice)->SetSelection(sel != wxNOT_FOUND ? sel : 0);
    XRCCTRL(*this, "choLayoutToToggle", wxChoice)->Enable(en);

    bool i18n = cfg->ReadBool(_T("/locale/enable"), false);
    XRCCTRL(*this, "chkI18N", wxCheckBox)->SetValue(i18n);

    wxString locPath = ConfigManager::GetDataFolder() + _T("/locale");
    if ( wxDirExists(locPath) )
    {
        wxString locFName;
        wxDir    locDir(locPath);
        if ( locDir.IsOpened() && locDir.GetFirst(&locFName/*, wxEmptyString, wxDIR_DIRS*/) )
        {
            do
            {
                const wxLanguageInfo* info = wxLocale::FindLanguageInfo(locFName);
                if (info)
                    XRCCTRL(*this, "choLanguage", wxChoice)->Append(info->Description);
            } while ( locDir.GetNext(&locFName) );
        }
    }

    XRCCTRL(*this, "choLanguage", wxChoice)->Enable(i18n);

    const wxLanguageInfo* info = wxLocale::FindLanguageInfo(cfg->Read(_T("/locale/language")));
    if (info)
    {
        const int position = XRCCTRL(*this, "choLanguage", wxChoice)->FindString(info->Description);
        XRCCTRL(*this, "choLanguage", wxChoice)->SetSelection(position);
    }


    // tab "Notebook"
    XRCCTRL(*this, "cmbEditorTabs",               wxChoice)->SetSelection(cfg->ReadInt(_T("/environment/tabs_style"), 0));
    XRCCTRL(*this, "cmbTabCloseStyle",            wxChoice)->SetSelection(cfg->ReadInt(_T("/environment/tabs_closestyle"), 0));
    XRCCTRL(*this, "chkListTabs",                 wxCheckBox)->SetValue(cfg->ReadBool(_T("/environment/tabs_list"), 0));
    XRCCTRL(*this, "chkStackedBasedTabSwitching", wxCheckBox)->SetValue(cfg->ReadBool(_T("/environment/tabs_stacked_based_switching"), 0));
    bool enableTabMousewheel = cfg->ReadBool(_T("/environment/tabs_use_mousewheel"),true);
    bool modToAdvance = cfg->ReadBool(_T("/environment/tabs_mousewheel_advance"),false);
    XRCCTRL(*this, "chkNBUseMousewheel",          wxCheckBox)->SetValue(enableTabMousewheel);
    XRCCTRL(*this, "rbNBModToAdvance",            wxRadioButton)->SetValue(modToAdvance);
    XRCCTRL(*this, "rbNBModToMove",               wxRadioButton)->SetValue(!modToAdvance);
    XRCCTRL(*this, "chkNBInvertAdvance",          wxCheckBox)->SetValue(cfg->ReadBool(_T("/environment/tabs_invert_advance"),false));
    XRCCTRL(*this, "chkNBInvertMove",             wxCheckBox)->SetValue(cfg->ReadBool(_T("/environment/tabs_invert_move"),false));
    XRCCTRL(*this, "txtMousewheelModifier",       wxTextCtrl)->SetValue(cfg->Read(_T("/environment/tabs_mousewheel_modifier"),_T("Ctrl")));
    XRCCTRL(*this, "txtMousewheelModifier",       wxTextCtrl)->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(EnvironmentSettingsDlg::OnMousewheelModifier));
    XRCCTRL(*this, "rbNBModToAdvance",            wxRadioButton)->Enable(enableTabMousewheel);
    XRCCTRL(*this, "rbNBModToMove",               wxRadioButton)->Enable(enableTabMousewheel);
    XRCCTRL(*this, "chkNBInvertAdvance",          wxCheckBox)->Enable(enableTabMousewheel);
    XRCCTRL(*this, "chkNBInvertMove",             wxCheckBox)->Enable(enableTabMousewheel);
    XRCCTRL(*this, "txtMousewheelModifier",       wxTextCtrl)->Enable(enableTabMousewheel);

    // tab "Docking"
    XRCCTRL(*this, "spnAuiBorder",                        wxSpinCtrl)->SetValue(cfg->ReadInt(_T("/environment/aui/border_size"), m_pArt->GetMetric(wxAUI_DOCKART_PANE_BORDER_SIZE)));
    XRCCTRL(*this, "spnAuiSash",                          wxSpinCtrl)->SetValue(cfg->ReadInt(_T("/environment/aui/sash_size"), m_pArt->GetMetric(wxAUI_DOCKART_SASH_SIZE)));
    XRCCTRL(*this, "spnAuiCaption",                       wxSpinCtrl)->SetValue(cfg->ReadInt(_T("/environment/aui/caption_size"), m_pArt->GetMetric(wxAUI_DOCKART_CAPTION_SIZE)));
    XRCCTRL(*this, "spnAuiHeaderFontSize",                wxSpinCtrl)->SetValue(cfg->ReadInt(_T("/environment/aui/header_font_size"), m_pArt->GetFont(wxAUI_DOCKART_CAPTION_FONT).GetPointSize()));
    XRCCTRL(*this, "cpAuiActiveCaptionColour",            wxColourPickerCtrl)->SetColour(cfg->ReadColour(_T("/environment/aui/active_caption_colour"), m_pArt->GetColour(wxAUI_DOCKART_ACTIVE_CAPTION_COLOUR)));
    XRCCTRL(*this, "cpAuiActiveCaptionGradientColour",    wxColourPickerCtrl)->SetColour(cfg->ReadColour(_T("/environment/aui/active_caption_gradient_colour"), m_pArt->GetColour(wxAUI_DOCKART_ACTIVE_CAPTION_GRADIENT_COLOUR)));
    XRCCTRL(*this, "cpAuiActiveCaptionTextColour",        wxColourPickerCtrl)->SetColour(cfg->ReadColour(_T("/environment/aui/active_caption_text_colour"), m_pArt->GetColour(wxAUI_DOCKART_ACTIVE_CAPTION_TEXT_COLOUR)));
    XRCCTRL(*this, "cpAuiInactiveCaptionColour",          wxColourPickerCtrl)->SetColour(cfg->ReadColour(_T("/environment/aui/inactive_caption_colour"), m_pArt->GetColour(wxAUI_DOCKART_INACTIVE_CAPTION_COLOUR)));
    XRCCTRL(*this, "cpAuiInactiveCaptionGradientColour",  wxColourPickerCtrl)->SetColour(cfg->ReadColour(_T("/environment/aui/inactive_caption_gradient_colour"), m_pArt->GetColour(wxAUI_DOCKART_INACTIVE_CAPTION_GRADIENT_COLOUR)));
    XRCCTRL(*this, "cpAuiInactiveCaptionTextColour",      wxColourPickerCtrl)->SetColour(cfg->ReadColour(_T("/environment/aui/inactive_caption_text_colour"), m_pArt->GetColour(wxAUI_DOCKART_INACTIVE_CAPTION_TEXT_COLOUR)));

    // tab "Dialogs"
    wxCheckListBox* clb = XRCCTRL(*this, "chkDialogs", wxCheckListBox);
    clb->Clear();

    m_AnnoyingDlgReturnMap[F(wxT("%d"), AnnoyingDialog::rtOK)]     = _("OK");
    m_AnnoyingDlgReturnMap[F(wxT("%d"), AnnoyingDialog::rtCANCEL)] = _("Cancel");
    m_AnnoyingDlgReturnMap[F(wxT("%d"), AnnoyingDialog::rtYES)]    = _("Yes");
    m_AnnoyingDlgReturnMap[F(wxT("%d"), AnnoyingDialog::rtNO)]     = _("No");
    ConfigManagerContainer::StringSet dialogs;
    if (acfg->Exists(wxT("/disabled_ret")))
    {
        // new config style
        dialogs = acfg->ReadSSet(wxT("/disabled_ret"));
    }
    else
    {
        // if the new config key does not exist, read from the old one
        dialogs = acfg->ReadSSet(wxT("/disabled"));
        // and copy it to the new one
        acfg->Write(wxT("/disabled_ret"), dialogs);
        // we do not do an in place upgrade of the format to maintain
        // compatibility with previous versions
    }
    for (ConfigManagerContainer::StringSet::iterator i = dialogs.begin(); i != dialogs.end(); ++i)
        clb->Append(AnnoyingDlgReturnToString(*i));

    // tab "Network"
    XRCCTRL(*this, "txtProxy", wxTextCtrl)->SetValue(cfg->Read(_T("/network_proxy")));

    FillApplicationColours();

    // disable some windows-only settings, in other platforms
#ifndef __WXMSW__
    XRCCTRL(*this, "chkAssociations", wxCheckBox)->Enable(false);
    XRCCTRL(*this, "btnSetAssocs", wxButton)->Enable(false);
    XRCCTRL(*this, "btnManageAssocs", wxButton)->Enable(false);
#endif

    // add all plugins configuration panels
    AddPluginPanels();

    // make sure everything is laid out properly
    GetSizer()->SetSizeHints(this);
    CentreOnParent();
    Layout();
}

EnvironmentSettingsDlg::~EnvironmentSettingsDlg()
{
    //dtor
    delete m_pImageList;
}

void EnvironmentSettingsDlg::AddPluginPanels()
{
    const wxString base = _T("images/settings/");
    // for plugins who do not supply icons, use common generic icons
    const wxString noimg = _T("images/settings/generic-plugin");

    wxListbook* lb = XRCCTRL(*this, "nbMain", wxListbook);
    // get all configuration panels which are *not* about compiler and editor.
    Manager::Get()->GetPluginManager()->GetConfigurationPanels(~(cgCompiler | cgEditor), lb,
                                                               m_PluginPanels, this);

    for (size_t i = 0; i < m_PluginPanels.GetCount(); ++i)
    {
        cbConfigurationPanel* panel = m_PluginPanels[i];
        panel->SetParentDialog(this);
        lb->AddPage(panel, panel->GetTitle());

        wxString onFile = ConfigManager::LocateDataFile(base + panel->GetBitmapBaseName() + _T(".png"), sdDataGlobal | sdDataUser);
        if (onFile.IsEmpty())
            onFile = ConfigManager::LocateDataFile(noimg + _T(".png"), sdDataGlobal | sdDataUser);
        wxString offFile = ConfigManager::LocateDataFile(base + panel->GetBitmapBaseName() + _T("-off.png"), sdDataGlobal | sdDataUser);
        if (offFile.IsEmpty())
            offFile = ConfigManager::LocateDataFile(noimg + _T("-off.png"), sdDataGlobal | sdDataUser);

        m_pImageList->Add(cbLoadBitmap(onFile));
        m_pImageList->Add(cbLoadBitmap(offFile));
        lb->SetPageImage(lb->GetPageCount() - 1, m_pImageList->GetImageCount() - 2);
    }
    UpdateListbookImages();
}

void EnvironmentSettingsDlg::LoadListbookImages()
{
    const wxString base = ConfigManager::GetDataFolder() + _T("/images/settings/");

    m_pImageList = new wxImageList(80, 80);
    wxBitmap bmp;
    for (int i = 0; i < IMAGES_COUNT; ++i)
    {
        bmp = cbLoadBitmap(base + base_imgs[i] + _T(".png"));
        m_pImageList->Add(bmp);
        bmp = cbLoadBitmap(base + base_imgs[i] + _T("-off.png"));
        m_pImageList->Add(bmp);
    }
}

void EnvironmentSettingsDlg::UpdateListbookImages()
{
    wxListbook* lb = XRCCTRL(*this, "nbMain", wxListbook);
    int sel = lb->GetSelection();

    if (SettingsIconsStyle(XRCCTRL(*this, "chSettingsIconsSize", wxChoice)->GetSelection()) == sisNoIcons)
    {
        SetSettingsIconsStyle(lb->GetListView(), sisNoIcons);
        lb->SetImageList(nullptr);
    }
    else
    {
        lb->SetImageList(m_pImageList);
        // set page images according to their on/off status
        for (size_t i = 0; i < IMAGES_COUNT + m_PluginPanels.GetCount(); ++i)
            lb->SetPageImage(i, (i * 2) + (sel == (int)i ? 0 : 1));
        SetSettingsIconsStyle(lb->GetListView(), sisLargeIcons);
    }

    // update the page title
    wxString label = lb->GetPageText(sel);
    // replace any stray & with && because label makes it an underscore
    while (label.Replace(_T(" & "), _T(" && ")))
        ;
    XRCCTRL(*this, "lblBigTitle", wxStaticText)->SetLabel(label);
    XRCCTRL(*this, "pnlTitleInfo", wxPanel)->Layout();
}

void EnvironmentSettingsDlg::OnPageChanging(wxListbookEvent& event)
{
    const int selection = event.GetSelection();
    if (selection == wxNOT_FOUND)
        return;

    wxListbook* lb = XRCCTRL(*this, "nbMain", wxListbook);
    wxWindow *page = lb->GetPage(selection);
    if (page == nullptr)
        return;

    for (cbConfigurationPanel *panel : m_PluginPanels)
    {
        if (panel == page)
        {
            panel->OnPageChanging();
            break;
        }
    }
}

void EnvironmentSettingsDlg::OnPageChanged(wxListbookEvent& event)
{
    // update only on real change, not on dialog creation
    if (event.GetOldSelection() != -1 && event.GetSelection() != -1)
        UpdateListbookImages();
    Layout();
}

void EnvironmentSettingsDlg::OnSetAssocs(cb_unused wxCommandEvent& event)
{
#ifdef __WXMSW__
    Associations::SetCore();
    //cbMessageBox(_("Code::Blocks associated with C/C++ files."), _("Information"), wxICON_INFORMATION, this);
#endif
}

void EnvironmentSettingsDlg::OnManageAssocs(cb_unused wxCommandEvent& event)
{
#ifdef __WXMSW__
    ManageAssocsDialog dlg(this);
    PlaceWindow(&dlg);
    dlg.ShowModal();
#endif
}

void EnvironmentSettingsDlg::OnResetDefaultColours(cb_unused wxCommandEvent& event)
{
    wxAuiDockArt* art = new wxAuiDefaultDockArt;

    XRCCTRL(*this, "spnAuiBorder", wxSpinCtrl)->SetValue(art->GetMetric(wxAUI_DOCKART_PANE_BORDER_SIZE));
    XRCCTRL(*this, "spnAuiSash", wxSpinCtrl)->SetValue(art->GetMetric(wxAUI_DOCKART_SASH_SIZE));
    XRCCTRL(*this, "spnAuiCaption", wxSpinCtrl)->SetValue(art->GetMetric(wxAUI_DOCKART_CAPTION_SIZE));
    XRCCTRL(*this, "spnAuiHeaderFontSize", wxSpinCtrl)->SetValue(art->GetFont(wxAUI_DOCKART_CAPTION_FONT).GetPointSize());
    XRCCTRL(*this, "cpAuiActiveCaptionColour", wxColourPickerCtrl)->SetColour(art->GetColour(wxAUI_DOCKART_ACTIVE_CAPTION_COLOUR));
    XRCCTRL(*this, "cpAuiActiveCaptionGradientColour", wxColourPickerCtrl)->SetColour(art->GetColour(wxAUI_DOCKART_ACTIVE_CAPTION_GRADIENT_COLOUR));
    XRCCTRL(*this, "cpAuiActiveCaptionTextColour", wxColourPickerCtrl)->SetColour(art->GetColour(wxAUI_DOCKART_ACTIVE_CAPTION_TEXT_COLOUR));
    XRCCTRL(*this, "cpAuiInactiveCaptionColour", wxColourPickerCtrl)->SetColour(art->GetColour(wxAUI_DOCKART_INACTIVE_CAPTION_COLOUR));
    XRCCTRL(*this, "cpAuiInactiveCaptionGradientColour", wxColourPickerCtrl)->SetColour(art->GetColour(wxAUI_DOCKART_INACTIVE_CAPTION_GRADIENT_COLOUR));
    XRCCTRL(*this, "cpAuiInactiveCaptionTextColour", wxColourPickerCtrl)->SetColour(art->GetColour(wxAUI_DOCKART_INACTIVE_CAPTION_TEXT_COLOUR));

    delete art;
}

void EnvironmentSettingsDlg::OnAutoHide(cb_unused wxCommandEvent& event)
{
    bool en = XRCCTRL(*this, "chkAutoHideMessages", wxCheckBox)->GetValue();
    XRCCTRL(*this, "chkAutoShowMessagesOnSearch",   wxCheckBox)->Enable(en);
    XRCCTRL(*this, "chkAutoShowMessagesOnWarn",     wxCheckBox)->Enable(en);
    XRCCTRL(*this, "chkAutoShowMessagesOnErr",      wxCheckBox)->Enable(en);
}

void EnvironmentSettingsDlg::OnUseIpcCheck(wxCommandEvent& event)
{
    XRCCTRL(*this, "chkRaiseViaIPC", wxCheckBox)->Enable(event.IsChecked());
}

void EnvironmentSettingsDlg::OnDblClickMaximizes(cb_unused wxCommandEvent& event)
{
    XRCCTRL(*this, "choLayoutToToggle", wxChoice)->Enable(event.IsChecked());
}

void EnvironmentSettingsDlg::OnMousewheelModifier(cb_unused wxKeyEvent& event)
{
    wxString keys;

    if (wxGetKeyState(WXK_SHIFT))
        keys += keys.IsEmpty()?wxT("Shift"):wxT("+Shift");

    if (wxGetKeyState(WXK_CONTROL))
        keys += keys.IsEmpty()?wxT("Ctrl"):wxT("+Ctrl");

#if defined(__WXMAC__) || defined(__WXCOCOA__)
    if (wxGetKeyState(WXK_COMMAND))
        keys += keys.IsEmpty()?wxT("XCtrl"):wxT("+XCtrl");
#endif

    if (wxGetKeyState(WXK_ALT))
        keys += keys.IsEmpty()?wxT("Alt"):wxT("+Alt");

    if (!keys.IsEmpty())
        XRCCTRL(*this, "txtMousewheelModifier", wxTextCtrl)->SetValue(keys);
}

void EnvironmentSettingsDlg::OnUseTabMousewheel(cb_unused wxCommandEvent& event)
{
    bool en = (bool)XRCCTRL(*this, "chkNBUseMousewheel",wxCheckBox)->GetValue();
    XRCCTRL(*this, "rbNBModToAdvance", wxRadioButton)->Enable(en);
    XRCCTRL(*this, "rbNBModToMove", wxRadioButton)->Enable(en);
    XRCCTRL(*this, "chkNBInvertAdvance", wxCheckBox)->Enable(en);
    XRCCTRL(*this, "chkNBInvertMove", wxCheckBox)->Enable(en);
}

void EnvironmentSettingsDlg::OnPlaceCheck(cb_unused wxCommandEvent& event)
{
    wxChoice *place = XRCCTRL(*this, "chChildWindowPlace", wxChoice);
    const cbChildWindowPlacement placement = cbChildWindowPlacement(place->GetSelection());
    XRCCTRL(*this, "chkPlaceHead", wxCheckBox)->Enable(placement == cbChildWindowPlacement::CenterOnDisplay);
}

void EnvironmentSettingsDlg::OnHeadCheck(wxCommandEvent& event)
{
    PlaceWindow(this, event.IsChecked() ? pdlHead : pdlCentre, true);
}

void EnvironmentSettingsDlg::OnI18NCheck(wxCommandEvent& event)
{
    XRCCTRL(*this, "choLanguage", wxChoice)->Enable(event.IsChecked());
}

void EnvironmentSettingsDlg::OnSettingsIconsSize(cb_unused wxCommandEvent& event)
{
    UpdateListbookImages();
    Layout();
}

void EnvironmentSettingsDlg::EndModal(int retCode)
{
    if (retCode == wxID_OK)
    {
        ConfigManager *cfg = Manager::Get()->GetConfigManager(_T("app"));
        ConfigManager *pcfg = Manager::Get()->GetConfigManager(_T("project_manager"));
        ConfigManager *mcfg = Manager::Get()->GetConfigManager(_T("message_manager"));
        ConfigManager *acfg = Manager::Get()->GetConfigManager(_T("an_dlg"));

        // tab "General"
        cfg->Write(_T("/environment/show_splash"),                 (bool) XRCCTRL(*this, "chkShowSplash",         wxCheckBox)->GetValue());
        cfg->Write(_T("/environment/single_instance"),             (bool) XRCCTRL(*this, "chkSingleInstance",     wxCheckBox)->GetValue());
        cfg->Write(_T("/environment/use_ipc"),                     (bool) XRCCTRL(*this, "chkUseIPC",             wxCheckBox)->GetValue());
        cfg->Write(_T("/environment/raise_via_ipc"),               (bool) XRCCTRL(*this, "chkRaiseViaIPC",        wxCheckBox)->GetValue());
        cfg->Write(_T("/environment/check_associations"),          (bool) XRCCTRL(*this, "chkAssociations",       wxCheckBox)->GetValue());
        cfg->Write(_T("/environment/check_modified_files"),        (bool) XRCCTRL(*this, "chkModifiedFiles",      wxCheckBox)->GetValue());
        cfg->Write(_T("/environment/ignore_invalid_targets"),      (bool) XRCCTRL(*this, "chkInvalidTargets",     wxCheckBox)->GetValue());
        cfg->Write(_T("/environment/robust_save"),                 (bool) XRCCTRL(*this, "chkRobustSave",         wxCheckBox)->GetValue());
        cfg->Write(_T("/console_shell"),                                  XRCCTRL(*this, "txtConsoleShell",       wxTextCtrl)->GetValue());
        cfg->Write(_T("/console_terminal"),                               XRCCTRL(*this, "cbConsoleTerm",         wxComboBox)->GetValue());
        cfg->Write(_T("/open_containing_folder"), XRCCTRL(*this, "txtOpenFolder", wxTextCtrl)->GetValue());

        // tab "View"
        cfg->Write(_T("/environment/blank_workspace"),       (bool) XRCCTRL(*this, "rbAppStart", wxRadioBox)->GetSelection() ? true : false);
        cfg->Write(_T("/environment/enable_project_layout"), (bool) XRCCTRL(*this, "chkProjectLayout", wxCheckBox)->GetValue());
        cfg->Write(_T("/environment/enable_editor_layout"),  (bool) XRCCTRL(*this, "chkEditorLayout", wxCheckBox)->GetValue());
        pcfg->Write(_T("/open_files"),                       (int)  XRCCTRL(*this, "rbProjectOpen", wxRadioBox)->GetSelection());

        {
            const int selection = XRCCTRL(*this, "chToolbarIconSize", wxChoice)->GetSelection();
            int size = 16;
            if (selection >= 0 && selection < cbCountOf(iconSizes))
                size = iconSizes[selection];

            // We call unset to remove the old bool value if it is present.
            cfg->UnSet(_T("/environment/toolbar_size"));
            cfg->Write(_T("/environment/toolbar_size"), size);
        }

        cfg->Write(_T("/environment/settings_size"),         (int)  XRCCTRL(*this, "chSettingsIconsSize", wxChoice)->GetSelection());
        mcfg->Write(_T("/auto_hide"),                        (bool) XRCCTRL(*this, "chkAutoHideMessages", wxCheckBox)->GetValue());
        mcfg->Write(_T("/auto_show_search"),                 (bool) XRCCTRL(*this, "chkAutoShowMessagesOnSearch", wxCheckBox)->GetValue());
        mcfg->Write(_T("/auto_show_build_warnings"),         (bool) XRCCTRL(*this, "chkAutoShowMessagesOnWarn", wxCheckBox)->GetValue());
        mcfg->Write(_T("/auto_show_build_errors"),           (bool) XRCCTRL(*this, "chkAutoShowMessagesOnErr", wxCheckBox)->GetValue());
        mcfg->Write(_T("/auto_focus_build_errors"),           (bool) XRCCTRL(*this, "chkAutoFocusMessagesOnErr", wxCheckBox)->GetValue());
        mcfg->Write(_T("/save_selection_change_in_mp"),       (bool) XRCCTRL(*this, "chkSaveSelectionChangeInMP", wxCheckBox)->GetValue());

        cfg->Write(_T("/environment/start_here_page"),       (bool) XRCCTRL(*this, "chkShowStartPage", wxCheckBox)->GetValue());

        cfg->Write(_T("/environment/view/dbl_clk_maximize"),    (bool)XRCCTRL(*this, "chkDblClkMaximizes", wxCheckBox)->GetValue());
        cfg->Write(_T("/environment/view/layout_to_toggle"),    XRCCTRL(*this, "choLayoutToToggle", wxChoice)->GetStringSelection());

        cfg->Write(_T("/locale/enable"),                     (bool) XRCCTRL(*this, "chkI18N", wxCheckBox)->GetValue());

        wxChoice *chLanguage = XRCCTRL(*this, "choLanguage", wxChoice);
        const int langSelection = chLanguage->GetSelection();
        const wxLanguageInfo *info = nullptr;
        if (langSelection != wxNOT_FOUND)
            info = wxLocale::FindLanguageInfo(chLanguage->GetString(langSelection));
        if (info)
            cfg->Write(_T("/locale/language"), info->CanonicalName);
        else
            cfg->Write(_T("/locale/language"), wxEmptyString);

        mcfg->Write(_T("/log_font_size"), (int)  XRCCTRL(*this, "spnLogFontSize", wxSpinCtrl)->GetValue());


        {
            // Keep in sync with the code in cbGetChildWindowPlacement.
            int placement = XRCCTRL(*this, "chChildWindowPlace", wxChoice)->GetSelection();
            if (placement < 0 || placement >= 3)
                placement = 0;
            cfg->Write(wxT("/dialog_placement/child_placement"), placement);
        }

        cfg->Write(_T("/dialog_placement/dialog_position"),  (int)  XRCCTRL(*this, "chkPlaceHead",   wxCheckBox)->GetValue() ? pdlHead : pdlCentre);

        // tab "Appearence"
        cfg->Write(_T("/environment/tabs_style"),            (int)  XRCCTRL(*this, "cmbEditorTabs",               wxChoice)->GetSelection());
        cfg->Write(_T("/environment/tabs_closestyle"),       (int)  XRCCTRL(*this, "cmbTabCloseStyle",            wxChoice)->GetSelection());
        cfg->Write(_T("/environment/tabs_list"),             (bool) XRCCTRL(*this, "chkListTabs",                 wxCheckBox)->GetValue());
        bool tab_switcher_mode =                             (bool) XRCCTRL(*this, "chkStackedBasedTabSwitching", wxCheckBox)->GetValue();
        if (Manager::Get()->GetConfigManager(_T("app"))->ReadBool(_T("/environment/tabs_stacked_based_switching")) != tab_switcher_mode)
        {
            if (tab_switcher_mode)
                Manager::Get()->GetEditorManager()->RebuildNotebookStack();
            else
                Manager::Get()->GetEditorManager()->DeleteNotebookStack();
        }
        cfg->Write(_T("/environment/tabs_stacked_based_switching"),          tab_switcher_mode);

        bool enableMousewheel = (bool) XRCCTRL(*this, "chkNBUseMousewheel",wxCheckBox)->GetValue();
        cfg->Write(_T("/environment/tabs_use_mousewheel"),           enableMousewheel);
        wxString key = XRCCTRL(*this, "txtMousewheelModifier", wxTextCtrl)->GetValue();
        cfg->Write(_T("/environment/tabs_mousewheel_modifier"),      key.IsEmpty()?_T("Ctrl"):key);
        cfg->Write(_T("/environment/tabs_mousewheel_advance"),       (bool) XRCCTRL(*this, "rbNBModToAdvance", wxRadioButton)->GetValue());
        cfg->Write(_T("/environment/tabs_invert_advance"),           (bool) XRCCTRL(*this, "chkNBInvertAdvance", wxCheckBox)->GetValue());
        cfg->Write(_T("/environment/tabs_invert_move"),              (bool) XRCCTRL(*this, "chkNBInvertMove", wxCheckBox)->GetValue());
        cbAuiNotebook::AllowScrolling(enableMousewheel);

        cbAuiNotebook::SetModKeys(cfg->Read(_T("/environment/tabs_mousewheel_modifier"),_T("Ctrl")));
        cbAuiNotebook::UseModToAdvance(cfg->ReadBool(_T("/environment/tabs_mousewheel_advance"),false));
        cbAuiNotebook::InvertAdvanceDirection(cfg->ReadBool(_T("/environment/tabs_invert_advance"),false));
        cbAuiNotebook::InvertMoveDirection(cfg->ReadBool(_T("/environment/tabs_invert_move"),false));

        cfg->Write(_T("/environment/aui/border_size"),                (int)  XRCCTRL(*this, "spnAuiBorder", wxSpinCtrl)->GetValue());
        cfg->Write(_T("/environment/aui/sash_size"),                  (int)  XRCCTRL(*this, "spnAuiSash", wxSpinCtrl)->GetValue());
        cfg->Write(_T("/environment/aui/caption_size"),               (int)  XRCCTRL(*this, "spnAuiCaption", wxSpinCtrl)->GetValue());
        cfg->Write(_T("/environment/aui/header_font_size"),           (int)  XRCCTRL(*this, "spnAuiHeaderFontSize", wxSpinCtrl)->GetValue());
        cfg->Write(_T("/environment/aui/active_caption_colour"),             XRCCTRL(*this, "cpAuiActiveCaptionColour", wxColourPickerCtrl)->GetColour());
        cfg->Write(_T("/environment/aui/active_caption_gradient_colour"),    XRCCTRL(*this, "cpAuiActiveCaptionGradientColour", wxColourPickerCtrl)->GetColour());
        cfg->Write(_T("/environment/aui/active_caption_text_colour"),        XRCCTRL(*this, "cpAuiActiveCaptionTextColour", wxColourPickerCtrl)->GetColour());
        cfg->Write(_T("/environment/aui/inactive_caption_colour"),           XRCCTRL(*this, "cpAuiInactiveCaptionColour", wxColourPickerCtrl)->GetColour());
        cfg->Write(_T("/environment/aui/inactive_caption_gradient_colour"),  XRCCTRL(*this, "cpAuiInactiveCaptionGradientColour", wxColourPickerCtrl)->GetColour());
        cfg->Write(_T("/environment/aui/inactive_caption_text_colour"),      XRCCTRL(*this, "cpAuiInactiveCaptionTextColour", wxColourPickerCtrl)->GetColour());

        m_pArt->SetMetric(wxAUI_DOCKART_PANE_BORDER_SIZE,                XRCCTRL(*this, "spnAuiBorder", wxSpinCtrl)->GetValue());
        m_pArt->SetMetric(wxAUI_DOCKART_SASH_SIZE,                       XRCCTRL(*this, "spnAuiSash", wxSpinCtrl)->GetValue());
        m_pArt->SetMetric(wxAUI_DOCKART_CAPTION_SIZE,                    XRCCTRL(*this, "spnAuiCaption", wxSpinCtrl)->GetValue());
        wxFont font = m_pArt->GetFont(wxAUI_DOCKART_CAPTION_FONT);
        font.SetPointSize(XRCCTRL(*this, "spnAuiHeaderFontSize", wxSpinCtrl)->GetValue());
        m_pArt->SetFont(wxAUI_DOCKART_CAPTION_FONT, font);
        m_pArt->SetColour(wxAUI_DOCKART_ACTIVE_CAPTION_COLOUR,           XRCCTRL(*this, "cpAuiActiveCaptionColour", wxColourPickerCtrl)->GetColour());
        m_pArt->SetColour(wxAUI_DOCKART_ACTIVE_CAPTION_GRADIENT_COLOUR,  XRCCTRL(*this, "cpAuiActiveCaptionGradientColour", wxColourPickerCtrl)->GetColour());
        m_pArt->SetColour(wxAUI_DOCKART_ACTIVE_CAPTION_TEXT_COLOUR,      XRCCTRL(*this, "cpAuiActiveCaptionTextColour", wxColourPickerCtrl)->GetColour());
        m_pArt->SetColour(wxAUI_DOCKART_INACTIVE_CAPTION_COLOUR,         XRCCTRL(*this, "cpAuiInactiveCaptionColour", wxColourPickerCtrl)->GetColour());
        m_pArt->SetColour(wxAUI_DOCKART_INACTIVE_CAPTION_GRADIENT_COLOUR,XRCCTRL(*this, "cpAuiInactiveCaptionGradientColour", wxColourPickerCtrl)->GetColour());
        m_pArt->SetColour(wxAUI_DOCKART_INACTIVE_CAPTION_TEXT_COLOUR,    XRCCTRL(*this, "cpAuiInactiveCaptionTextColour", wxColourPickerCtrl)->GetColour());

        // tab "Dialogs"
        wxCheckListBox* lb = XRCCTRL(*this, "chkDialogs", wxCheckListBox);

        ConfigManagerContainer::StringSet dialogs = acfg->ReadSSet(_T("/disabled_ret"));

        for (size_t i = 0; i < lb->GetCount(); ++i)
        {
            if (lb->IsChecked(i))
                dialogs.erase(StringToAnnoyingDlgReturn(lb->GetString(i)));
        }

        acfg->Write(_T("/disabled_ret"), dialogs);

        // tab "Network"
        cfg->Write(_T("/network_proxy"),    XRCCTRL(*this, "txtProxy", wxTextCtrl)->GetValue());

        WriteApplicationColours();

        // finally, apply settings in all plugins' panels
        for (size_t i = 0; i < m_PluginPanels.GetCount(); ++i)
        {
            cbConfigurationPanel* panel = m_PluginPanels[i];
            panel->OnApply();
        }

        // save the colours manager here, just in case there are duplicate colour controls
        Manager::Get()->GetColourManager()->Save();
    }
    else
    {
        // finally, cancel settings in all plugins' panels
        for (size_t i = 0; i < m_PluginPanels.GetCount(); ++i)
        {
            cbConfigurationPanel* panel = m_PluginPanels[i];
            panel->OnCancel();
        }
    }

    wxScrollingDialog::EndModal(retCode);
}

namespace
{
struct AppColoursClientData : wxClientData
{
    AppColoursClientData(const wxString &id_) : id(id_) {}

    wxString id;
};

static void CreateAndSetBitmap(wxStaticBitmap &control, const wxColour &colour)
{
    const wxString label(_T("WWWWw"));

    int width, height;
    control.GetTextExtent(label, &width, &height);
    height = (height * 3) / 2;

    wxBitmap bmp(width, height);
    wxMemoryDC dc;
    dc.SelectObject(bmp);
    dc.SetFont(control.GetFont());
    dc.SetPen(*wxBLACK_PEN);
    dc.SetBrush(wxBrush(colour));
    dc.DrawRectangle(wxRect(0, 0, width, height));
    dc.SelectObject(wxNullBitmap);
    control.SetBitmap(bmp);
}

} // anonymous namespace

void EnvironmentSettingsDlg::FillApplicationColours()
{
    wxListBox *list = XRCCTRL(*this, "lstColours", wxListBox);
    wxChoice *categories = XRCCTRL(*this, "chCategory", wxChoice);

    bool fillCategories = (categories->GetCount() == 0);
    std::set<wxString> setCategories;

    wxString category = categories->GetStringSelection();
    if (categories->GetSelection() == 0)
        category = wxEmptyString;

    list->Clear();
    const ColourManager::ColourDefMap &colours = Manager::Get()->GetColourManager()->GetColourDefinitions();
    for (ColourManager::ColourDefMap::const_iterator it = colours.begin(); it != colours.end(); ++it)
    {
        if (!it->second.IsValid())
            continue;
        if (category.empty())
            list->Append(it->second.category + wxT(" : ") + it->second.name, new AppColoursClientData(it->first));
        else if (category == it->second.category)
            list->Append(it->second.name, new AppColoursClientData(it->first));
        if (fillCategories)
            setCategories.insert(it->second.category);
    }

    if (fillCategories)
    {
        categories->Append(_("All"));
        categories->Select(0);
        for (std::set<wxString>::const_iterator it = setCategories.begin(); it != setCategories.end(); ++it)
            categories->Append(*it);
    }

    if (list->GetCount() > 0)
    {
        list->SetSelection(0);
        DoChooseAppColourItem(0);
    }
    else
    {
        DoChooseAppColourItem(-1);
    }
}

void EnvironmentSettingsDlg::OnChooseAppColourCategory(cb_unused wxCommandEvent &event)
{
    FillApplicationColours();
}

void EnvironmentSettingsDlg::OnChooseAppColourItem(wxCommandEvent &event)
{
    DoChooseAppColourItem(event.GetSelection());
}

void EnvironmentSettingsDlg::DoChooseAppColourItem(int index)
{
    wxColourPickerCtrl *picker = XRCCTRL(*this, "colourPicker", wxColourPickerCtrl);
    wxButton *btnDefault = XRCCTRL(*this, "btnDefaultColour", wxButton);
    wxStaticBitmap *bmpDefaultColour = XRCCTRL(*this, "bmpDefaultColour", wxStaticBitmap);
    wxListBox *list = XRCCTRL(*this, "lstColours", wxListBox);

    const AppColoursClientData *data = (index < 0) ? nullptr : static_cast <AppColoursClientData *> (list->GetClientObject(index));
    if (!data)
    {
        picker->SetColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
        picker->Enable(false);
        btnDefault->Enable(false);
        CreateAndSetBitmap(*bmpDefaultColour, wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
        return;
    }

    const ColourManager::ColourDefMap &colours = Manager::Get()->GetColourManager()->GetColourDefinitions();
    const ColourManager::ColourDefMap::const_iterator it = colours.find(data->id);
    if (it != colours.end())
    {
        std::map<wxString, wxColour>::const_iterator colourIt = m_ChangedAppColours.find(data->id);
        wxColour activeColour;
        if (colourIt != m_ChangedAppColours.end())
            activeColour = colourIt->second;
        else
            activeColour = it->second.value;

        picker->SetColour(activeColour);
        picker->Enable(true);

        btnDefault->Enable(activeColour != it->second.defaultValue);
        CreateAndSetBitmap(*bmpDefaultColour, it->second.defaultValue);
    }
    else
    {
        picker->SetColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
        picker->Enable(false);
        btnDefault->Enable(false);
        CreateAndSetBitmap(*bmpDefaultColour, wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    }
}

static bool GetColourDefinitionFromList(wxString* id, ColourManager::ColourDef* def, wxListBox* list, int index = wxNOT_FOUND)
{
    if (index == wxNOT_FOUND)
    {
        index = list->GetSelection();
        if (index == wxNOT_FOUND)
            return false;
    }

    const AppColoursClientData* data;
    data = static_cast <AppColoursClientData *> (list->GetClientObject(index));
    if (!data)
        return false;

    const ColourManager::ColourDefMap &colours = Manager::Get()->GetColourManager()->GetColourDefinitions();
    const ColourManager::ColourDefMap::const_iterator it = colours.find(data->id);
    if (it == colours.end())
        return false;

    *def = it->second;
    *id = data->id;
    return true;
}

void EnvironmentSettingsDlg::OnClickAppColour(wxColourPickerEvent &event)
{
    wxListBox* list = XRCCTRL(*this, "lstColours", wxListBox);
    wxString id;
    ColourManager::ColourDef colourDef;
    if (!GetColourDefinitionFromList(&id, &colourDef, list))
        return;

    const wxColour newColour(event.GetColour());
    m_ChangedAppColours[id] = newColour;

    wxButton* btnDefault = XRCCTRL(*this, "btnDefaultColour", wxButton);
    btnDefault->Enable(newColour != colourDef.defaultValue);
}

void EnvironmentSettingsDlg::OnClickAppColourDefault(cb_unused wxCommandEvent& event)
{
    wxListBox* list = XRCCTRL(*this, "lstColours", wxListBox);
    wxString id;
    ColourManager::ColourDef colourDef;
    if (!GetColourDefinitionFromList(&id, &colourDef, list))
        return;

    m_ChangedAppColours[id] = colourDef.defaultValue;
    wxColourPickerCtrl *picker = XRCCTRL(*this, "colourPicker", wxColourPickerCtrl);
    picker->SetColour(colourDef.defaultValue);

    wxButton *btnDefault = XRCCTRL(*this, "btnDefaultColour", wxButton);
    btnDefault->Enable(false);
}

void EnvironmentSettingsDlg::OnClickAppResetAll(cb_unused wxCommandEvent& event)
{
    wxListBox* list = XRCCTRL(*this, "lstColours", wxListBox);
    const int count = list->GetCount();
    const int selected = list->GetSelection();
    for (int index = 0; index < count; ++index)
    {
        wxString id;
        ColourManager::ColourDef colourDef;

        if (!GetColourDefinitionFromList(&id, &colourDef, list, index))
            continue;

        m_ChangedAppColours[id] = colourDef.defaultValue;
        if (index == selected)
        {
            wxColourPickerCtrl *picker = XRCCTRL(*this, "colourPicker", wxColourPickerCtrl);
            picker->SetColour(colourDef.defaultValue);
        }
    }
}

void EnvironmentSettingsDlg::WriteApplicationColours()
{
    if (m_ChangedAppColours.empty())
        return;

    ColourManager *manager = Manager::Get()->GetColourManager();
    for (std::map<wxString, wxColour>::const_iterator it = m_ChangedAppColours.begin();
         it != m_ChangedAppColours.end();
         ++it)
    {
        manager->SetColour(it->first, it->second);
    }
}

/*
  AnnoyingDialog captions are in the form of
  "Question xyz?:4"
  where '4' corresponds to an AnnoyingDialog::dReturnType enum value
  The following two methods translate to and from the human readable form of
  "Question xyz?:Yes
 */

wxString EnvironmentSettingsDlg::AnnoyingDlgReturnToString(const wxString& caption)
{
    std::map<wxString, wxString>::const_iterator it = m_AnnoyingDlgReturnMap.find(caption.AfterLast(wxT(':')));
    if (it != m_AnnoyingDlgReturnMap.end())
        return caption.BeforeLast(wxT(':')) + wxT(':') + it->second;
    return caption;
}

wxString EnvironmentSettingsDlg::StringToAnnoyingDlgReturn(const wxString& caption)
{
    for (std::map<wxString, wxString>::const_iterator it = m_AnnoyingDlgReturnMap.begin();
         it != m_AnnoyingDlgReturnMap.end(); ++it)
    {
        if (caption.AfterLast(wxT(':')) == it->second)
            return caption.BeforeLast(wxT(':')) + wxT(':') + it->first;
    }
    return caption;
}

wxColour EnvironmentSettingsDlg::GetValue(const wxString &id)
{
    const ColourManager::ColourDefMap &colours = Manager::Get()->GetColourManager()->GetColourDefinitions();
    const ColourManager::ColourDefMap::const_iterator it = colours.find(id);
    if (it == colours.end())
        return *wxBLACK;

    std::map<wxString, wxColour>::const_iterator colourIt = m_ChangedAppColours.find(id);
    if (colourIt != m_ChangedAppColours.end())
        return colourIt->second;
    else
        return it->second.value;
}

void EnvironmentSettingsDlg::SetValue(const wxString &id, const wxColour &colour)
{
    m_ChangedAppColours[id] = colour;
}

void EnvironmentSettingsDlg::ResetDefault(const wxString &id)
{
    m_ChangedAppColours.erase(id);
}
