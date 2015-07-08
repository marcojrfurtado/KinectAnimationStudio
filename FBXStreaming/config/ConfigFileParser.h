#pragma once

#include "../common/stdafx.h"

#define LATENCY_WINDOW "Latency"
#define ENABLE_INTERLEAVING "Interleaving"
#define ENABLE_GLOBAL_TRANSFORMATION "Global"


class ConfigFileParser
{
public:
	static ConfigFileParser& getInstance()
	{
		static ConfigFileParser    instance; // Guaranteed to be destroyed.
		// Instantiated on first use.
		return instance;
	}
	
	/// <summary>
	/// Get requested parameters (LATENCY_WINDOW, ENABLE_INTERLEAVING)
	/// </summary>
	/// <return>If the requested parameter is found(key of the map), return the corresponding string. Else
	//it returns the string "ERROR"</return>
	std::string getParameter(const std::string);

private:
	
	/// <summary>
	/// Constructor
	/// </summary>
	ConfigFileParser() { LoadConfigFile(); };

	/// <summary>
	/// Load and config.txt file, inserting parameters into the map "parameters" - Interleaving, latency, AbsMarkers
	/// </summary>
	void LoadConfigFile();


	// Map to store the parameters from config.txt
	std::map<std::string, std::string> parameters;

	// C++ 11
	// =======
	// We can use the better technique of deleting the methods
	// we don't want.
	ConfigFileParser(ConfigFileParser const&) = delete;
	void operator=(ConfigFileParser const&) = delete;

};