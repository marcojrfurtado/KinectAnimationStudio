#include "ConfigFileParser.h"


/// <summary>
/// Load and config.txt file, inserting parameters into the map "parameters" - Interleaving, latency, AbsMarkers
/// </summary>
void ConfigFileParser::LoadConfigFile() {

	// create a file-reading object
	std::ifstream configFile;
	char Path[512];
	char *p;
	GetModuleFileName(NULL, Path, 512);
	p = strrchr(Path, '\\');
	strcpy_s(p + 1, strlen(p) + 1, "config.txt");
	configFile.open(Path);

	std::string line;
	while (std::getline(configFile, line))
	{
		std::istringstream iss(line);
		std::string a, b;
		if (!(iss >> a >> b)) { break; } // error

		// process pair (a,b)
		parameters[a] = b;
	}
	

	// Unable to store text file properly to map
	/*
	char key [100];
	char keyValue[100];
	FILE *configFile;
	char Path[512];
	char *p;
	GetModuleFileName(NULL, Path, 512);
	p = strrchr(Path, '\\');
	strcpy_s(p + 1, strlen(p) + 1, "config.txt");
	errno_t fResults = fopen_s(&configFile, Path, "r");

	//read file 
	if (fResults != 0)
	{
		return;
	}
	
	while (fscanf_s(configFile, " %s %[^\n] ", key, keyValue) == 2) {
		parameters[key] = keyValue;
	}



	// TODO: Add enable absolute markers parameter later
	//parameters["MarkersEnable"] = ;

	fclose(configFile);
	*/
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