//----------------------------------------------------------------------------
//-- CSettings.cpp
//-- VlaGan: Global ogf-sdk settings 
//----------------------------------------------------------------------------
#include "CSettings.h"
#include <format>
#include <fstream>
#include <filesystem>
#include "CLog.h"


#define PATH_USERDATA "userdata"

//----------------------------------------------------------------------------
CSettings::CSettings() {
	m_IniFile = "settings.ini";
}

CSettings::~CSettings() { Save(); }
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//-- Save/Load settings ini file
//----------------------------------------------------------------------------
void CSettings::Save() {

	std::ofstream m_FileStream;
	std::filesystem::path iniPath = std::filesystem::path(PATH_USERDATA) / m_IniFile;

	m_FileStream.open(iniPath, std::ios::out);
	if (m_FileStream.is_open()) {
		std::string gamedata_folder = std::format("gamedata_folder={}", m_GamedataPath.c_str());
		m_FileStream << gamedata_folder << "\n";
		m_FileStream.flush();
		m_FileStream.close();
	}
}

void CSettings::ReadIni() {

	std::filesystem::path iniPath = std::filesystem::path(PATH_USERDATA) / m_IniFile;

	if (std::filesystem::exists(iniPath)) {
		LogMsg(eLogLevel::DEBUG, "CSettings::ReadIni() - Reading ini file");

		std::ifstream fileStream;
		fileStream.open(iniPath, std::ios::in);

		if (fileStream.is_open()) {
			std::string line;
			while (std::getline(fileStream, line)) {

				if (line.empty() || line[0] == ';' || line[0] == '#') {
					continue;
				}

				size_t pos = line.find('=');
				if (pos == std::string::npos) {
					continue;
				}

				// Extract key and value
				std::string key = line.substr(0, pos);
				std::string value = line.substr(pos + 1);

				// Trim whitespace from key and value
				key.erase(0, key.find_first_not_of(" \t\r\n"));
				key.erase(key.find_last_not_of(" \t\r\n") + 1);
				value.erase(0, value.find_first_not_of(" \t\r\n"));
				value.erase(value.find_last_not_of(" \t\r\n") + 1);

				//-- parse settigns
				if (key == "gamedata_folder") {
					m_GamedataPath = value;
					LogMsg(eLogLevel::DEBUG, "CSettings::ReadIni() - gamedata_folder = [%s]", m_GamedataPath.c_str());
				}
			}
			fileStream.close();
		}
	}
	else {
		std::error_code ec;
		std::filesystem::create_directories(PATH_USERDATA, ec);
		if (ec) {
			LogMsg(eLogLevel::ERR, "CSettings::ReadIni() - Failed to create userdata directory");
			return;
		}

		//-- create file
		std::ofstream m_FileStream;
		m_FileStream.open(iniPath, std::ios::out);
		if (m_FileStream.is_open())
			m_FileStream.close();

		LogMsg(eLogLevel::DEBUG, "CSettings::ReadIni() - Created ini file");
	}
}
//----------------------------------------------------------------------------


std::string CSettings::GetTexturePath(std::string texture_path) {
	return std::format("{}\\textures\\{}", m_GamedataPath, texture_path);
}

std::string CSettings::GetMeshesPath() {
	return std::format("{}\\meshes\\", m_GamedataPath);
}
