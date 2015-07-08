#pragma once

#include "../common/stdafx.h"

#define LATENCY_WINDOW "Latency"
#define ENABLE_INTERLEAVING "Interleaving"


class ConfigFileParser
{
public:
	static ConfigFileParser& getInstance()
	{
		static ConfigFileParser    instance; // Guaranteed to be destroyed.
		// Instantiated on first use.
		return instance;
	}
	
	std::string getParameter(const std::string);

private:
	ConfigFileParser() { LoadConfigFile(); };                   // Constructor? (the {} brackets) are needed here.

	/// <summary>
	/// Load and config.txt file, inserting parameters into the map "parameters" - Interleaving, latency, AbsMarkers
	/// </summary>
	void LoadConfigFile();


	std::map<std::string, std::string> parameters;

	// C++ 11
	// =======
	// We can use the better technique of deleting the methods
	// we don't want.
	ConfigFileParser(ConfigFileParser const&) = delete;
	void operator=(ConfigFileParser const&) = delete;

};