//----------------------------------------------------------------------------
//-- CScene
//----------------------------------------------------------------------------
#pragma once
#include <vector>
#include <string>
#include "../Core/CCamera.h"

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
	void DeleteModel(CModel* model);

	//-- Importing
	CModel* LoadModel(const std::string& path);
	CModel* LoadModel(const std::string& path, const std::string& ext);
	CModel* LoadModelFromOGF(const std::string& path);

	//-- Exporting
	bool CanExportModel();
	void ExportCSCoP(const std::string& path);
	void ExportSoC(const std::string& path);

	std::vector<CModel*>& GetModels() { return m_Models; }

	CCamera* Camera() { return &m_Camera; }

	bool m_bDrawGrid{ true };
	bool m_bWireframe{ false };
	bool m_bDrawSkeleton{ false };

	//-- Gizmo voznya
	eGizmoMode m_GizmoMode{ eGizmoMode::eSelect };

	//-- Gizmo space: true = world axes, false = local (object) axes.
	bool m_bGizmoWorldSpace{ true };

	CModel* m_SelectedModel{};

	//-- here will be global time factor
	float m_fTimeFactor{ 1.f };

private:
	std::vector<CModel*> m_Models;
	
	//-- scene camera  
	CCamera m_Camera;
};
