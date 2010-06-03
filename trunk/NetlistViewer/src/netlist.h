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

#include <boost/graph/adjacency_matrix.hpp>

#include "schematic.h"

// ----------------------------------------------------------------------------
// typedefs & enums
// ----------------------------------------------------------------------------

class svBaseDevice;
class svSubckt;

typedef boost::adjacency_matrix<boost::undirectedS> svUGraph;
typedef std::string svNode;
typedef std::vector<svBaseDevice*> svBaseDeviceArray;
typedef std::vector<svSubckt> svSubcktArray;

enum svRotation
{
    SVR_0,      //!< no rotation.
    SVR_90,     //!< 90 degrees clockwise rotation.
    SVR_180,    //!< 180 degrees clockwise rotation.
    SVR_270     //!< 270 degrees clockwise rotation.
};

extern wxPoint svInvalidPoint;

// ----------------------------------------------------------------------------
// helper classes
// ----------------------------------------------------------------------------
 
//! An extension of the standard string class to handle some SPICE-specific parsing.
class svString : public wxString
{
public:
    svString() {}
    svString(const wxString& str) : wxString(str) {}

    bool startsWithOneOf(const std::string& str, unsigned int* len = NULL) const;

    //! Parses this string as if it contains a SPICE value.
    //! SPICE values are written either in scientific format (xxxEyyy)
    //! or using unit multipliers (xxxU).
    bool getValue(double* res) const;
};

//! A basic SPICE device.
class svBaseDevice
{
    //! The nodes connected with this device.
    std::vector<svNode> m_nodes;

    //! The name of this device.
    std::string m_name;

public:     // misc functions

    //! Returns the character which characterizes the device in a SPICE netlist
    //! like 'C' for capacitors or 'R' for resistors.
    virtual char getSpiceIdentifier() const = 0;

    //! Returns an uppercase string with a human-readable description of this device.
    virtual std::string getHumanReadableDesc() const = 0;

    //! Clones this instance and returns the just allocated object.
    virtual svBaseDevice* clone() const = 0;


    void setName(const std::string& name)
        { m_name = name; }
    std::string getName() const
        { return m_name; }

public:     // node management functions

    //! Adds the given node name to the list of nodes connected to this device.
    void addNode(const svNode& newnode)
        {
            m_nodes.push_back(newnode); 
            wxASSERT(m_nodes.size() <= getNodesCount());
        }

    //! Returns the nodes to which this device is connected.
    const std::vector<svNode>& getNodes() const
        { return m_nodes; }

    //! Returns the i-th node.
    const svNode& getNode(unsigned int i) const
        { return m_nodes[i]; }

    //! Returns true if this device is connected to the given node.
    bool isConnectedTo(const svNode& node, unsigned int* idx = NULL) const
        { 
            std::vector<svNode>::const_iterator it = find(m_nodes.begin(), m_nodes.end(), node);
            if (it != m_nodes.end())
            {
                if (idx) *idx = it - m_nodes.begin();
                return true;
            }

            return false;
        }

    //! Returns the number of nodes to which this device is connected
    //! (which corresponds to the number of "pins" of this device).
    virtual unsigned int getNodesCount() const = 0;

public:     // parsing functions

    //! Parses the given string as a property for this device 
    //! (which occurs as the j-th argument on the SPICE netlist).
    //! FIXME FIXME:
    //!   convert this to a parseProperties(const wxArrayString& props);
    //!   which receives all the strings listed after the component's node connections
    virtual bool parseProperty(unsigned int j, const std::string& prop) = 0;

public:     // drawing functions

    // IMPORTANT: wxPoint containing 'grid positions' are points containing integer
    //            coordinates eventually negative. wxPoint containing 'pixel positions'
    //            are instead points referred to a specific grid spacing and are always
    //            positive.

    //! Draws this device on the given DC placing the first node of the device at
    //! the given grid position. Note that the conversion between the grid position 'x'
    //! and the pixel position 'y' is computed as:
    //!     y = x*gridSpacing;
    virtual void draw(wxDC& dc, const wxPoint& firstNodeGridPosition, unsigned int gridSpacing) const
        {
            dc.DrawText(getSpiceIdentifier(), firstNodeGridPosition*gridSpacing);
        }

    //! Returns the grid position (relative to zero-th node) for the given node index
    //! (which should always be < getNodesCount()).
    //! The grid position for the zero-th node is always (0,0).
    //! Note that the values returned by this function define the (default) orientation
    //! of the component.
    virtual wxPoint getGridNodePosition(unsigned int nodeIdx) const = 0;

    //! @overload
    //! Returns the grid position for the given node. If this device is
    //! not attached to it, then the function returns ::svInvalidPoint.
    virtual wxPoint getGridNodePosition(const svNode& node) const
        { 
            for (size_t i=0; i<m_nodes.size(); i++)
                if (m_nodes[i] == node)
                    return getGridNodePosition(i);
            return svInvalidPoint; 
        }

    // NOTE: first node (the reference node must always be the topmost node!)

    virtual wxPoint getLeftmostGridNodePosition() const = 0;
    virtual wxPoint getRightmostGridNodePosition() const = 0;
    virtual wxPoint getBottommostGridNodePosition() const = 0;

    //! Returns a value between 0 and 1 which indicates the caller the "predisposition" of this device
    //! to be drawn with the given rotation.
    //! A return value of zero means that it cannot be drawn rotated.
    //! A return value of one means that given rotation is ok.
    //! If the device returns the same value for multiple rotation values, it means that it's
    //! equally ok to draw the device in those rotation values.
    virtual double getRotationPredisposition(svRotation rot) const
        { return 1; }
};

//! A SPICE subcircuit.
class svSubckt
{
    std::string m_name;
    std::set<svNode> m_nodes;
    std::vector<svBaseDevice*> m_devices;

public:
    svSubckt(const std::string& name = "") 
        { m_name = name; }

    svSubckt(const svSubckt& tocopy) 
    {
        m_name = tocopy.m_name;
        m_nodes = tocopy.m_nodes;
        for (size_t i=0; i<tocopy.m_devices.size(); i++)
            m_devices.push_back(tocopy.m_devices[i]->clone());
    }

    ~svSubckt()
    {
        for (size_t i=0; i<m_devices.size(); i++)
            delete m_devices[i];
        m_devices.clear();
    }

    //! Adds a node to this subcircuit (unless a node with the same name already exists!).
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

    void setName(const std::string& name)
        { m_name = name; }
    std::string getName() const
        { return m_name; }

    //! FIXME
    svUGraph buildGraph() const;

    //! FIXME
    svSchematic buildSchematic() const;
    void draw(wxDC& dc) const;
};

//! The parser class for SPICE netlists.
class svParser
{
    bool loadSubCkt(svSubckt& subckt, const wxArrayString& lines, 
                    size_t startIdx, size_t endIdx);

public:
    svParser() {}
    
    //! Loads a SPICE netlist and returns the array of parsed subcircuits.
    bool load(svSubcktArray& ret, const std::string& filename);
};


// ----------------------------------------------------------------------------
// passive devices
// ----------------------------------------------------------------------------

//! Abstract class which represents resistors, capacitors, inductors.
class svPassiveDevice : public svBaseDevice
{
    //! The characteristic value of this device (e.g. resistance or capacitance).
    double m_value;

    //! The initial condition for this device.
    double m_ic;

    //! The model name for this device.
    std::string m_modelName;

public:
    svPassiveDevice()
    {
        m_value = m_ic = 0;
    }

    unsigned int getNodesCount() const { return 2; }

    wxPoint getGridNodePosition(unsigned int nodeIdx) const
    {
        // default orientation: vertical
        wxASSERT(nodeIdx >= 0 && nodeIdx <= 1);
        if (nodeIdx == 0) return wxPoint(0,0);
        if (nodeIdx == 1) return wxPoint(0,1);
        return svInvalidPoint;
    }

    wxPoint getLeftmostGridNodePosition() const { return wxPoint(0,0); }
    wxPoint getRightmostGridNodePosition() const { return wxPoint(0,0); }
    wxPoint getBottommostGridNodePosition() const { return wxPoint(0,1); }

    // SPICE line for such kind of devices is something like:
    //   L|C|R{name} {+node} {-node} [model] {value} [IC={initial}]  
    bool parseProperty(unsigned int j, const std::string& prop)
    {
        wxString strtemp;
        svString str(prop);
        double temp;

        if (str.getValue(&temp))
        {
            // this must be the {value} property...
            m_value = temp;
        }
        else if (str.Upper().StartsWith("IC=", &strtemp))
        {
            // this must be the initial condition property...
            if (!svString(strtemp).getValue(&temp))
            {
                wxLogError("Invalid initial condition for %s: %s", getHumanReadableDesc(), strtemp);
                return false;
            }

            m_ic = temp;
        }
        else if (j == 0)        // is this the first property after the node list?
        {
            m_modelName = prop;
        }
        else
        {
            wxLogError("Invalid value for %s: %s", getHumanReadableDesc(), prop);
            return false;
        }

        return true;
    }
};

class svCapacitor : public svPassiveDevice
{
public:
    svCapacitor() {}

    char getSpiceIdentifier() const { return 'C'; }
    std::string getHumanReadableDesc() const { return "CAPACITOR"; }
    svBaseDevice* clone() const { return new svCapacitor(*this); }
};

class svResistor : public svPassiveDevice
{
public:
    svResistor() {}

    char getSpiceIdentifier() const { return 'R'; }
    std::string getHumanReadableDesc() const { return "RESISTOR"; }
    svBaseDevice* clone() const { return new svResistor(*this); }

    void draw(wxDC& dc, const wxPoint& firstNodeGridPosition, unsigned int gridSpacing) const
    {
        wxPoint firstNodePos = firstNodeGridPosition*gridSpacing;
        wxPoint secondNodePos = (firstNodeGridPosition + getGridNodePosition(1))*gridSpacing;
        int w = gridSpacing/7, l = gridSpacing/7;

        dc.DrawLine(firstNodePos, firstNodePos + wxPoint(0, l));
        dc.DrawLine(firstNodePos + wxPoint(0, l), firstNodePos + wxPoint(w, 2*l));
        dc.DrawLine(firstNodePos + wxPoint(w, 2*l), firstNodePos + wxPoint(-w, 3*l));
        dc.DrawLine(firstNodePos + wxPoint(-w, 3*l), firstNodePos + wxPoint(w, 4*l));
        dc.DrawLine(firstNodePos + wxPoint(w, 4*l), firstNodePos + wxPoint(-w, 5*l));
        dc.DrawLine(firstNodePos + wxPoint(-w, 5*l), firstNodePos + wxPoint(0, 6*l));
        dc.DrawLine(firstNodePos + wxPoint(0, 6*l), firstNodePos + wxPoint(0, 7*l));
        dc.DrawText(getSpiceIdentifier() + getName(), firstNodePos + wxPoint(2*w, 3*l));
    }
};

class svInductor : public svPassiveDevice
{
public:
    svInductor() {}

    char getSpiceIdentifier() const { return 'L'; }
    std::string getHumanReadableDesc() const { return "INDUCTOR"; }
    svBaseDevice* clone() const { return new svInductor(*this); }
};

class svDiode : public svPassiveDevice
{
public:
    svDiode() {}

    char getSpiceIdentifier() const { return 'D'; }
    std::string getHumanReadableDesc() const { return "DIODE"; }
    svBaseDevice* clone() const { return new svDiode(*this); }
};

// ----------------------------------------------------------------------------
// transistors
// ----------------------------------------------------------------------------

//! Abstract class which represents MOS, BJTs, JFETs.
class svTransistorDevice : public svBaseDevice
{
    //! The model name for this device.
    std::string m_modelName;

public:
    svTransistorDevice()
    {
    }

    unsigned int getNodesCount() const { return 3; }

    wxPoint getGridNodePosition(unsigned int nodeIdx) const
    {
        // NOTE:
        // node 0 == drain/collector
        // node 1 == gate/base
        // node 2 == source/emitter

        // default orientation: vertical
        wxASSERT(nodeIdx >= 0 && nodeIdx <= 2);
        if (nodeIdx == 0) return wxPoint(0,0);
        if (nodeIdx == 1) return wxPoint(-1,1);
        if (nodeIdx == 2) return wxPoint(0,2);
        return svInvalidPoint;
    }

    wxPoint getLeftmostGridNodePosition() const { return wxPoint(-1,1); }
    wxPoint getRightmostGridNodePosition() const { return wxPoint(0,0); }
    wxPoint getBottommostGridNodePosition() const { return wxPoint(0,2); }

    // SPICE line for such kind of devices is something like:
    //    J{name} {d} {g} {s} {model} [{area]} 
    //    M{name} {d} {g} {s} {sub} {mdl}  [L={value}]  [W={value}] 
    //    Q{name} {c} {b} {e} [{subs}] {model} [{area}]  
    bool parseProperty(unsigned int j, const std::string& prop)
    {
        m_modelName = prop;
        return true;
    }
};

class svMOS : public svTransistorDevice
{
public:
    svMOS() {}

    char getSpiceIdentifier() const { return 'M'; }
    std::string getHumanReadableDesc() const { return "MOSFET"; }
    svBaseDevice* clone() const { return new svMOS(*this); }
};

class svBJT : public svTransistorDevice
{
public:
    svBJT() {}

    char getSpiceIdentifier() const { return 'Q'; }
    std::string getHumanReadableDesc() const { return "BJT"; }
    svBaseDevice* clone() const { return new svBJT(*this); }
};

class svJFET : public svTransistorDevice
{
public:
    svJFET() {}

    char getSpiceIdentifier() const { return 'J'; }
    std::string getHumanReadableDesc() const { return "JFET"; }
    svBaseDevice* clone() const { return new svJFET(*this); }
};

/*
class svDiode : public svBaseDevice
{
    std::string node1, node2, modelname;

public:
    svDiode(){}
    void set(std::string& d, std::string& g, std::string& s, std::string& b, std::string& mname){}// Not used here
    void set(std::string& n1, std::string& n2, std::string& mname); 
    void printall(void);
};*/

// ----------------------------------------------------------------------------
// independent sources
// ----------------------------------------------------------------------------

class svSourceDevice : public svBaseDevice
{
public:
    unsigned int getNodesCount() const { return 2; }

    wxPoint getGridNodePosition(unsigned int nodeIdx) const
    {
        // NOTE:
        // node 0 == plus (or the current output)
        // node 1 == minus (or the current input)

        // default orientation: vertical
        wxASSERT(nodeIdx >= 0 && nodeIdx <= 1);
        if (nodeIdx == 0) return wxPoint(0,0);
        if (nodeIdx == 1) return wxPoint(0,1);
        return svInvalidPoint;
    }

    wxPoint getLeftmostGridNodePosition() const { return wxPoint(0,0); }
    wxPoint getRightmostGridNodePosition() const { return wxPoint(0,0); }
    wxPoint getBottommostGridNodePosition() const { return wxPoint(0,1); }
};

/*
   EXPONENTIAL
	EXP( {v1} {v2} {trise_delay} {tau_rise} {tfall_delay} {tau_fall) )
   PULSE
	PULSE( {v1} {v2} {tdelay} {trise} {tfall} {width} {period} )
   PIECE WISE LINEAR
	PWL( {time1} {v1} {time2} {v2} ... {time3} {v3} )
   SINGLE FREQUENCY FM
	SFFM( {voffset} {vpeak} {fcarrier} {mod_index} {fsignal} )
   SINE WAVE
	SIN( {voffset} {vpeak} {freq} {tdelay} {damp_factor} {phase} )
*/
class svIndipendentSource : public svSourceDevice
{
    double m_value;

public:
    svIndipendentSource() {}

    unsigned int getNodesCount() const { return 2; }

    // SPICE line for such kind of devices is something like:
    // I|V{name} {+node} {-node} [[DC] {value}] [AC {mag} [{phase}]]
    bool parseProperty(unsigned int j, const std::string& prop)
    {
        wxString strtemp;
        svString str(prop);
        double temp;

        if (str.getValue(&temp))
        {
            // this must be the {value} property...
            m_value = temp;
        }
        else if (str.Upper().StartsWith("DC=", &strtemp))
        {
            // this must be the DC value property...
            if (!svString(strtemp).getValue(&temp))
            {
                wxLogError("Invalid initial condition for %s: %s", getHumanReadableDesc(), strtemp);
                return false;
            }

            m_value = temp;
        }
        else
        {
            wxLogError("Invalid value for %s: %s", getHumanReadableDesc(), prop);
            return false;
        }

        return true;
    }
};

class svISource : public svIndipendentSource
{
public:
    svISource() {}

    char getSpiceIdentifier() const { return 'I'; }
    std::string getHumanReadableDesc() const { return "CURRENT SOURCE"; }
    svBaseDevice* clone() const { return new svISource(*this); }
};

class svVSource : public svIndipendentSource
{
public:
    svVSource() {}

    char getSpiceIdentifier() const { return 'V'; }
    std::string getHumanReadableDesc() const { return "VOLTAGE SOURCE"; }
    svBaseDevice* clone() const { return new svVSource(*this); }

    void draw(wxDC& dc, const wxPoint& firstNodeGridPosition, unsigned int gridSpacing) const
    {
        wxPoint firstNodePos = firstNodeGridPosition*gridSpacing;
        wxPoint secondNodePos = (firstNodeGridPosition + getGridNodePosition(1))*gridSpacing;

        int r = gridSpacing/4;
        dc.DrawLine(firstNodePos, secondNodePos);
        dc.DrawCircle((firstNodePos+secondNodePos)/2, r);
        dc.DrawText(getSpiceIdentifier() + getName(), (firstNodePos+secondNodePos)/2 - 0.7*wxPoint(r,r));
    }
};

// ----------------------------------------------------------------------------
// controlled sources
// ----------------------------------------------------------------------------
/*
E device - Voltage Controlled Voltage Source  VCVS.
   E{name} {+node} {-node} {+cntrl} {-cntrl} {gain} 
   E{name} {+node} {-node} POLY({value}) {{+cntrl} {-cntrl}}* {{coeff}}* 
	Examples: 
	EBUFF   1   2  10  11  1.0 
	EAMP   13   0  POLY(1)  26  0  500 

G device - Voltage Controlled Current Source  VCCS.  
	G{name} {+node} {-node} {+control} {-control} {gain} 
	Examples: 	
	GBUFF   1   2  10  11  1.0


VALUE
   E|G{name} {+node} {-node} VALUE {expression}
	Examples:
	GMULT 1 0 VALUE = { V(3)*V(5,6)*100 }
	ERES 1 3 VALUE = { I(VSENSE)*10K }

TABLE
   E|G{name} {+node} {-node} TABLE {expression} = (invalue, outvalue)*
	Examples:
	ECOMP 3 0 TABLE {V(1,2)} = (-1MV 0V) (1MV, 10V)
LAPLACE
   E|G{name} {+node} {-node} LAPLACE {expression} {s expression} 
	Examples:
	ELOPASS 4 0 LAPLACE {V(1)} {10 / (s/6800 + 1)}
FREQ
   E|G{name} {+node} {-node} FREQ  {expression} (freq, gain, phase)*
	Examples:
	EAMP 5 0 FREQ {V(1)} (1KZ, 10DB, 0DEG) (10KHZ, 0DB, -90DEG)
POLY
 E|G{name} {+node} {-node} POLY(dim) {inputs X} {coeff k0,k1,...} [IC=value]
	Examples:
	EAMP 3 0 POLY(1) (2,0)  0 500 
	EMULT2 3 0 POLY(2) (1,0) (2,0)  0 0 0 0 1
	ESUM3 6 0 POLY(3) (3,0) (4,0) (5,0)  0 1.2 0.5 1.2
     COEFFICIENTS
	POLY(1) 
	y = k0  +  k1*X1  +  k2*X1*X1  +   k3*X1*X1*X1  + ...
	POLY(2)
	y = k0    + k1*X1        +  k2*X2       + 
	          + k3*X1*X1     +  k4*X2*X1    +  k5*X2*X2    + 
		  + k6*X1*X1*X1  +  k7*X2*X1*X1 +  k8*X2*X2*X1 + 
		  + k9*X2*X2*X2  +  ...
	POLY(3)
	y = k0    + k1*X1     +  k2*X2     +  k3*X3    + 
		  + k4*X1*X1  +  k5*X2*X1  +  k6*X3*X1 + 
		  + k7*X2*X2  +  k8*X2*X3  +  k9*X3*X3 + ...

F device - Current Controlled Current Source  CCCS.  
	F{name} {+node} {-node} {vsource name} {gain}  
	Examples: 
	FSENSE  1   2  VSENSE  10.0 

H device - Current Controlled Voltage Source CCVS.  
   H{name} {+node} {-node} {vsource name} {gain} 
   H{name} {+node} {-node} POLY({value}) { {vsource name} }* {{coeff}}*

*/
class svVoltageControlledSource : public svSourceDevice
{
    double m_gain;
    std::string m_value;
    std::string m_ctrlNode1;
    std::string m_ctrlNode2;

public:
    svVoltageControlledSource() {}

    unsigned int getNodesCount() const { return 2; }

    // SPICE line for such kind of devices is something like:
    //  E|G{name} {+node} {-node} {+cntrl} {-cntrl} {gain} 
    //  E|G{name} {+node} {-node} VALUE {expression}
    //  E|G{name} {+node} {-node} TABLE {expression} = (invalue, outvalue)*
    //  E|G{name} {+node} {-node} LAPLACE {expression} {s expression} 
    //  E|G{name} {+node} {-node} FREQ  {expression} (freq, gain, phase)*
    //  E|G{name} {+node} {-node} POLY(dim) {inputs X} {coeff k0,k1,...} [IC=value]
    bool parseProperty(unsigned int j, const std::string& prop)
    {
        wxString strtemp;
        svString str(prop);

        if (!m_value.empty())
            return true;       // discard every other property: this is a VALUE-defined source

        if (str.Upper().StartsWith("VALUE", &strtemp))
        {
            // this must be the {value} property...
            m_value = strtemp;
        }
        else if (j == 0)
        {
            m_ctrlNode1 = prop;
        }
        else if (j == 1)
        {
            m_ctrlNode2 = prop;
        }
        else if (j == 2 && !m_ctrlNode1.empty() && !m_ctrlNode2.empty())
        {
            if (!str.getValue(&m_gain))
                return false;
        }
        else
        {
            wxLogError("Invalid value for %s: %s", getHumanReadableDesc(), prop);
            return false;
        }

        return true;
    }
};

class svESource : public svVoltageControlledSource
{
public:
    svESource() {}

    char getSpiceIdentifier() const { return 'E'; }
    std::string getHumanReadableDesc() const { return "VOLTAGE-CONTROLLED VOLTAGE SOURCE"; }
    svBaseDevice* clone() const { return new svESource(); }
};

class svGSource : public svVoltageControlledSource
{
public:
    svGSource() {}

    char getSpiceIdentifier() const { return 'G'; }
    std::string getHumanReadableDesc() const { return "VOLTAGE-CONTROLLED CURRENT SOURCE"; }
    svBaseDevice* clone() const { return new svGSource(); }
};


// ----------------------------------------------------------------------------
// device factory
// ----------------------------------------------------------------------------

class svDeviceFactory
{
    static svBaseDeviceArray s_registered;

public:
    static void registerDevice(svBaseDevice* dev)
        { s_registered.push_back(dev); }

    //! Returns a device with an identifier matching the given character.
    //! Note that the ownership of the pointer is transferred to the caller.
    static svBaseDevice* getDeviceMatchingIdentifier(char dev)
        {
            // IMPORTANT: never return a pointer to our internal database;
            //            always return only clones
            for (size_t i=0; i<s_registered.size(); i++)
                if (s_registered[i]->getSpiceIdentifier() == dev)
                    return s_registered[i]->clone();
            return NULL;
        }

    static void registerAllDevices()
        {
            svDeviceFactory::registerDevice(new svCapacitor);
            svDeviceFactory::registerDevice(new svResistor);
            svDeviceFactory::registerDevice(new svInductor);
            svDeviceFactory::registerDevice(new svDiode);
            svDeviceFactory::registerDevice(new svISource);
            svDeviceFactory::registerDevice(new svVSource);
            svDeviceFactory::registerDevice(new svMOS);
            svDeviceFactory::registerDevice(new svBJT);
            svDeviceFactory::registerDevice(new svJFET);
            svDeviceFactory::registerDevice(new svGSource);
            svDeviceFactory::registerDevice(new svESource);
        }

    static void unregisterAllDevices()
        {
            for (size_t i=0; i<s_registered.size(); i++)
                delete s_registered[i];
            s_registered.clear();
        }
};

#endif      // _NETLIST_H_
