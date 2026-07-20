//----------------------------------------------------------------------------
//-- CSettings.h
//-- VlaGan: Global ogf-sdk settings 
//----------------------------------------------------------------------------
#pragma once
#include <string>


class CSettings {
public:
	CSettings();
	~CSettings();

	//-- singleton
	CSettings(const CSettings&) = delete;
	CSettings& operator=(const CSettings&) = delete;

	static CSettings& Get() {
		static CSettings PrgSettings;
		return PrgSettings;
	}
	void Save();
	void ReadIni();
	
	bool GamedataPathInited() { return !m_GamedataPath.empty(); }
	std::string GamedataPath() { return m_GamedataPath; };

	std::string GetTexturePath(std::string texture_path);
	std::string GetMeshesPath();

	std::string m_GamedataPath{};

private:
	std::string m_IniFile;
};
