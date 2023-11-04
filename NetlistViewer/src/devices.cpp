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

// template specializations:
template<> void svDeviceFactory::registerAllDevicesForSerialization<boost::archive::text_iarchive>(boost::archive::text_iarchive&);
template<> void svDeviceFactory::registerAllDevicesForSerialization<boost::archive::text_oarchive>(boost::archive::text_oarchive&);

svBaseDeviceArray svDeviceFactory::s_registered;
wxGraphicsPath svExternalPin::s_path;
wxGraphicsPath svCapacitor::s_path;
wxGraphicsPath svResistor::s_path;
wxGraphicsPath svInductor::s_path;
wxGraphicsPath svDiode::s_path;
wxGraphicsPath svMOS::s_path;
wxGraphicsPath svMOS::s_pathArrow;
wxGraphicsPath svBJT::s_path;
wxGraphicsPath svBJT::s_pathArrow;
wxGraphicsPath svJFET::s_path;
wxGraphicsPath svJFET::s_pathArrow;
wxGraphicsPath svSource::s_pathIndipendent;
wxGraphicsPath svSource::s_pathDipendent;
wxGraphicsPath svSource::s_pathCurrentArrow;
wxGraphicsPath svSource::s_pathVoltageSigns;
wxGraphicsPath svCircuit::s_pathGround;


// ----------------------------------------------------------------------------
// device factory
// ----------------------------------------------------------------------------

void svDeviceFactory::registerAllDevices()
{
    svDeviceFactory::registerDevice(new svExternalPin);
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

void svDeviceFactory::initGraphics(wxGraphicsContext* gc, unsigned int gridSpacing)
{
    svExternalPin::initGraphics(gc, gridSpacing);
    svCapacitor::initGraphics(gc, gridSpacing);
    svResistor::initGraphics(gc, gridSpacing);
    svInductor::initGraphics(gc, gridSpacing);
    svDiode::initGraphics(gc, gridSpacing);
    svMOS::initGraphics(gc, gridSpacing);
    svBJT::initGraphics(gc, gridSpacing);
    svJFET::initGraphics(gc, gridSpacing);
    svISource::initGraphics(gc, gridSpacing);
    svVSource::initGraphics(gc, gridSpacing);
    svGSource::initGraphics(gc, gridSpacing);
    svESource::initGraphics(gc, gridSpacing);
    svCircuit::initGraphics(gc, gridSpacing);
}

void svDeviceFactory::releaseGraphics()
{
    svExternalPin::releaseGraphics();
    svCapacitor::releaseGraphics();
    svResistor::releaseGraphics();
    svInductor::releaseGraphics();
    svDiode::releaseGraphics();
    svMOS::releaseGraphics();
    svBJT::releaseGraphics();
    svJFET::releaseGraphics();
    svISource::releaseGraphics();
    svVSource::releaseGraphics();
    svGSource::releaseGraphics();
    svESource::releaseGraphics();
    svCircuit::releaseGraphics();
}

void svDeviceFactory::unregisterAllDevices()
{
    for (size_t i = 0; i < s_registered.size(); i++)
        delete s_registered[i];
    s_registered.clear();
}

svBaseDevice* svDeviceFactory::getDeviceMatchingIdentifier(char dev)
{
    // IMPORTANT: never return a pointer to our internal database;
    //            always return only clones
    for (size_t i = 0; i < s_registered.size(); i++)
        if (s_registered[i]->getSPICEid() == dev)
            return s_registered[i]->clone();
    return NULL;
}
