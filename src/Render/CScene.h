//----------------------------------------------------------------------------
//-- CScene
//----------------------------------------------------------------------------
#pragma once
#include <vector>
#include <string>

class CModel;

enum class eGizmoMode {
	eSelect = 0,
	eTranslate,
	eRotate,
	eScale
};

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
	CModel* LoadModelFromOGF(const std::string& path);
	void DeleteModel(CModel* model);

	std::vector<CModel*>& GetModels() { return m_Models; }

	bool m_bDrawGrid{ true };
	bool m_bWireframe{ false };
	bool m_bDrawSkeleton{ false };

	//-- Gizmo voznya
	eGizmoMode m_GizmoMode{ eGizmoMode::eSelect };

	//-- Gizmo space: true = world axes, false = local (object) axes.
	bool m_bGizmoWorldSpace{ true };

	CModel* m_SelectedModel{};

private:
	std::vector<CModel*> m_Models;
};
