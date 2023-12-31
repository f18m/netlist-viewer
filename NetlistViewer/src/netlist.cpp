/////////////////////////////////////////////////////////////////////////////
// Name:        netlist.cpp
// Purpose:     SPICE netlist parsing & processing
// Author:      Francesco Montorsi
// Created:     30/05/2010
// Copyright:   (c) 2010 Francesco Montorsi
// Licence:     GPL licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
 
#include <wx/wx.h>
#include <wx/wfstream.h>
#include <wx/sstream.h>
#include <wx/tokenzr.h>

#include <string.h>
#include <stdio.h>

#include <algorithm>

#include <boost/graph/kamada_kawai_spring_layout.hpp>
#include <boost/graph/circle_layout.hpp>

#include "netlist.h"
#include "devices.h"


/*
    For more informations about the SPICE netlist format please go to:
      http://www.ecircuitcenter.com/SPICEsummary.htm
*/


// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// globals
// ----------------------------------------------------------------------------

wxPoint svInvalidPoint = wxPoint(-1e9, -1e9);
svNode svGroundNode = svNode("0");      // SPICE conventional name for GND

#define ALLOWED_CHARS       "0123456789.+-"

struct {
    const char* postfixShort;
    const char* postfixLong;
    double multiplier;
} g_mult[] = 
{
    { "F", "FEMTO", 1e-15 },
    { "P", "PICO", 1e-12 },
    { "N", "NANO", 1e-9 },
    { "U", "MICRO", 1e-6 },
    { "M", "MILLI", 1e-3 },
    { "K", "KILO", 1e3 },
    { "MEG", "MEGA", 1e6 },
    { "G", "GIGA", 1e9 },
    { "T", "TERA", 1e12 }
};

struct {
    const char* nameShort;
    const char* nameLong;
} g_units[] = 
{
    { "F", "FARAD" },
    { "OHM", "" },
    { "H", "HENRY" },
    { "A", "AMPERE" },
    { "V", "VOLT" }
};

// ----------------------------------------------------------------------------
// svString
// ----------------------------------------------------------------------------

extern std::string eng(double value, int digits, int numeric);


/* static */
svString svString::formatValue(double v)
{
    return svString(eng(v, 2, 0));
}

bool svString::startsWithOneOf(const std::string& str, unsigned int* len) const
{
    if (len) 
        *len = 0;

    for (size_t i=0; i<this->size(); i++)
    {
        bool ok = false;
        for (size_t j=0; j<str.size(); j++)
        {
            if (at(i) == str.at(j))
            {
                ok = true;
                break;
            }
        }

        if (i == 0 && !len)
            return ok;
        else if (ok)
            (*len)++;
        else if (!ok)
            break;
    }

    return true;
}

bool svString::getValue(double *res) const
{
    unsigned int numlen;
    if (!startsWithOneOf(ALLOWED_CHARS, &numlen))
    {
        *res = 0;
        return false;
    }

    double firstPart = atof(substr(0, numlen).c_str());
    std::string secondPartStr;
    for (size_t i=numlen; i<size(); i++)
        secondPartStr.push_back(toupper(at(i)));

    if (secondPartStr.size() == 0)
    {
        *res = firstPart;
        return true;
    }

    std::string thirdPartStr;
    double secondPart;
    if (secondPartStr.at(0) == 'E')
    {
        if (!svString(secondPartStr.substr(1)).startsWithOneOf(ALLOWED_CHARS, &numlen))
        {
            *res = 0;
            return false;
        }
        
        int exp = atoi(secondPartStr.substr(1, 1+numlen).c_str());
        secondPart = pow(10.0, exp);

        thirdPartStr = secondPartStr.substr(1+numlen);
    }
    else
    {
        secondPart = 0;
        for (size_t i=0; i<WXSIZEOF(g_mult); i++)
        {
            if (wxString(secondPartStr).StartsWith(g_mult[i].postfixLong))
            {
                secondPart = g_mult[i].multiplier;
                thirdPartStr = secondPartStr.substr(strlen(g_mult[i].postfixLong));
                break;
            }
        }

        // try searching for the "short" postfix
        if (secondPart == 0)
        {
            for (size_t i=0; i<WXSIZEOF(g_mult); i++)
            {
                if (wxString(secondPartStr).StartsWith(g_mult[i].postfixShort))
                {
                    secondPart = g_mult[i].multiplier;
                    thirdPartStr = secondPartStr.substr(strlen(g_mult[i].postfixShort));
                    break;
                }
            }
        }

        if (secondPart == 0)
        {
            // maybe there's no multiplier (e.g. "10Volt")
            secondPart = 1.0;
            thirdPartStr = secondPartStr;
        }
    }

    if (thirdPartStr.size() > 0)
    {
        // now we only need to parse 'thirdPartStr'...
        std::string fourthPartStr;
        for (size_t i=0; i<WXSIZEOF(g_units); i++)
        {
            if (strlen(g_units[i].nameLong) > 0 &&
                wxString(thirdPartStr).StartsWith(g_units[i].nameLong))
            {
                fourthPartStr = thirdPartStr.substr(strlen(g_units[i].nameLong));
                break;
            }
            if (wxString(thirdPartStr).StartsWith(g_units[i].nameShort))
            {
                fourthPartStr = thirdPartStr.substr(strlen(g_units[i].nameShort));
                break;
            }
        }

        if (fourthPartStr.size() > 0)
            return false;       // there should be nothing more to parse!
    }

    // last, compose the parsed number:
    *res = firstPart * secondPart;

    return true;
}

// ----------------------------------------------------------------------------
// svParserSPICE
// ----------------------------------------------------------------------------

bool svParserSPICE::load(svCircuitArray& ret, const std::string& filename)
{
    wxFileInputStream input_stream(filename);
    if (!input_stream.IsOk())
    {
        wxLogError("Cannot open file '%s'.", filename);
        return false;
    }

    wxStringOutputStream netlist_contents;
    if (input_stream.Read(netlist_contents).GetLastError() != wxSTREAM_EOF)
    {
        wxLogError("Cannot read file '%s'.", filename);
        return false;
    }

    // first of all, split the file at newline boundaries
    wxArrayString lines = wxStringTokenize(netlist_contents.GetString(), "\n");
    wxArrayString toparse;
    for (size_t i=0; i<lines.size(); i++)
    {
        // remove unwanted blanks from start/end of each line
        lines[i].Trim(false /* from left */);
        lines[i].Trim(true /* from right */);

        // discard empty lines
        if (lines[i].size() == 0)
            continue;

        // discard comments
        if (lines[i].StartsWith("*"))
            continue;

        // + is the continuation character in SPICE syntax
        if (lines[i].StartsWith("+"))
            toparse.back() += lines[i].Mid(1);
        else
            toparse.push_back(lines[i]);
    }

    // FIXME: the line numbers reported in wxLogError from now on will have a 
    // "wrong" number since we removed empty lines and comment lines...
    // this should be fixed adding a toparse => lines index map table

    // handle some special SPICE statements
    // (the .SUBCKT statement)
    for (size_t i=0; i<toparse.size(); i++)
    {
        wxString strtemp;
        if (toparse[i].StartsWith(".SUBCKT ", &strtemp))
        {
            size_t startIdx = i+1;

            // search for the end of this .SUBCKT
            int endIdx = -1;
            for (size_t j=startIdx; j<toparse.size(); j++)
            {
                wxArrayString arr = wxStringTokenize(toparse[j], " ", wxTOKEN_DEFAULT);
                if (arr.size() > 0 && arr[0] == ".ENDS")
                {
                    endIdx = j;
                    break;
                }
            }

            if (endIdx == -1)
            {
                wxLogError("Could not find the .ENDS statement for the .SUBCKT statement of line %d", startIdx);
                return false;
            }

            // parse the subcircuit we just found
            svCircuit sub;
            if (!sub.parseSPICESubCkt(toparse, startIdx, (size_t)endIdx))
                return false;

            // parse arguments of this SUBCKT statement 
            wxArrayString subckt_args = wxStringTokenize(strtemp, " ", wxTOKEN_DEFAULT);
            if (subckt_args.size() > 0)
            {
                sub.setName(subckt_args[0].ToStdString());
                subckt_args.erase(subckt_args.begin());
            }
            for (size_t j=0; j<subckt_args.size(); j++)
                // convert to lowercase because SPICE is case insensitive
                sub.addExternalNode(subckt_args[j].Lower().ToStdString());

            // now finally we can save the parsed subcircuit
            ret.push_back(sub);
        }
    }

    return true;
}

// ----------------------------------------------------------------------------
// svCircuit
// ----------------------------------------------------------------------------

void svCircuit::addExternalNode(const svNode& extNode)
{ 
    m_nodes.insert(extNode); 
    addDevice(new svExternalPin(extNode));
}

bool svCircuit::parseSPICESubCkt(const wxArrayString& lines, size_t startIdx, size_t endIdx)
{
    release();

    for (size_t i=startIdx; i<endIdx; i++)
    {
        wxArrayString arr = wxStringTokenize(lines[i], " ", wxTOKEN_DEFAULT);
        if (arr.size() <= 1)
            continue;

        wxString comp_name = arr[0];
        arr.erase(arr.begin());

        // intercept some "special" SPICE statements
        if (comp_name == ".MODEL")
            continue;

        // first letter of the component identifies it:
        svBaseDevice* dev = svDeviceFactory::getDeviceMatchingIdentifier(comp_name.Upper()[0]);
        if (!dev)
        {
            wxLogError("Unknown component type for '%s'\n", comp_name);
            return false;
        }

        dev->setName(comp_name.substr(1).ToStdString());

        if (arr.size() < dev->getNodesCount())
        {
            wxLogError("At line %d: device '%s' is missing one (or more) of the required nodes", 
                       i, dev->getHumanReadableDesc().c_str());
            return false;
        }

        for (size_t j=0; j<dev->getNodesCount(); j++)
        {
            // convert to lowercase because SPICE is case insensitive
            std::string nodeName = arr[0].Lower().ToStdString();

            // add this node both to the global circuit and to the current device...
            addNode(nodeName);
            dev->addNode(nodeName);

            arr.erase(arr.begin());     // this node has been processed; remove it
        }

        wxASSERT(dev->getNodes().size() == dev->getNodesCount());

        for (size_t j=0; j<arr.size(); j++)
        {
            // there are additional properties device-specific:
            if (!dev->parseSPICEProperty(j, arr[j].ToStdString()))
            {
                wxLogError("Error parsing argument '%s' of line %d: '%s'", arr[j], i, lines[i]);
                return false;
            }
        }

        addDevice(dev);
    }

    return true;
}

svUGraph svCircuit::buildGraph() const
{
    if (m_nodes.empty())
        return svUGraph(0);

    // first, convert the std::set containing the circuit's nodes to a std::vector
    // so that we can associate each circuit node with a number (the node's index) 
    std::vector<svNode> circuitNodes;
    for (std::set<svNode>::const_iterator i = m_nodes.begin(); i != m_nodes.end(); i++)
        circuitNodes.push_back(*i);

    wxASSERT(circuitNodes[0] == svGroundNode);
    svUGraph ug(circuitNodes.size()-1 /* the node 0 (GND) does not need to be part of the graph */);

    // now create an "edge" in the graph for each device
    for (size_t i=0; i<m_devices.size(); i++)
    {
        // all nodes of the same device should be placed nearby...
        const std::vector<svNode>& deviceNodes = m_devices[i]->getNodes();
        std::vector<int> deviceNodeIndexes;
        for (size_t j=0; j<deviceNodes.size(); j++)
        {
            if (deviceNodes[j] != svGroundNode)
            {
                // find this node in the circuit's vector of nodes
                std::vector<svNode>::const_iterator 
                    result = find(circuitNodes.begin(), circuitNodes.end(), deviceNodes[j]);
                wxASSERT(result != circuitNodes.end());
                
                deviceNodeIndexes.push_back(result - circuitNodes.begin() - 1);
            }
        }

        for (size_t j=0; j<deviceNodeIndexes.size(); j++)
            for (size_t k=0; k<deviceNodeIndexes.size(); k++)
                if (k != j)
                    add_edge(deviceNodeIndexes[j], deviceNodeIndexes[k], ug);
    }

    return ug;
}

const wxRect& svCircuit::placeDevices(svPlaceAlgorithm ag)
{
    m_bb = wxRect(0,0,0,0);

    if (m_devices.size() == 0)
        return m_bb;

    switch (ag)
    {
    case SVPA_PLACE_NON_OVERLAPPED:
        {
            wxPoint lastPt;

            // first place all external pins in a row
            std::vector<unsigned int> idx_external_pins;
            for (unsigned int i=0; i<m_devices.size(); i++)
                if (std::string(typeid(*m_devices[i]).name()) == std::string("class svExternalPin"))
                {
                    idx_external_pins.push_back(i);
                    m_devices[i]->setGridPosition(wxRealPoint(idx_external_pins.size(), 0));
                }

            // then place all other devices on a second row
            lastPt.x = 1;
            lastPt.y = 2;
            for (unsigned int i=0; i<m_devices.size(); i++)
            {
                if (std::find(idx_external_pins.begin(), idx_external_pins.end(), i) != idx_external_pins.end())
                    continue;

                unsigned int w = m_devices[i]->getRightmostGridNodePosition() - 
                                 m_devices[i]->getLeftmostGridNodePosition();

                unsigned int center_offset_w = m_devices[i]->getRelativeGridNodePosition(0).x -
                                               m_devices[i]->getLeftmostGridNodePosition();

                m_devices[i]->setGridPosition(lastPt + wxPoint(center_offset_w, 0));

                lastPt.x += w+1;
            }
        }
        break;

    case SVPA_KAMADA_KAWAI:
        {
            svUGraph graph = buildGraph();
            //svUGraph::PositionMap map;
            
            typedef struct 
            {
                double x, y;
            } point2D;

            //typedef typename boost::graph_traits<Graph>::vertex_descriptor Vertex;
//            boost::property_map<svUGraph, point2D> map;// = g�et(boost::vertex_index, graph);
        //    boost::property_map<svUGraph, vertex_id_t>::type vertex_id_map; 
//            circle_graph_layout(graph, map, 20.0);
        }
        break;

    case SVPA_HEURISTIC_1:
        {
            // start placing the first device at the center of our virtual grid
            // (we place it using its first node as reference)
            for (unsigned int i=0; i<m_devices.size(); i++)
                m_devices[i]->setGridPosition(wxPoint(0,0));

            // now place other devices connected to the same nodes of the first device:
            for (size_t j=0; j<m_devices[0]->getNodesCount(); j++)
            {
                // TODO: verify the position is free
                // TODO: cycle on the nodes, not on the devices! first place close together
                //       all devices attached to the same node

                if (m_devices[0]->getNode(j) != svGroundNode)        // node 0 == GND
                {
                    unsigned int temp;
                    for (size_t i=1; i<m_devices.size(); i++)
                        if (m_devices[i]->isConnectedTo(m_devices[0]->getNode(j), &temp))
                        {
                            // place it to the right of the previous device so that it can be easily
                            // connected...
                            wxPoint pos = m_devices[0]->getGridPosition() + wxPoint(1,0);
                            pos.x -= m_devices[i]->getLeftmostGridNodePosition();
                            pos.y += m_devices[i]->getRelativeGridNodePosition(temp).y;
                            m_devices[i]->setGridPosition(pos);
                            break;
                        }
                
                    break;
                }
            }

            // TODO: finish placement of other devices
        }
        break;
    }

    // define the translation values to use to make all grid points positive:
    wxPoint offset;
    for (size_t i=0; i<m_devices.size(); i++)
    {
        wxPoint pt = m_devices[i]->getGridPosition();
        offset.x = std::min(offset.x, pt.x + m_devices[i]->getLeftmostGridNodePosition());
        offset.y = std::min(offset.y, pt.y + m_devices[i]->getTopmostGridNodePosition());
    }
    offset = wxPoint(2,2) + offset*(-1);

    for (size_t i=0; i<m_devices.size(); i++)
        m_devices[i]->setGridPosition(m_devices[i]->getGridPosition() + offset);

    updateBoundingBox();
    return m_bb;
}

void svCircuit::updateBoundingBox()
{
    m_bb.x = m_bb.y = INT_MAX-1;
    for (size_t i=0; i<m_devices.size(); i++)
    {
        m_bb.x = std::min(m_devices[i]->getGridPosition().x +
                          m_devices[i]->getLeftmostGridNodePosition(), m_bb.x);
        m_bb.y = std::min(m_devices[i]->getGridPosition().y +
                          m_devices[i]->getTopmostGridNodePosition(), m_bb.y);

        m_bb.width = std::max(m_devices[i]->getGridPosition().x +
                              m_devices[i]->getRightmostGridNodePosition(), m_bb.width);
        m_bb.height = std::max(m_devices[i]->getGridPosition().y +
                               m_devices[i]->getBottommostGridNodePosition(), m_bb.height);
    }

    m_bb.width -= m_bb.x;
    m_bb.height -= m_bb.y;
}

void svCircuit::initGraphics(wxGraphicsContext*gc, unsigned int gridSize)
{
    s_pathGround = gc->CreatePath();

    double w = gridSize/3.0, d = gridSize/10.0;
    drawLine(s_pathGround, wxRealPoint(-w,0), wxRealPoint(w,0));
    drawLine(s_pathGround, wxRealPoint(-w*2/4,d), wxRealPoint(w*2/4,d));
    drawLine(s_pathGround, wxRealPoint(-w*1/4,2*d), wxRealPoint(w*1/4,2*d));
}

void svCircuit::draw(wxGraphicsContext* gc, unsigned int gridSize, int selectedDevice) const
{
    // draw all the devices
    wxPen normal(*wxBLACK, 2),
          selected(*wxRED, 2);
    gc->SetFont(*wxSWISS_FONT, *wxBLACK);
    for (size_t i=0; i<m_devices.size(); i++)
    {
        m_devices[i]->drawWithDesc(gc, gridSize, selectedDevice == (int)i ? selected : normal);

#if 0
        // draw the bounding box for each device
        gc->SetTransform(gc->CreateMatrix());   // reset the transformation matrix
        wxRect r = m_devices[i]->getRealBoundingBox(gridSize);
        gc->SetPen(selected);
        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        gc->DrawRectangle(r.x, r.y, r.width, r.height);
#endif

        // decorate the nodes of this device
        for (size_t j=0; j<m_devices[i]->getNodesCount(); j++)
        {
            wxRealPoint nodePos = 
                (m_devices[i]->getGridPosition() + m_devices[i]->getRelativeGridNodePosition(j))*gridSize;

            // reset the transformation matrix
            wxGraphicsMatrix m = gc->CreateMatrix();
            m.Translate(nodePos.x, nodePos.y);
            gc->SetTransform(m);

            if (m_devices[i]->getNode(j) == svGroundNode)
                gc->StrokePath(s_pathGround);
            else
                gc->DrawText(m_devices[i]->getNode(j), 0, 0, 0);
        }
    }

    // draw an "airwire" for each device node
    const wxPen* origWirePens[] =
    {
        wxBLUE_PEN,
        wxCYAN_PEN,
        wxGREEN_PEN,
        wxYELLOW_PEN,
        wxGREY_PEN,
        wxLIGHT_GREY_PEN,
        wxMEDIUM_GREY_PEN,
        wxRED_PEN
    };

    std::vector<wxPen> wirePens;
    for (unsigned int i=0; i < WXSIZEOF(origWirePens); i++)
    {
        wirePens.push_back(wxPen(*origWirePens[i]));
        wirePens.back().SetWidth(3);            // default wx pens are VERY THIN!!!
    }

    // reset transformation matrix:
    gc->SetTransform(gc->CreateMatrix());

    unsigned int idx = 0;
    for (std::set<svNode>::const_iterator i=m_nodes.begin(); i != m_nodes.end(); i++)
    {
        if (*i != svGroundNode)
        {
            unsigned int penIdx = (idx++) % wirePens.size();
            gc->SetPen(wirePens[penIdx]);

            // TO-OPTIMIZE: we may scan the device list and then each device's node and 
            //              put the device's node relative position in an array indexed by
            //              the node's name (single pass on each device's node and we have
            //              the data organized as we need).
            std::vector<wxPoint> arrConnectedNodes = getDeviceNodesConnectedTo(*i);
#if 0
            for (size_t j=0; j<arrConnectedNodes.size(); j++)
                for (size_t k=0; k<arrConnectedNodes.size(); k++)
                    if (k != j)
                        drawLine(gc, arrConnectedNodes[j]*gridSize, arrConnectedNodes[k]*gridSize);
#elif 1
            // TODO: we should find the spanning tree over the graph formed by the nodes in arrConnectedNodes
            for (size_t j=1; j<arrConnectedNodes.size(); j++)
                drawLine(gc, arrConnectedNodes[j-1]*gridSize, arrConnectedNodes[j]*gridSize);
#else
            for (size_t j=1; j<arrConnectedNodes.size(); j++)
            {
                wxRealPoint tmppt(arrConnectedNodes[j]*gridSize);

                tmppt.x = arrConnectedNodes[j-1].x*gridSize;
                drawLine(gc, arrConnectedNodes[j-1]*gridSize, tmppt);
                drawLine(gc, tmppt, arrConnectedNodes[j]*gridSize);
            }
#endif
        }
    }
}

std::vector<wxPoint> svCircuit::getDeviceNodesConnectedTo(const svNode& node) const
{
    std::vector<wxPoint> ret;
    for (size_t i = 0; i < m_devices.size(); i++)
    {
        wxPoint pt = m_devices[i]->getRelativeGridNodePosition(node);
        if (pt != svInvalidPoint)
            ret.push_back(m_devices[i]->getGridPosition() + pt);
    }
    return ret;
}

void svCircuit::assign(const svCircuit& tocopy)
{
    release();
    m_name = tocopy.m_name;
    m_nodes = tocopy.m_nodes;
    m_bb = tocopy.m_bb;
    for (size_t i = 0; i < tocopy.m_devices.size(); i++)
        m_devices.push_back(tocopy.m_devices[i]->clone());
}

void svCircuit::release()
{
    for (size_t i = 0; i < m_devices.size(); i++)
        delete m_devices[i];
    m_devices.clear();
    m_name.clear();
    m_nodes.clear();
    m_bb = wxRect(0, 0, 0, 0);
}

int svCircuit::hitTest(const wxPoint& gridPt, unsigned int gridSize, unsigned int tolerance) const
        {
    for (size_t i = 0; i < m_devices.size(); i++)
    {
        wxRect r = m_devices[i]->getRealBoundingBox(gridSize);
        r.Inflate(tolerance, tolerance);
        if (r.x < gridPt.x && r.y < gridPt.y && r.x + r.width >= gridPt.x && r.y + r.height >= gridPt.y)
            return i;
    }
    return wxNOT_FOUND;
}
