#include "ConfigFileParser.h"


/// <summary>
/// Load and config.txt file, inserting parameters into the map "parameters" - Interleaving, latency, AbsMarkers
/// </summary>
void ConfigFileParser::LoadConfigFile() {

	// create a file-reading object
	char Path[512];
	GetLocalFile("config.txt", Path, 512);
	std::ifstream configFile(Path);
	
	// Unable to open config file
	if (!configFile.is_open())
		return;

	std::string line;
	while (std::getline(configFile, line))
	{
		std::istringstream iss(line);
		std::string a, b;
		if (!(iss >> a >> b)) { break; } // error

		// process pair (a,b)
		parameters[a] = b;
	}


	// Close file after reading it
	configFile.close();
	return;
}


/// <summary>
/// Get requested parameters (LATENCY_WINDOW, ENABLE_INTERLEAVING)
/// </summary>
/// <return>If the requested parameter is found(key of the map), return the corresponding string. Else
//it returns the string "ERROR"</return>
std::string ConfigFileParser::getParameter(const std::string param) {
	std::map<std::string, std::string>::iterator it;
	it = parameters.find(param);
	if (it == parameters.end()) {
		UI_Printf("Could not find requested parameter %s", param);
		return "ERROR";
	}
	
	return it->second;
}