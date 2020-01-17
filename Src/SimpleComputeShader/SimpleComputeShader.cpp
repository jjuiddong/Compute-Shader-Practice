//
// Simple Compute Shader
//
// reference
//		https://www.codeproject.com/Articles/42612/DirectX-Compute-Shaders
//
#include "../../../Common/Common/common.h"
#include "../../../Common/Graphic11/graphic11.h"
#include "../../../Common/Framework11/framework11.h"
using common::StrId;
using common::StrPath;
using common::Vector4;
using common::Vector3;
using common::Vector2;
using common::sRectf;

using namespace graphic;
using namespace framework;

class cViewer : public framework::cGameMain2
{
public:
	cViewer();
	virtual ~cViewer();
	virtual bool OnInit() override;
	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;
	virtual void OnEventProc(const sf::Event &evt) override;

public:
	graphic::cComputeShader m_computeShader;
	graphic::cResourceBuffer m_buf1;
	graphic::cResourceBuffer m_buf2;
	graphic::cResourceBuffer m_resultBuf;
	graphic::cResourceBuffer m_copyBuf;
};

INIT_FRAMEWORK3(cViewer);

struct BufType
{
	int i;
	float f;

	void SetValue(int i, float f)
	{
		this->i = i;
		this->f = f;
	}
};

::std::vector<BufType> g_vBuf0, g_vBuf1;
const int NUM_ELEMENTS = 100;


cViewer::cViewer()
{
	graphic::cResourceManager::Get()->SetMediaDirectory("./media/");

	m_windowName = L"Simple Compute Shader";
	m_isLazyMode = true;
	const RECT r = { 0, 0, 1024, 768 };
	m_windowRect = r;
}

cViewer::~cViewer()
{
}


bool cViewer::OnInit()
{
	const float WINSIZE_X = float(m_windowRect.right - m_windowRect.left);
	const float WINSIZE_Y = float(m_windowRect.bottom - m_windowRect.top);
	GetMainCamera().SetCamera(Vector3(30, 30, -30), Vector3(0, 0, 0), Vector3(0, 1, 0));
	GetMainCamera().SetProjection(MATH_PI / 4.f, (float)WINSIZE_X / (float)WINSIZE_Y, 0.1f, 10000.0f);
	GetMainCamera().SetViewPort(WINSIZE_X, WINSIZE_Y);

	m_camera.SetCamera(Vector3(-3, 10, -10), Vector3(0, 0, 0), Vector3(0, 1, 0));
	m_camera.SetProjection(MATH_PI / 4.f, (float)WINSIZE_X / (float)WINSIZE_Y, 1.0f, 10000.f);
	m_camera.SetViewPort(WINSIZE_X, WINSIZE_Y);

	GetMainLight().Init(cLight::LIGHT_DIRECTIONAL,
		Vector4(0.3f, 0.3f, 0.3f, 1), Vector4(0.9f, 0.9f, 0.9f, 1),
		Vector4(0.2f, 0.2f, 0.2f, 1));
	const Vector3 lightPos(-300, 300, -300);
	const Vector3 lightLookat(0, 0, 0);
	GetMainLight().SetPosition(lightPos);
	GetMainLight().SetDirection((lightLookat - lightPos).Normal());
	m_gui.SetContext();
	m_gui.SetContext();
	m_gui.SetStyleColorsDark();


	// initialize compute shader
	cRenderer &renderer = m_renderer;
	bool result = m_computeShader.Create(renderer, "./BasicCompute11.cso");
	g_vBuf0.resize(NUM_ELEMENTS);
	g_vBuf1.resize(NUM_ELEMENTS);
	for (int i = 0; i < NUM_ELEMENTS; ++i)
	{
		g_vBuf0[i].SetValue(i, (float)i);
		g_vBuf1[i].SetValue(i, (float)i);
	}
	m_buf1.Create(renderer, &g_vBuf0[0], sizeof(BufType), (UINT)g_vBuf0.size(), true);
	m_buf2.Create(renderer, &g_vBuf1[0], sizeof(BufType), (UINT)g_vBuf1.size(), true);
	m_resultBuf.Create(renderer, nullptr, sizeof(BufType), (UINT)g_vBuf1.size(), false, true);
	m_copyBuf.CreateReadBuffer(renderer, m_resultBuf);

	// process compute shader
	m_computeShader.Bind(renderer);
	ID3D11ShaderResourceView* srvs[2] = { m_buf1.m_srv, m_buf2.m_srv };
	renderer.GetDevContext()->CSSetShaderResources(0, 2, srvs);
	m_resultBuf.BindUnorderedAccessView(renderer, 0);
	renderer.GetDevContext()->Dispatch(NUM_ELEMENTS, 1, 1);

	ID3D11UnorderedAccessView* ppUAViewNULL[1] = { NULL };
	renderer.GetDevContext()->CSSetUnorderedAccessViews(0, 1, ppUAViewNULL, (UINT*)(&ppUAViewNULL));
	ID3D11ShaderResourceView* ppSRVNULL[3] = { NULL, NULL, NULL };
	renderer.GetDevContext()->CSSetShaderResources(0, 3, ppSRVNULL);
	renderer.GetDevContext()->CSSetConstantBuffers(0, 0, NULL);

	m_copyBuf.CopyFrom(renderer, m_resultBuf);
	if (BufType *p = (BufType*)m_copyBuf.Lock(renderer))
	{
		FILE *fp = fopen("output.txt", "w");
		for (int i = 0; i < NUM_ELEMENTS; ++i)
			fprintf(fp, "%d %f \n", p[i].i, p[i].f);

		fclose(fp);
	}
	m_copyBuf.Unlock(renderer);
	//

	return true;
}


void cViewer::OnUpdate(const float deltaSeconds)
{
	__super::OnUpdate(deltaSeconds);
	cAutoCam cam(&m_camera);
	GetMainCamera().Update(deltaSeconds);
}


void cViewer::OnRender(const float deltaSeconds)
{
}


void cViewer::OnEventProc(const sf::Event &evt)
{
	ImGuiIO& io = ImGui::GetIO();
	switch (evt.type)
	{
	case sf::Event::KeyPressed:
		switch (evt.key.cmd)
		{
		case sf::Keyboard::Escape: close(); break;
		}
		break;
	}
}
