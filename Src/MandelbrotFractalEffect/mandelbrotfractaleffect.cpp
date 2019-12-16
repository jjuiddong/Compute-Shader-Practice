//
// Mandelbrot Fractal using Effect Shader (Compute Shader)
//
// reference
//		https://docs.microsoft.com/ko-kr/samples/microsoft/xbox-atg-samples/simplecomputepc/
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

struct sCbFractal
{
	XMVECTOR MaxThreadIter;
	XMVECTOR Window;
};

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
	graphic::cShader11 m_fractalShader; // compute shader
	graphic::cTexture m_fractalTexture;
	graphic::cConstantBuffer<sCbFractal> m_cbFractal;
	graphic::cTexture m_fractalColorMap;
	graphic::cQuad2D m_fractalQuad;
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
	GetMainCamera().SetProjection(MATH_PI / 4.f, (float)WINSIZE_X / (float)WINSIZE_Y
		, 0.1f, 10000.0f);
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

	cRenderer &renderer = m_renderer;
	sRectf rect = m_windowRect;

	bool result = false;
	result = m_fractalShader.Create(renderer, "./fractal.fxo", "Compute", 0);
	result = m_fractalTexture.Create(renderer, (int)rect.Width(), (int)rect.Height()
		, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D11_USAGE_DEFAULT, true);
	result = m_cbFractal.Create(renderer);

	const uint GradientTexels[8] = { 0xFF000040, 0xFF000080, 0xFF0000C0, 0xFF0000FF
		, 0xFF0040FF, 0xFF0080FF, 0xFF00C0FF, 0xFF00FFFF };
	result = m_fractalColorMap.Create(renderer, 8, 1
		, DXGI_FORMAT_R8G8B8A8_UNORM, GradientTexels, sizeof(GradientTexels)
		, D3D11_USAGE_DEFAULT, false);

	cTexture *tex = cResourceManager::Get()->LoadTexture(renderer, "box.dds");
	m_fractalQuad.Create(renderer, 0, 0, rect.Width(), rect.Height());
	m_fractalQuad.m_texture = &m_fractalTexture;

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
	cRenderer &renderer = m_renderer;

	m_fractalShader.SetTechnique("Compute");
	m_fractalShader.Begin();
	m_fractalShader.BeginPass(renderer, 0);

	m_cbFractal.m_v->MaxThreadIter =
		Vector4((float)m_fractalTexture.m_imageInfo.Width
			, (float)m_fractalTexture.m_imageInfo.Height
			, 300
			, 0).GetVectorXM();
	m_cbFractal.m_v->Window = Vector4(4.0f, 2.25f, -0.65f, 0.0f).GetVectorXM();
	m_cbFractal.Update(renderer, 6);
	m_fractalColorMap.Bind(renderer, 0);
	m_fractalTexture.BindUnorderedAccessView(renderer, 0);

	// make sure to update value in shader if this changes
	const uint s_numShaderThreads = 8;
	const uint threadGroupX = m_fractalTexture.m_imageInfo.Width / s_numShaderThreads;
	const uint threadGroupY = m_fractalTexture.m_imageInfo.Height / s_numShaderThreads;
	renderer.GetDevContext()->Dispatch(threadGroupX, threadGroupY, 1);
	m_fractalTexture.Unbind(renderer, 0);

	m_fractalQuad.Render(renderer);
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
