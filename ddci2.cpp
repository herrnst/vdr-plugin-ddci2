/////////////////////////////////////////////////////////////////////////////
//
// @file ddci.cpp @brief Digital Devices Common Interface plugin for VDR.
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

#include "ddciadapter.h"
#include "logging.h"

#include <vdr/plugin.h>

#include <getopt.h>
#include <string.h>

static const char *VERSION = "0.0.4";
static const char *DESCRIPTION = "External Digital Devices CI-Adapter";

static const char *DEV_DVB_CI = "ci";

int LogLevel;

/**
 * This class implements the interface to the CAM device.
 */
class PluginDdci: public cPlugin
{
private:
	DdCiAdapter *adapters[ MAXDEVICES ];

	/* tupples of numbers "A C", where A is the adapter number and C the CI device number
	 * for the device directory structure "/dev/dvb/adapterA/ciC"
	 */
	cStringList dd_ci_names;

	void Cleanup();
	bool FindDdCi();  // fill dd_ci_names; returns TRUE, if one or more ci devices are found
	bool GetDdCi(int &adapter, int &ci);  // get the next tupple from list

public:
	PluginDdci();
	virtual ~PluginDdci();

	/* see file plugin.h in the VDR include directory for the description of
	 * the following functions
	 */
	virtual const char *Version();
	virtual const char *Description();
	virtual const char *CommandLineHelp();
	virtual bool ProcessArgs(int argc, char *argv[]);

	// virtual bool Initialize();  	  // currently not used
	virtual bool Start();
	virtual void Stop();
	// virtual void Housekeeping();   // currently not used
	// virtual void MainThreadHook(); // currently not used
};

//------------------------------------------------------------------------

static inline bool DirentIsName(struct dirent *d, const char *name)
{
	return strstr(d->d_name, DEV_DVB_ADAPTER) == d->d_name;
}

//------------------------------------------------------------------------

static inline int DirentGetNameNum(struct dirent *d, int offset)
{
	return strtol(d->d_name + offset, NULL, 10);
}

//------------------------------------------------------------------------

static inline cString CiDevName(const char *name, int adapter, int ci)
{
	return cString::sprintf("%s/%s%d/%s%d", DEV_DVB_BASE, DEV_DVB_ADAPTER, adapter, name, ci);
}

//------------------------------------------------------------------------

static int CiDevOpen(const char *name, int adapter, int ci, int mode)
{
	cString fname(CiDevName(name, adapter, ci));
	int fd = open(fname, mode);
	if (fd < 0)
		L_ERROR_STR(*fname);
	return fd;
}

//------------------------------------------------------------------------

void PluginDdci::Cleanup()
{
	for (int i = 0; i < MAXDEVICES; i++) {
		delete adapters[ i ];
		adapters[ i ] = NULL;
	}
}

//------------------------------------------------------------------------

bool PluginDdci::FindDdCi()
{
	cReadDir dvbdir(DEV_DVB_BASE);
	if (dvbdir.Ok()) {
		dirent *a;
		while ((a = dvbdir.Next()) != NULL) {
			if (DirentIsName(a, DEV_DVB_ADAPTER)) {
				int adapter = DirentGetNameNum(a, strlen(DEV_DVB_ADAPTER));
				cReadDir adapterdir(AddDirectory(DEV_DVB_BASE, a->d_name));
				if (adapterdir.Ok()) {
					cReadDir adapterdir2(adapterdir);
					struct dirent *f;
					int ci = -1;
					while ((f = adapterdir.Next()) != NULL) {
						if (DirentIsName(f, DEV_DVB_CI)) {
							ci = DirentGetNameNum(f, strlen(DEV_DVB_CI));

							// there must be no frontend device!
							struct dirent *f2;
							while ((f2 = adapterdir2.Next()) != NULL) {
								if (DirentIsName(f2, DEV_DVB_FRONTEND)) {
									ci = -1;
									break;
								}
							}
							// frontend found -> ignore this adapter
							if (ci == -1)
								break;

							cString fname(CiDevName(DEV_DVB_CI, adapter, ci));
							L_DBG("found DD CI adapter '%s'", *fname);

							dd_ci_names.Append(strdup(cString::sprintf("%2d %2d", adapter, ci)));
						}
					}
				}
			}
		}
	}

	int found = dd_ci_names.Size();
	if (found > 0) {
		dd_ci_names.Sort();
		L_INF("found %d DD CI adapter%s", found, found > 1 ? "s" : "");
	} else
		L_INF("no DD CI adapter found");

	return found > 0;
}

//------------------------------------------------------------------------

bool PluginDdci::GetDdCi(int &adapter, int &ci)
{
	for (int i = 0; i < dd_ci_names.Size(); i++) {
		if (2 == sscanf(dd_ci_names[ i ], "%d %d", &adapter, &ci)) {
			dd_ci_names.Remove(i);
		}
	}
	return false;
}

//------------------------------------------------------------------------

PluginDdci::PluginDdci()
{
	memset(adapters, 0x00, sizeof(adapters));
	LogLevel = LL_DEFAULT;
}

//------------------------------------------------------------------------

PluginDdci::~PluginDdci()
{
}

//------------------------------------------------------------------------

const char *PluginDdci::Version()
{
	return VERSION;
}

//------------------------------------------------------------------------

const char *PluginDdci::Description()
{
	return DESCRIPTION;
}

//------------------------------------------------------------------------

const char *PluginDdci::CommandLineHelp()
{
	return "  -l        --loglevel   0/1/2/3 log nothing/error/info/debug\n";
}

//------------------------------------------------------------------------

bool PluginDdci::ProcessArgs(int argc, char *argv[])
{
	static struct option long_options[] = {
		{ "loglevel", no_argument, NULL, 'l' },
		{ NULL, no_argument, NULL, 0 }
	};

	int c, ll;
	while ((c = getopt_long(argc, argv, "l", long_options, NULL)) != -1) {
		switch (c) {
		case 'l':
			ll = strtol(optarg, 0, 10);
			if ((ll < 0) || (ll > 3)) {
				#define LSTR "Invalid Loglevel entered"
				L_ERR(LSTR);
				fprintf(stderr, LSTR "\n");
				#undef LSTR
				return false;
			} else {
				LogLevel = ll;
			}
			break;
		default:
			return false;
		}
	}
	return true;
}

//------------------------------------------------------------------------

bool PluginDdci::Start()
{
	L_INF("plugin version %s initializing (VDR %s)", VERSION, VDRVERSION);

	if (FindDdCi()) {
		for (int i = 0; i < cDevice::NumDevices(); i++) {
			if (cDevice *device = cDevice::GetDevice(i)) {
				int adapter, ci;
				if (GetDdCi(adapter, ci)) {
					int ca_fd = CiDevOpen(DEV_DVB_CA, adapter, ci, O_RDWR);
					int ci_fd = CiDevOpen(DEV_DVB_CI, adapter, ci, O_RDWR);
					if ((ca_fd >= 0) && (ci_fd >= 0)) {
						L_INF("Creating DdCiAdapter for device %d", device->CardIndex());

						cString fname(CiDevName(DEV_DVB_CA, adapter, ci));
						adapters[ i ] = new DdCiAdapter(device, ca_fd, ci_fd, fname);
					} else {
						close(ca_fd);
						close(ci_fd);
					}
				}
			}
		}
	}

	L_INF("plugin started");
	return true;
}

//------------------------------------------------------------------------

void PluginDdci::Stop()
{
	Cleanup();
	L_INF("plugin stopped");
}

VDRPLUGINCREATOR(PluginDdci); // Don't touch this!
