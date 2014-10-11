#pragma once

#include "iselectionset.h"
#include "iradiant.h"
#include "icommandsystem.h"

#include <map>
#include "SelectionSet.h"

#include <boost/enable_shared_from_this.hpp>

#include <wx/event.h>
#include <wx/toolbar.h>

namespace selection
{

class SelectionSetToolmenu;

class SelectionSetManager :
	public ISelectionSetManager,
	public boost::enable_shared_from_this<SelectionSetManager>,
	public wxEvtHandler
{
private:
	typedef std::set<ISelectionSetManager::Observer*> Observers;
	Observers _observers;

	typedef std::map<std::string, SelectionSetPtr> SelectionSets;
	SelectionSets _selectionSets;

	SelectionSetToolmenu* _toolMenu;
	wxToolBarToolBase* _clearAllButton;

public:
	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
	void shutdownModule();

	void onRadiantStartup();

	// ISelectionManager implementation
	void addObserver(Observer& observer);
	void removeObserver(Observer& observer);

	void foreachSelectionSet(Visitor& visitor);
	void foreachSelectionSet(const VisitorFunc& functor);
	ISelectionSetPtr createSelectionSet(const std::string& name);
	void deleteSelectionSet(const std::string& name);
	void deleteAllSelectionSets();
	ISelectionSetPtr findSelectionSet(const std::string& name);

	// Command target
	void deleteAllSelectionSets(const cmd::ArgumentList& args);

private:
	void onDeleteAllSetsClicked(wxCommandEvent& ev);

	void notifyObservers();
};

} // namespace

