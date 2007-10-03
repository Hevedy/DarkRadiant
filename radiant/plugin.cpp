/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <iostream>
#include <gtk/gtkimage.h>
#include "plugin.h"

#include "debugging/debugging.h"

#include "brush/TexDef.h"
#include "iradiant.h"
#include "ifilesystem.h"
#include "ishaders.h"
#include "iclipper.h"
#include "igrid.h"
#include "ieventmanager.h"
#include "ientity.h"
#include "ieclass.h"
#include "igame.h"
#include "irender.h"
#include "iscenegraph.h"
#include "iselection.h"
#include "ifilter.h"
#include "isound.h"
#include "igl.h"
#include "iundo.h"
#include "ireference.h"
#include "ifiletypes.h"
#include "ipreferencesystem.h"
#include "ibrush.h"
#include "iuimanager.h"
#include "ipatch.h"
#include "iimage.h"
#include "itoolbar.h"
#include "imap.h"
#include "iparticles.h"
#include "inamespace.h"

#include "gtkutil/messagebox.h"
#include "gtkutil/filechooser.h"

#include "error.h"
#include "map.h"
#include "modulesystem/ApplicationContextImpl.h"
#include "gtkmisc.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "mainframe.h"
#include "multimon.h"
#include "camera/GlobalCamera.h"
#include "entity.h"
#include "select.h"
#include "nullmodel.h"
#include "xyview/GlobalXYWnd.h"
#include "map/AutoSaver.h"
#include "map/PointFile.h"
#include "map/CounterManager.h"

#include "modulesystem/StaticModule.h"

#include "settings/GameManager.h"

#include <boost/shared_ptr.hpp>

// TODO: Move this elsewhere
ui::ColourSchemeManager& ColourSchemes() {
	static ui::ColourSchemeManager _manager;
	return _manager;
}

class RadiantCoreAPI :
	public IRadiant
{
	map::CounterManager _counters;
	
	typedef std::set<RadiantEventListenerPtr> EventListenerList;
	EventListenerList _eventListeners;
public:
	RadiantCoreAPI() {
		globalOutputStream() << "RadiantCore initialised.\n";
	}
	
	virtual GtkWindow* getMainWindow() {
		return MainFrame_getWindow();
	}
	
	virtual GdkPixbuf* getLocalPixbuf(const std::string& fileName) {
		// Construct the full filename using the Bitmaps path
		std::string fullFileName(GlobalRegistry().get(RKEY_BITMAPS_PATH) + fileName);
		return gdk_pixbuf_new_from_file(fullFileName.c_str(), NULL);
	}
	
	virtual GdkPixbuf* getLocalPixbufWithMask(const std::string& fileName) {
		std::string fullFileName(GlobalRegistry().get(RKEY_BITMAPS_PATH) + fileName);
		
		GdkPixbuf* rgb = gdk_pixbuf_new_from_file(fullFileName.c_str(), 0);
		if (rgb != NULL) {
			// File load successful, add alpha channel
			GdkPixbuf* rgba = gdk_pixbuf_add_alpha(rgb, TRUE, 255, 0, 255);
			gdk_pixbuf_unref(rgb);
			return rgba;
		}
		else {
			// File load failed
			return NULL;
		}
	}
	
	virtual ICounter& getCounter(CounterType counter) {
		// Pass the call to the helper class
		return _counters.get(counter);
	}
	
	virtual void setStatusText(const std::string& statusText) {
		Sys_Status(statusText);
	}
	
	virtual const char* getGameDescriptionKeyValue(const char* key) {
		return GlobalGameManager().currentGame()->getKeyValue(key);
	}
	
	virtual const char* getRequiredGameDescriptionKeyValue(const char* key) {
		return GlobalGameManager().currentGame()->getRequiredKeyValue(key);
	}
	
	virtual Vector3 getColour(const std::string& colourName) {
		return ColourSchemes().getColourVector3(colourName);
	}
  
	virtual void updateAllWindows() {
		UpdateAllWindows();
	}
	
	virtual void addEventListener(RadiantEventListenerPtr listener) {
		_eventListeners.insert(listener);
	}
	
	virtual void removeEventListener(RadiantEventListenerPtr listener) {
		EventListenerList::iterator found = _eventListeners.find(listener);
		if (found != _eventListeners.end()) {
			_eventListeners.erase(found);
		}
	}
	
	// Broadcasts a "shutdown" event to all the listeners
	void broadcastShutdownEvent() {
		for (EventListenerList::iterator i = _eventListeners.begin();
		     i != _eventListeners.end(); i++)
		{
			(*i)->onRadiantShutdown();
		}
	}
	
	// Broadcasts a "startup" event to all the listeners
	void broadcastStartupEvent() {
		for (EventListenerList::iterator i = _eventListeners.begin();
		     i != _eventListeners.end(); i++)
		{
			(*i)->onRadiantStartup();
		}
	}
	
	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name(MODULE_RADIANT);
		return _name;
	}
	
	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies;
		
		// greebo: TODO: This list can probably be made smaller,
		// not all modules are necessary during initialisation
		if (_dependencies.empty()) {
			_dependencies.insert(MODULE_EVENTMANAGER);
			_dependencies.insert(MODULE_UIMANAGER);
			_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
			_dependencies.insert(MODULE_ENTITYCREATOR);
			_dependencies.insert(MODULE_SHADERSYSTEM);
			_dependencies.insert(MODULE_BRUSHCREATOR);
			_dependencies.insert(MODULE_SCENEGRAPH);
			_dependencies.insert(MODULE_SHADERCACHE);
			_dependencies.insert(MODULE_FILETYPES);
			_dependencies.insert(MODULE_SELECTIONSYSTEM);
			_dependencies.insert(MODULE_REFERENCECACHE);
			_dependencies.insert(MODULE_OPENGL);
			_dependencies.insert(MODULE_ECLASSMANAGER);
			_dependencies.insert(MODULE_UNDOSYSTEM);
			_dependencies.insert(MODULE_NAMESPACE);
			_dependencies.insert(MODULE_CLIPPER);
			_dependencies.insert(MODULE_GRID);
			_dependencies.insert(MODULE_SOUNDMANAGER);
			_dependencies.insert(MODULE_PARTICLESMANAGER);
			_dependencies.insert(MODULE_GAMEMANAGER);
			_dependencies.insert("Doom3MapLoader");
			_dependencies.insert("ImageLoaderTGA");
			_dependencies.insert("ImageLoaderJPG");
			_dependencies.insert("ImageLoaderDDS");
		}
		
		return _dependencies;
	}
	
	virtual void initialiseModule(const ApplicationContext& ctx) {
		globalOutputStream() << "RadiantAPI::initialiseModule called.\n";
		
		// Reset the node id count
	  	scene::Node::resetIds();
	  	
	    GlobalFiletypes().addType(
	    	"sound", "wav", FileTypePattern("PCM sound files", "*.wav"));

	    Selection_construct();
	    MultiMon_Construct();
	    map::PointFile::Instance().registerCommands();
	    Map_Construct();
	    MainFrame_Construct();
	    GlobalCamera().construct();
	    GlobalXYWnd().construct();
	    GlobalTextureBrowser().construct();
	    Entity_Construct();
	    map::AutoSaver().init();
	}
	
	virtual void shutdownModule() {
		globalOutputStream() << "RadiantCoreAPI::shutdownModule called.\n";
		
		GlobalFileSystem().shutdown();

		map::PointFile::Instance().destroy();
	    Entity_Destroy();
	    GlobalXYWnd().destroy();
	    GlobalCamera().destroy();
	    MainFrame_Destroy();
	    Map_Destroy();
	    MultiMon_Destroy();
	    Selection_destroy();
	}
};

// Define the static Radiant module
module::StaticModule<RadiantCoreAPI> radiantCoreModule;
