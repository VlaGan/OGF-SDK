//----------------------------------------------------------------------------
//-- CScene
//----------------------------------------------------------------------------
#pragma once
#include <vector>
#include <string>

class CModel;

class CScene {
public:
	CScene();
	~CScene();

	CScene(const CScene&) = delete;
	CScene& operator=(const CScene&) = delete;

	static CScene& Get() {
		static CScene Scene;
		return Scene;
	}
	void ClearScene();

	CModel* LoadModel(const std::string& path);

	std::vector<CModel*>& GetModels() { return m_Models; }

	bool m_bDrawGrid{ true };
	bool m_bWireframe{ false };

	CModel* m_SelectedModel{};

private:
	std::vector<CModel*> m_Models;
};
