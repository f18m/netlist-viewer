/////////////////////////////////////////////////////////////////////////////
// Name:        app.cpp
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
#include <wx/dcbuffer.h>
#include "netlist.h"

#ifdef __WXMSW__
    #include <wx/msw/msvcrt.h>      // useful to catch memory leaks when compiling under MSVC 
#endif

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

#define SV_VERSION_STR         "1.0"

// IDs for the controls and the menu commands
enum
{
    SpiceViewer_ShowGrid = wxID_HIGHEST+1,
    SpiceViewer_Open = wxID_OPEN,
    SpiceViewer_Quit = wxID_EXIT,
    SpiceViewer_About = wxID_ABOUT
};

// ----------------------------------------------------------------------------
// resources
// ----------------------------------------------------------------------------

// the application icon (under Windows and OS/2 it is in resources and even
// though we could still include the XPM here it would be unused)
#if !defined(__WXMSW__) && !defined(__WXPM__)
    #include "icon.xpm"
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

// define a scrollable canvas for displaying the schematic
class SpiceViewerCanvas: public wxScrolledCanvas
{
public:
    SpiceViewerCanvas(wxFrame *parent);

    void OnPaint(wxPaintEvent &event);
    void OnMouseMove(wxMouseEvent &event);
    void OnMouseDown(wxMouseEvent &event);
    void OnMouseUp(wxMouseEvent &event);
    void OnMouseWheel(wxMouseEvent &event);

    void SetCircuit(const svCircuit& ckt)
    { 
        m_ckt = ckt; 
        UpdateVirtualSize();
        UpdateGraphics();
    }

    //! Updates all graphic objects cached in the current circuit (sub)objects.
    //! This function needs to be called only on new circuit (see SetCircuit())
    //! and in case the grid size has been changed (see OnMouseWheel()).
    void UpdateGraphics()
    {
        wxGraphicsContext *gc = wxGraphicsContext::Create(this);
        if (!gc)
            return;

        svDeviceFactory::initGraphics(gc, m_gridSize);
        delete gc;
    }

    void UpdateVirtualSize()
    {
        wxRect rc = m_ckt.getBoundingBox();
        SetVirtualSize((rc.x+rc.width+1)*m_gridSize, 
                       (rc.y+rc.height+1)*m_gridSize);
    }

    void ShowGrid(bool b)
    {
        m_bShowGrid = b; 
        Refresh();
    }

private:        // misc vars
    svCircuit m_ckt;
    unsigned int m_gridSize;
    wxPen m_gridPen;
    bool m_bShowGrid;

private:        // vars for dragging
    svBaseDevice* m_pDraggedDev;
    wxPoint m_ptDraggedDevOffset; // in pixel coords
    unsigned int m_idxDraggedDev;

    wxDECLARE_EVENT_TABLE()
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
    void OnShowGrid(wxCommandEvent& event);

private:
    SpiceViewerCanvas* m_canvas;

    wxDECLARE_EVENT_TABLE()
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// SpiceViewerApp - the application class
// ----------------------------------------------------------------------------

wxIMPLEMENT_APP(SpiceViewerApp)

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
    SpiceViewerFrame *frame = new SpiceViewerFrame("Netlist viewer");

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
// SpiceViewerFrame - main frame
// ----------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(SpiceViewerFrame, wxFrame)
    EVT_MENU(SpiceViewer_ShowGrid,  SpiceViewerFrame::OnShowGrid)
    EVT_MENU(SpiceViewer_Open,  SpiceViewerFrame::OnOpen)
    EVT_MENU(SpiceViewer_Quit,  SpiceViewerFrame::OnQuit)
    EVT_MENU(SpiceViewer_About, SpiceViewerFrame::OnAbout)
wxEND_EVENT_TABLE()

SpiceViewerFrame::SpiceViewerFrame(const wxString& title)
                            : wxFrame(NULL, wxID_ANY, title)
{
    SetIcon(wxICON(appicon));    // set the frame icon

    wxMenu *fileMenu = new wxMenu;
    fileMenu->AppendCheckItem(SpiceViewer_ShowGrid, "&Show grid", 
                              "Should the grid for the devices be shown?")->Check();
    fileMenu->Append(SpiceViewer_Open, "&Open...", "Open a SPICE netlist to view");
    fileMenu->AppendSeparator();
    fileMenu->Append(SpiceViewer_Quit, "E&xit\tAlt-X", "Quit this program");

    wxMenu *helpMenu = new wxMenu;
    helpMenu->Append(SpiceViewer_About, "&About...\tF1", "Show about dialog");

    // now append the freshly created menu to the menu bar...
    wxMenuBar *menuBar = new wxMenuBar();
    menuBar->Append(fileMenu, "&File");
    menuBar->Append(helpMenu, "&Help");

    // ... and attach this menu bar to the frame
    SetMenuBar(menuBar);

    // create a status bar just for fun (by default with 1 pane only)
    CreateStatusBar(1);
    SetStatusText("Welcome to Netlist Viewer " SV_VERSION_STR "!");

    // create the main canvas of this application
    // (it will get automatically linked to this frame and its size will be
    //  adjusted to fill the entire window)
    m_canvas = new SpiceViewerCanvas(this);
}

void SpiceViewerFrame::OnShowGrid(wxCommandEvent& event)
{
    if (m_canvas)
        m_canvas->ShowGrid(event.IsChecked());
}

void SpiceViewerFrame::OnOpen(wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog 
        openFileDialog(this, "Open SPICE netlist", "", "",
                       "SPICE netlists (*.net;*.cir;*.ckt)|*.net;*.cir;*.ckt", wxFD_OPEN|wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;     // the user changed idea...
    
    // proceed loading the file chosen by the user:
    svParserSPICE parser;
    svCircuitArray subcktArray; 
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

    subcktArray[0].placeDevices(SVPA_PLACE_NON_OVERLAPPED);
    m_canvas->SetCircuit(subcktArray[0]);
    SetTitle(wxString::Format("Netlist Viewer [%s]", subcktArray[0].getName()));

    Refresh();
}

void SpiceViewerFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    Close(true /* force the frame to close */);
}

void SpiceViewerFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxAboutDialogInfo aboutInfo;
    aboutInfo.SetName("Netlist Viewer");
    aboutInfo.SetVersion(SV_VERSION_STR);
    aboutInfo.SetDescription("SPICE netlist viewer. This program converts a SPICE text netlist to a graphical schematic.");
    aboutInfo.SetCopyright("(C) 2010");
    aboutInfo.SetWebSite("https://sourceforge.net/projects/netlistviewer");
    aboutInfo.AddDeveloper("Francesco Montorsi");

    wxAboutBox(aboutInfo);
}

// ----------------------------------------------------------------------------
// SpiceViewerCanvas
// ----------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(SpiceViewerCanvas, wxScrolledCanvas)
    EVT_PAINT(SpiceViewerCanvas::OnPaint)

    EVT_MOUSEWHEEL(SpiceViewerCanvas::OnMouseWheel)
    EVT_MOTION(SpiceViewerCanvas::OnMouseMove)

    // left&right mouse buttons:
    EVT_LEFT_DOWN(SpiceViewerCanvas::OnMouseDown)
    EVT_LEFT_UP(SpiceViewerCanvas::OnMouseUp)
    EVT_RIGHT_UP(SpiceViewerCanvas::OnMouseUp)
wxEND_EVENT_TABLE()

SpiceViewerCanvas::SpiceViewerCanvas(wxFrame *parent)
        : wxScrolledCanvas(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                           wxHSCROLL | wxVSCROLL | wxFULL_REPAINT_ON_RESIZE)
{
    m_pDraggedDev = NULL;
    m_idxDraggedDev = wxNOT_FOUND;
    m_gridSize = 40;
    m_gridPen = wxPen(*wxLIGHT_GREY, 1, wxPENSTYLE_DOT);
    m_bShowGrid = true;

    SetScrollRate(m_gridSize/10, m_gridSize/10);
    SetCursor(wxCURSOR_CROSS);
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);
}

void SpiceViewerCanvas::OnPaint(wxPaintEvent &WXUNUSED(event))
{
#if 1
    wxPaintDC dc(this);
    DoPrepareDC(dc);
#else
    wxBufferedPaintDC dc(this, wxBUFFER_VIRTUAL_AREA);
#endif

    // clear our background
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.SetBackgroundMode(wxSOLID);
    dc.SetPen(*wxTRANSPARENT_PEN);
    //dc.Clear();  -- doesn't clear all the virtual area!
    dc.DrawRectangle(wxPoint(0,0), GetVirtualSize()+wxSize(1,1));

    // draw the grid
    if (m_bShowGrid)
    {
        dc.SetPen(m_gridPen);
        wxSize sz = GetVirtualSize();
        for (int xx=m_gridSize; xx<sz.GetWidth(); xx+=m_gridSize)
            dc.DrawLine(xx, 0, xx, sz.GetHeight());
        for (int yy=m_gridSize; yy<sz.GetHeight(); yy+=m_gridSize)
            dc.DrawLine(0, yy, sz.GetWidth(), yy);
    }

    // draw the schematic currently loaded
    /*m_ckt.draw(dc, m_gridSize, 
               m_pDraggedDev ? m_idxDraggedDev : wxNOT_FOUND);*/
    wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
    if (!gc)
        return;

    m_ckt.draw(gc, m_gridSize, m_pDraggedDev ? m_idxDraggedDev : wxNOT_FOUND);
    delete gc;
}

void SpiceViewerCanvas::OnMouseDown(wxMouseEvent &event)
{
    if (m_pDraggedDev != NULL || !event.LeftDown())
        return;

    wxClientDC dc(this);
    DoPrepareDC(dc);

    wxPoint click(event.GetLogicalPosition(dc));
    int idx = m_ckt.hitTest(wxRealPoint(click.x, click.y)/m_gridSize, 0.25);
    if (idx == wxNOT_FOUND)
        return;

    // the device we're dragging:
    m_pDraggedDev = m_ckt.getDevices().at(idx);
    m_idxDraggedDev = idx;

    // the offset (in pixel) between the clicked point and the reference node of the dragged device
    m_ptDraggedDevOffset = m_pDraggedDev->getGridPosition()*m_gridSize - click;

    Refresh();
}

void SpiceViewerCanvas::OnMouseMove(wxMouseEvent &event)
{
    if (!m_pDraggedDev)
        return;

    wxClientDC dc(this);
    DoPrepareDC(dc);
    wxPoint click(event.GetLogicalPosition(dc));

    // compute the delta of the distance (in grid units) from the original
    // dragged device's position
    double dx = double(click.x + m_ptDraggedDevOffset.x)/m_gridSize - m_pDraggedDev->getGridPosition().x;
    double dy = double(click.y + m_ptDraggedDevOffset.y)/m_gridSize - m_pDraggedDev->getGridPosition().y;

    // are we closer to another grid point?
    if (fabs(dx)>0.5 || fabs(dy)>0.5)
    {
        // get the position of the closest grid point
        wxPoint newGridPt = m_pDraggedDev->getGridPosition() + wxPoint(wxRound(dx),wxRound(dy));

        // update&refresh
        m_pDraggedDev->setGridPosition(newGridPt);
        m_ckt.updateBoundingBox();
        UpdateVirtualSize();
        Refresh();
    }
}

void SpiceViewerCanvas::OnMouseUp(wxMouseEvent &event)
{
    if (event.LeftUp())
    {
        m_pDraggedDev = NULL;
        Refresh();
    }
    else if (event.RightUp() && m_pDraggedDev)
    {
        // rotate the device being dragged
        m_pDraggedDev->rotateClockwise();
        Refresh();
    }
}

void SpiceViewerCanvas::OnMouseWheel(wxMouseEvent &event)
{
    if (!event.ControlDown())
    {
        wxPoint offset;
        if (event.GetWheelAxis() == 0)      // vertical
            offset = wxPoint(0, -2*event.GetWheelRotation()/event.GetWheelDelta());
        else if (event.GetWheelAxis() == 1)      // horizontal
            offset = wxPoint(-2*event.GetWheelRotation()/event.GetWheelDelta(), 0);

        Scroll(GetViewStart() + offset);

        return;
    }

    // zoom!
    m_gridSize += 2*event.GetWheelRotation()/event.GetWheelDelta();
    if (m_gridSize < 10)
        m_gridSize = 10;
    else if (m_gridSize > 150)
        m_gridSize = 150;

    UpdateGraphics();
    UpdateVirtualSize();
    Refresh();
}
