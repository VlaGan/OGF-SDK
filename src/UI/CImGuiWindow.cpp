//----------------------------------------------------------------------------
//-- CImGUiWindow: Base class for ImGui windows
//----------------------------------------------------------------------------
#include "CImGuiWindow.h"

CImGuiWindow::CImGuiWindow(std::string name)
{
	m_Name = name;
	m_Opened = true;
	m_Collapsed = false;
}

CImGuiWindow::~CImGuiWindow() {}

bool CImGuiWindow::RenderBegin()
{
	m_Collapsed = !ImGui::Begin(TitleWithIcon().c_str(), &m_Opened, m_flags);
	if (!m_Opened)
		return false;
	return !m_Collapsed;
}

void CImGuiWindow::Render()
{
	if (RenderBegin())
		RenderContent();
	RenderEnd();
}

void CImGuiWindow::RenderContent() {}

void CImGuiWindow::RenderEnd()
{
	ImGui::End();
}
