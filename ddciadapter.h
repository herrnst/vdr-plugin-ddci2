/////////////////////////////////////////////////////////////////////////////
//
// @file ddciadapter.h @brief Digital Devices Common Interface plugin for VDR.
//
// Copyright (c) 2013 - 2014 by Jasmin Jessich.  All Rights Reserved.
//
// Contributor(s):
//
// License: GPLv2
//
// This file is part of vdr_plugin_ddci2.
//
// vdr_plugin_ddci2 is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// vdr_plugin_ddci2 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with vdr_plugin_ddci2.  If not, see <http://www.gnu.org/licenses/>.
//
// $Id:  $
/////////////////////////////////////////////////////////////////////////////

#ifndef __DDCIADAPTER_H
#define __DDCIADAPTER_H

#include <vdr/ci.h>

/**
 * This class implements the interface to the CAM device.
 */
class DdCiAdapter: public cCiAdapter
{
private:
	/* VDR currently allows only *one* device per CI adapter. Moreover,
	 * it is bound in a 1:1 relation to that device, just from the
	 * creation.
	 */
	cDevice *device;    //< the bound device
	int fd_ca;          //< .../frontendX/caX device file handle
	int fd_ci;          //< .../frontendX/ciX device file handle
	cString caDevName;  //< .../frontendX/caX device path

protected:
	/* see file ci.h in the VDR include directory for the description of
	 * the following functions
	 */
	virtual int Read(uint8_t *Buffer, int MaxLength);
	virtual void Write(const uint8_t *Buffer, int Length);
	virtual bool Reset(int Slot);
	virtual eModuleStatus ModuleStatus(int Slot);
	virtual bool Assign(cDevice *Device, bool Query = false);

public:
	/**
	 * Constructor.
	 * Checks for the available slots of the CAM and starts the
	 * controlling thread.
	 * @param dev the assigned device
	 * @param ca_fd the file handle for the .../frontendX/caX device
	 * @param ci_fd the file handle for the .../frontendX/ciX device
	 * @param devName the name of the device (.../frontendX/ciX)
	 **/
	DdCiAdapter(cDevice *dev, int ca_fd, int ci_fd, cString &devName);

	/// Destructor.
	virtual ~DdCiAdapter();

	// static DdCiAdapter *CreateCiAdapter(int ca_fd, int ci_fd);
};

#endif //__DDCIADAPTER_H
