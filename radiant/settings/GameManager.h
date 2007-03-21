#ifndef GAMEMANAGER_H_
#define GAMEMANAGER_H_

#include <string>
#include <map>

#include "iregistry.h"
#include "Game.h"

namespace game {

/** greebo: The Manager class for keeping track
 * 			of the possible games and the current game.
 */
class Manager :
	public RegistryKeyObserver // Observes the engine path
{
public:
	// The map containing the named Game objects 
	typedef std::map<std::string, GamePtr> GameMap;

private:	
	GameMap _games;
	
	std::string _currentGameType;
	
	// The current engine path
	std::string _enginePath;
	
	bool _enginePathInitialised;
	
public:
	Manager();

	/** greebo: RegistryKeyObserver implementation, gets notified 
	 * 			upon engine path changes.
	 */
	void keyChanged();
	
	/** greebo: Gets/sets the engine path.
	 * 			setEnginePath() triggers a VFS refresh
	 */
	void setEnginePath(const std::string& path);
	std::string getEnginePath() const;

	/** greebo: Initialises the engine path from the settings in the registry.
	 * 			If nothing is found, the game file is queried.
	 */
	void initEnginePath();
	
	/** greebo: Returns the current Game (shared_ptr).
	 */
	GamePtr currentGame();
	
	/** greebo: Returns the type of the currently active game.
	 * 			This is a convenience method to be used when loading
	 * 			modules that require a game type like "doom3".
	 */
	const char* getCurrentGameType();
	
	/** greebo: Loads the game files and the saved settings.
	 * 			If no saved game setting is found, the user
	 * 			is asked to enter the relevant information in a Dialog. 
	 */
	void initialise();
	
	/** greebo: Scans the "games/" subfolder for .game description foles.
	 */
	void loadGameFiles();
	
	// Accessor method containing the static instance
	static Manager& Instance();
};

} // namespace game

#endif /*GAMEMANAGER_H_*/
