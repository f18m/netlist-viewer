/////////////////////////////////////////////////////////////////////////////
// Name:        schematic.h
// Purpose:     schematic representation structures
// Author:      Francesco Montorsi
// Created:     30/05/2010
// RCS-ID:      $Id$
// Copyright:   (c) 2010 Francesco Montorsi
// Licence:     GPL licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _SCHEMATIC_H_
#define _SCHEMATIC_H_

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
 
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <set>


// ----------------------------------------------------------------------------
// helper classes
// ----------------------------------------------------------------------------

class svGrid 
{
public:
    svGrid() {}



};

class svSchematic 
{
    //std::string m_name;
    //std::vector<svBaseDevice*> m_devices;

public:
    svSchematic() {}

    void draw(wxDC& dc) const;
};


#endif      // _SCHEMATIC_H_
