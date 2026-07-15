//----------------------------------------------------------------------------
//-- CScene
//----------------------------------------------------------------------------
#include "CScene.h"
#include "Model/CModel.h"
#include "CHW.h"
#include "../_defines.h"

CScene::CScene() {

	auto model = LoadModel("appdata/models/Minori.fbx");
	if (model) {
		model->SetScale(1.0f, 1.0f, 1.0f);
		//model->SetPosition(0.f, -0.35f, -2.5f);
		model->SetRotation(deg2rad(90.f), 0.f, deg2rad(0.f));
		model->m_CurrentTime = 0.f;
		model->m_TicksPerSecond = 30.f;
	}

	auto model2 = LoadModel("appdata/models/cover.fbx");
	if (model2) {
		model2->SetScale(3.0f, 3.0f, 3.0f);
		model2->SetPosition(0.f, -0.1f, 0.0f);
		model2->SetRotation(deg2rad(90.f), 0.f, deg2rad(180.f));
		for (auto& m : model2->m_Meshes) {
			m.SetShader(CHW::Get().m_Device, L"appdata/shaders/skinned_not.hlsl");
			//m.SetShadowShader(hw.m_Device, L"appdata/shaders/shadow_map.hlsl");
		}
		model2->m_CurrentTime = 0.f;
		model2->m_TicksPerSecond = 0.f;
	}

	LogMsg(eLogLevel::DEBUG, "CRenderer::LoadScene() -> Success");
}

CScene::~CScene() {
	ClearScene();
}

void CScene::ClearScene() {
	for (CModel* model : m_Models) {
		delete model;
	}
	m_Models.clear();
}

void CScene::DeleteModel(CModel* model) {
	for (auto it = m_Models.begin(); it != m_Models.end(); it++) {
		if (model == (*it)) {
			if (m_SelectedModel == model)
				m_SelectedModel = nullptr;
			delete (*it);
			m_Models.erase(it);
			break;
		}
	}
}

CModel* CScene::LoadModel(const std::string& path) {
	CHW& hw = CHW::Get();
	CModel* model = new CModel();
	if (!model->LoadFromFile(hw.m_Device, path)) {
		LogMsg(eLogLevel::ERR, "CScene::LoadModel: Model[%s] was not loaded!", path.c_str());
		delete model;
		return nullptr;
	}
	m_Models.push_back(model);
	return model;
}
