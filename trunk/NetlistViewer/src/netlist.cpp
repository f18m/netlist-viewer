/////////////////////////////////////////////////////////////////////////////
// Name:        netlist.c
// Purpose:     SPICE netlist parsing & processing
// Author:      Francesco Montorsi
// Created:     30/05/2010
// RCS-ID:      $Id$
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

#include "netlist.h"


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
 
svBaseDeviceArray svDeviceFactory::s_registered;
wxPoint svInvalidPoint = wxPoint(-1e10, -1e10);

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
// classes
// ----------------------------------------------------------------------------

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

bool svParser::loadSubCkt(svSubckt& ret, const wxArrayString& lines, size_t startIdx, size_t endIdx)
{
    for (size_t i=startIdx; i<endIdx; i++)
    {
        wxLogDebug(lines[i]);

        wxArrayString arr = wxStringTokenize(lines[i], " ", wxTOKEN_DEFAULT);
        if (arr.size() <= 1)
            continue;

        wxString comp_name = arr[0];
        arr.erase(arr.begin());

        // intercept some "special" SPICE statements
        if (comp_name == ".MODEL")
            continue;

        // first letter of the component identifies it:
        svBaseDevice* dev = svDeviceFactory::getDeviceMatchingIdentifier(comp_name[0]);
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
            ret.addNode(arr[0].ToStdString());
            dev->addNode(arr[0].ToStdString());

            arr.erase(arr.begin());     // this node has been processed; remove it
        }

        for (size_t j=0; j<arr.size(); j++)
        {
            // there are additional properties device-specific:
            if (!dev->parseProperty(j, arr[j].ToStdString()))
            {
                wxLogError("Error parsing argument '%s' of line %d: '%s'", arr[j], i, lines[i]);
                return false;
            }
        }

        ret.addDevice(dev);
    }

    return true;
}

bool svParser::load(svSubcktArray& ret, const std::string& filename)
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
        lines[i].Trim(false /* from left */);
        lines[i].Trim(true /* from right */);

        // discard comments
        if (!lines[i].StartsWith("*") && lines[i].size() > 0)
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
            svSubckt sub(strtemp.BeforeFirst(' ').ToStdString());
            if (!loadSubCkt(sub, toparse, startIdx, (size_t)endIdx))
                return false;

            ret.push_back(sub);
        }
    }

    return true;
}

svUGraph svSubckt::buildGraph() const
{
    if (m_nodes.empty())
        return svUGraph(0);

    // first, convert the std::set containing the circuit's nodes to a std::vector
    // so that we can associate each circuit node with a number (the node's index) 
    std::vector<svNode> circuitNodes;
    for (std::set<svNode>::const_iterator i = m_nodes.begin(); i != m_nodes.end(); i++)
        circuitNodes.push_back(*i);

    wxASSERT(circuitNodes[0] == "0");
    svUGraph ug(circuitNodes.size()-1 /* the node 0 (GND) does not need to be part of the graph */);

    // now create an "edge" in the graph for each device
    for (size_t i=0; i<m_devices.size(); i++)
    {
        // all nodes of the same device should be placed nearby...
        const std::vector<svNode>& deviceNodes = m_devices[i]->getNodes();
        std::vector<int> deviceNodeIndexes;
        for (size_t j=0; j<deviceNodes.size(); j++)
        {
            if (deviceNodes[j] != "0")
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

svSchematic svSubckt::buildSchematic() const
{
    svSchematic ret;

    return ret;
}

void svSubckt::draw(wxDC& dc) const
{
/*    class svGridPosition
    {
        int x;
        int y;

    public:
        svGridPosition(int xx, int yy) 
            { x=xx; y=yy; }
    };

    std::vector<svGridPosition> m_devicesPos;
*/
    std::vector<wxPoint> m_devicesPos;

    // start placing the first device at the center of our virtual grid
    // (we place it using its first node as reference)
    //m_devicesPos.push_back(wxPoint(0,0));
    //m_devicesPos.insert(m_devicesPos.end(), m_devices.size()-1, svInvalidPoint);
    m_devicesPos.insert(m_devicesPos.end(), m_devices.size(), wxPoint(0,0));

    // now place other devices connected to the same nodes of the first device:
    for (size_t j=0; j<m_devices[0]->getNodesCount(); j++)
    {
        // TODO: verify the position is free
        // TODO: cycle on the nodes, not on the devices! first place close together
        //       all devices attached to the same node

        if (m_devices[0]->getNode(j) != "0")        // node 0 == GND
        {
            unsigned int temp;
            for (size_t i=1; i<m_devices.size(); i++)
                if (m_devices[i]->isConnectedTo(m_devices[0]->getNode(j), &temp))
                {
                    // place it to the right of the previous device so that it can be easily
                    // connected...
                    m_devicesPos[i] = m_devicesPos[0] + wxPoint(1,0);
                    m_devicesPos[i].x -= m_devices[i]->getLeftmostGridNodePosition().x;
                    m_devicesPos[i].y += m_devices[i]->getGridNodePosition(temp).y;
                    break;
                }
        
            break;
        }
    }

    // define the translation values to use to make all grid points positive:
    wxPoint offset;
    for (size_t i=0; i<m_devices.size(); i++)
    {
        offset.x = std::min(offset.x, m_devicesPos[i].x);
        offset.y = std::min(offset.y, m_devicesPos[i].y);
    }
    offset = wxPoint(2,2) + offset*(-1);

    // draw the grid
    const unsigned int gridSize = 50;
    dc.SetPen(*wxLIGHT_GREY_PEN);
    for (int xx=0; xx<dc.GetSize().GetWidth(); xx+=gridSize)
        dc.DrawLine(xx, 0, xx, dc.GetSize().GetHeight());
    for (int yy=0; yy<dc.GetSize().GetHeight(); yy+=gridSize)
        dc.DrawLine(0, yy, dc.GetSize().GetWidth(), yy);

    // draw all the devices
    dc.SetPen(wxPen(*wxBLACK, 2));
    for (size_t i=0; i<m_devices.size(); i++)
    {
        m_devices[i]->draw(dc, offset + m_devicesPos[i], gridSize);

        dc.SetBackgroundMode(wxTRANSPARENT);
        for (size_t j=0; j<m_devices[i]->getNodesCount(); j++)
            dc.DrawText(m_devices[i]->getNode(j), 
                        (offset + m_devicesPos[i] + m_devices[i]->getGridNodePosition(j))*gridSize);
        dc.SetBackgroundMode(wxSOLID);
    }
}

