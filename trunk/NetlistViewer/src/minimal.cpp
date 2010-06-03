/////////////////////////////////////////////////////////////////////////////
// Name:        minimal.cpp
// Purpose:     Spice Viewer application entry point
// Author:      Francesco Montorsi
// Created:     28/05/2010
// RCS-ID:      $Id$
// Copyright:   (c) Francesco Montorsi
// Licence:     GPL license
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
 
// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"
 
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/aboutdlg.h>
#include <boost/graph/kamada_kawai_spring_layout.hpp>
#include <boost/graph/circle_layout.hpp>

#include "netlist.h"

#define SV_VERSION_STR         "1.0"

// ----------------------------------------------------------------------------
// resources
// ----------------------------------------------------------------------------

// the application icon (under Windows and OS/2 it is in resources and even
// though we could still include the XPM here it would be unused)
#if !defined(__WXMSW__) && !defined(__WXPM__)
    #include "sample.xpm"
#endif

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// main application class
class SpiceViewerApp : public wxApp
{
public:
    virtual bool OnInit();
    virtual int OnExit();
};

// main application frame
class SpiceViewerFrame : public wxFrame
{
public:
    SpiceViewerFrame(const wxString& title);

    // event handlers (these functions should _not_ be virtual)
    void OnOpen(wxCommandEvent& event);
    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);

private:
    DECLARE_EVENT_TABLE()
};

// define a scrollable canvas for drawing onto
class SpiceViewerCanvas: public wxScrolledWindow
{
public:
    SpiceViewerCanvas(wxFrame *parent);

    void OnPaint(wxPaintEvent &event);
    /*void OnMouseMove(wxMouseEvent &event);
    void OnMouseDown(wxMouseEvent &event);
    void OnMouseUp(wxMouseEvent &event);*/

private:
    DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// IDs for the controls and the menu commands
enum
{
    SpiceViewer_Open = wxID_OPEN,
    SpiceViewer_Quit = wxID_EXIT,
    SpiceViewer_About = wxID_ABOUT
};

// ----------------------------------------------------------------------------
// event tables and other macros for wxWidgets
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(SpiceViewerFrame, wxFrame)
    EVT_MENU(SpiceViewer_Open,  SpiceViewerFrame::OnOpen)
    EVT_MENU(SpiceViewer_Quit,  SpiceViewerFrame::OnQuit)
    EVT_MENU(SpiceViewer_About, SpiceViewerFrame::OnAbout)
END_EVENT_TABLE()

IMPLEMENT_APP(SpiceViewerApp)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// the application class
// ----------------------------------------------------------------------------

bool SpiceViewerApp::OnInit()
{
    // call the base class initialization method, currently it only parses a
    // few common command-line options but it could be do more in the future
    if ( !wxApp::OnInit() )
        return false;

    svDeviceFactory::registerAllDevices();
    setlocale(LC_NUMERIC, "C");

#define SELF_TESTS 1
#if SELF_TESTS
    struct {
        svString testString;
        double value;
    } test[] = 
    {
        { "1", 1 },
        { "2.3", 2.3 },
        { "2.3e-9", 2.3e-9 },
        { "23.3n", 23.3e-9 },
        { "2.3nF", 2.3e-9 },
        { "99.9pFaraD", 99.9e-12 },
        { "10V", 10 }
    };

    const double EPSILON = 1e-9;

    for (size_t i=0; i<WXSIZEOF(test); i++)
    {
        double temp;
        wxASSERT(test[i].testString.getValue(&temp));
        wxASSERT(fabs(temp - test[i].value) < EPSILON);
    }
#endif

    // create the main application window
    SpiceViewerFrame *frame = new SpiceViewerFrame("SPICE netlist viewer");

    // and show it (the frames, unlike simple controls, are not shown when
    // created initially)
    frame->Show(true);

    // success: wxApp::OnRun() will be called which will enter the main message
    // loop and the application will run. If we returned false here, the
    // application would exit immediately.
    return true;
}

int SpiceViewerApp::OnExit()
{
    svDeviceFactory::unregisterAllDevices();

    return wxApp::OnExit();
}

// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------

SpiceViewerFrame::SpiceViewerFrame(const wxString& title)
                            : wxFrame(NULL, wxID_ANY, title)
{
    // set the frame icon
    SetIcon(wxICON(sample));

    // create a menu bar
    wxMenu *fileMenu = new wxMenu;

    // the "About" item should be in the help menu
    wxMenu *helpMenu = new wxMenu;
    helpMenu->Append(SpiceViewer_About, "&About...\tF1", "Show about dialog");
    fileMenu->Append(SpiceViewer_Open, "&Open", "Open a SPICE netlist to view");
    fileMenu->AppendSeparator();
    fileMenu->Append(SpiceViewer_Quit, "E&xit\tAlt-X", "Quit this program");

    // now append the freshly created menu to the menu bar...
    wxMenuBar *menuBar = new wxMenuBar();
    menuBar->Append(fileMenu, "&File");
    menuBar->Append(helpMenu, "&Help");

    // ... and attach this menu bar to the frame
    SetMenuBar(menuBar);

    // create a status bar just for fun (by default with 1 pane only)
    CreateStatusBar(1);
    SetStatusText("Welcome to SPICE netlist viewer " SV_VERSION_STR "!");

    new SpiceViewerCanvas(this);
}


// ----------------------------------------------------------------------------
// main frame - event handlers
// ----------------------------------------------------------------------------

    svSubcktArray subcktArray; 

void SpiceViewerFrame::OnOpen(wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog 
        openFileDialog(this, _("Open SPICE netlist"), "", "",
                       "SPICE netlists (*.net;*.cir)|*.net;*.cir", wxFD_OPEN|wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;     // the user changed idea...
    
    // proceed loading the file chosen by the user:
    svParser parser;
    if (!parser.load(subcktArray, openFileDialog.GetPath().ToStdString()))
    {
        wxLogError("Error while parsing the netlist file '%s'", openFileDialog.GetPath());
        return;
    }

    if (subcktArray.size() == 0)
    {
        wxLogError("The nelist file '%s' didn't contain any subcircuit", openFileDialog.GetPath());
        return;
    }

    if (subcktArray.size() > 1)
    {
        wxLogError("Sorry: multi-subcircuits not supported yet...");
        return;
    }

#if 0
    svUGraph graph = subcktArray[0].buildGraph();
    //svUGraph::PositionMap map;
    
    typedef struct 
    {
        double x, y;
    } point2D;

    //typedef typename boost::graph_traits<Graph>::vertex_descriptor Vertex;
    boost::property_map<svUGraph, point2D> map;// = g§et(boost::vertex_index, graph);
//    boost::property_map<svUGraph, vertex_id_t>::type vertex_id_map; 
    circle_graph_layout(graph, map, 20.0);
#else
    subcktArray[0].buildSchematic();
#endif

    Refresh();
}

void SpiceViewerFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    Close(true /* force the frame to close */);
}

void SpiceViewerFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxAboutDialogInfo aboutInfo;
    aboutInfo.SetName("Spice Viewer");
    aboutInfo.SetVersion(SV_VERSION_STR);
    aboutInfo.SetDescription(_("SPICE netlist viewer. This program converts a SPICE text netlist to a graphical schematic."));
    aboutInfo.SetCopyright("(C) 2010");
    aboutInfo.SetWebSite("http://???");
    aboutInfo.AddDeveloper("Francesco Montorsi");

    wxAboutBox(aboutInfo);
}

// ----------------------------------------------------------------------------
// SpiceViewerCanvas
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(SpiceViewerCanvas, wxScrolledWindow)
    EVT_PAINT(SpiceViewerCanvas::OnPaint)
    /*EVT_MOTION(MyCanvas::OnMouseMove)
    EVT_LEFT_DOWN(MyCanvas::OnMouseDown)
    EVT_LEFT_UP(MyCanvas::OnMouseUp)*/
END_EVENT_TABLE()

SpiceViewerCanvas::SpiceViewerCanvas(wxFrame *parent)
        : wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                           wxHSCROLL | wxVSCROLL | wxNO_FULL_REPAINT_ON_RESIZE)
{
}

void SpiceViewerCanvas::OnPaint(wxPaintEvent &WXUNUSED(event))
{
    wxPaintDC pdc(this);
    PrepareDC(pdc);
    //m_owner->PrepareDC(dc);

    pdc.SetBackground(*wxWHITE_BRUSH);
    pdc.SetBackgroundMode(wxSOLID);
    pdc.Clear();

    if (subcktArray.size() > 0)
        subcktArray[0].draw(pdc);
}



