/////////////////////////////////////////////////////////////////////////////
// Name:        netlist.h
// Purpose:     SPICE netlist parsing & processing
// Author:      Francesco Montorsi
// Created:     30/05/2010
// RCS-ID:      $Id$
// Copyright:   (c) 2010 Francesco Montorsi
// Licence:     GPL licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _NETLIST_H_
#define _NETLIST_H_

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
 
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <set>

#include <wx/graphics.h>

#include <boost/functional/hash.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/base_object.hpp>

// ----------------------------------------------------------------------------
// typedefs & enums
// ----------------------------------------------------------------------------

class svBaseDevice;
class svCircuit;

typedef boost::adjacency_matrix<boost::undirectedS> svUGraph;
typedef std::string svNode;
typedef std::vector<svBaseDevice*> svBaseDeviceArray;
typedef std::vector<svCircuit> svCircuitArray;

enum svRotation
{
    SVR_0 = 0,      //!< no rotation.
    SVR_90 = 1,     //!< 90 degrees clockwise rotation.
    SVR_180 = 2,    //!< 180 degrees clockwise rotation.
    SVR_270 = 3     //!< 270 degrees clockwise rotation.
};

enum svPlaceAlgorithm
{
    SVPA_PLACE_NON_OVERLAPPED,
    SVPA_KAMADA_KAWAI,
    SVPA_HEURISTIC_1
};

// globals:

extern wxPoint svInvalidPoint;
extern svNode svGroundNode;

// ----------------------------------------------------------------------------
// helper functions
// ----------------------------------------------------------------------------
 
//! Graphic helper; draws a straight line on the given graphic path.
static void drawLine(wxGraphicsPath& path, const wxRealPoint& pt1, const wxRealPoint& pt2)
{
    path.MoveToPoint(pt1.x, pt1.y);
    path.AddLineToPoint(pt2.x, pt2.y);
}

//! Graphic helper; draws a straight line on the given graphic path.
static void drawLine(wxGraphicsContext* gc, const wxRealPoint& pt1, const wxRealPoint& pt2)
{
    gc->StrokeLine(pt1.x, pt1.y, pt2.x, pt2.y);
}

//! Graphic helper; optimized rotation for a rectangle around origin.
static wxRect2DDouble rotateRect(const wxRect2DDouble& r, svRotation rot)
{
    wxRect2DDouble ret;
    switch (rot)
    {
    case SVR_0:
        ret = r;
        break;
    case SVR_90:
        ret.SetLeftTop(wxPoint2DDouble(-r.GetBottom(), r.GetLeft()));
        ret.SetRightBottom(wxPoint2DDouble(-r.GetTop(), r.GetRight()));
        break;
    case SVR_180:
        ret.SetLeftTop(wxPoint2DDouble(-r.GetRight(), -r.GetBottom()));
        ret.SetRightBottom(wxPoint2DDouble(-r.GetLeft(), -r.GetTop()));
        break;
    case SVR_270:
        ret.SetLeftTop(wxPoint2DDouble(r.GetTop(), -r.GetRight()));
        ret.SetRightBottom(wxPoint2DDouble(r.GetBottom(), -r.GetLeft()));
        break;
    }

    ret.m_width = fabs(ret.m_width);
    ret.m_height = fabs(ret.m_height);

    return ret;
}

namespace boost {
namespace serialization {
    // NOTE: when the class Archive corresponds to an output archive, the
    // & operator is defined similar to <<.  Likewise, when the class Archive
    // is a type of input archive the & operator is defined similar to >>.

    template<class Archive>
    void serialize(Archive& ar, wxRect& r, const unsigned int WXUNUSED(version))
    {
        ar & r.x; ar & r.y; ar & r.width; ar & r.height;
    }

    template<class Archive>
    void serialize(Archive& ar, wxPoint& p, const unsigned int WXUNUSED(version))
    {
        ar & p.x; ar & p.y;
    }
} // namespace serialization
} // namespace boost


// ----------------------------------------------------------------------------
// helper classes
// ----------------------------------------------------------------------------
 
//! An extension of the standard string class to handle some SPICE-specific parsing.
class svString : public wxString
{
public:
    svString() {}
    svString(const wxString& str) : wxString(str) {}
    svString(const char* str) : wxString(str) {}

    //! Returns true if this string starts with a character contained in the
    //! given string @a str. If it does, returns how many characters at the
    //! beginning of the string are contained in @a str.
    bool startsWithOneOf(const std::string& str, unsigned int* len = NULL) const;

    //! Parses this string as if it contains a SPICE value.
    //! SPICE values are written either in scientific format (xxxEyyy)
    //! or using unit multipliers (xxxU).
    bool getValue(double* res) const;

    //! Returns a string containing a number formatted in engineering format.
    static svString formatValue(double value);
};




// ----------------------------------------------------------------------------
// svCircuit
// ----------------------------------------------------------------------------

//! A generic electrical network.
//! In SPICE terms, this class represents a subcircuit.
class svCircuit
{
    //! The name of this circuit.
    std::string m_name;

    //! The array of electrical (internal) nodes.
    //! Each node is connected to one or more device nodes.
    std::set<svNode> m_nodes;

    //! The bounding box for the grid where the devices of this circuit are placed.
    //! This member variable is updated only by the placeDevices() function.
    wxRect m_bb;

    //! The array of devices.
    //! Each device has two or more nodes connected with the elements of the m_nodes array.
    std::vector<svBaseDevice*> m_devices;

    //! The ground symbol.
    static wxGraphicsPath s_pathGround;

    void assign(const svCircuit& tocopy);
    void release();

private:     // serialization functions

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int WXUNUSED(version))
    {
        ar & m_name;
        ar & m_nodes;
        ar & m_bb;
        ar & m_devices;
    }

public:
    svCircuit(const std::string& name = "") 
        { m_name = name; }

    svCircuit(const svCircuit& tocopy) 
    {
        assign(tocopy);
    }

    ~svCircuit()
    {
        release();
    }

    svCircuit& operator=(const svCircuit& value)
    {
        assign(value);
        return *this;
    }

public:     // node & device management functions

    //! Adds an external node to this subcircuit.
    //! An external node can be connected to the network outside the subcircuit;
    //! all other internal nodes cannot be connected to an external network.
    void addExternalNode(const svNode& extNode);

    //! Adds an internal node to this subcircuit (unless a node with the same name already exists!).
    void addNode(const svNode& name)
        { m_nodes.insert(name); }

    //! Adds the given device to this subcircuit.
    //! Note that this object will take the ownership of the given pointer.
    void addDevice(svBaseDevice* dev)
        { m_devices.push_back(dev); }

    const std::set<svNode>& getNodes() const
        { return m_nodes; }
    const std::vector<svBaseDevice*>& getDevices() const
        { return m_devices; }

    //! Returns an array of positions of the device nodes connected with the
    //! the given one.
    std::vector<wxPoint> getDeviceNodesConnectedTo(const svNode& node) const;

public:     // misc functions

    void setName(const std::string& name)
        { m_name = name; }
    std::string getName() const
        { return m_name; }

    //! FIXME
    svUGraph buildGraph() const;

    //! Updates the devices' positions (in the virtual grid) using the 
    //! specified algorithm. Returns the bounding box of the circuit.
    const wxRect& placeDevices(svPlaceAlgorithm ag);

public:     // drawing functions

    //! Updates the internal bouding box. Call this function after e.g.
    //! changing the position of one of the circuit's devices.
    void updateBoundingBox();

    //! Returns the bounding box for the devices of this circuit
    //! as grid coordinates.
    const wxRect& getBoundingBox() const
        { return m_bb; }

    static void initGraphics(wxGraphicsContext* gc, unsigned int gridSpacing);

    static void releaseGraphics()
        { s_pathGround.UnRef(); }

    //! Draws this circuit on the given DC, with the given grid size
    //! (in pixels).
    void draw(wxGraphicsContext* gc, unsigned int gridSpacing, 
              int selectedDevice = wxNOT_FOUND) const;

    //! Returns the index of the first device whose (absolute) center point
    //! lies in the given rectangle.
    int hitTest(const wxPoint& gridPt, unsigned int gridSize, unsigned int tolerance) const;

public:     // parser functions

    //! Parses the given lines as a SPICE description of a SUBCKT.
    bool parseSPICESubCkt(const wxArrayString& lines, 
                          size_t startIdx, size_t endIdx);
};



// ----------------------------------------------------------------------------
// svParserSPICE
// ----------------------------------------------------------------------------

//! The parser class for SPICE netlists.
class svParserSPICE
{
public:
    svParserSPICE() {}
    
    //! Loads a SPICE netlist and returns the array of parsed subcircuits.
    bool load(svCircuitArray& ret, const std::string& filename);
};


#endif      // _NETLIST_H_
