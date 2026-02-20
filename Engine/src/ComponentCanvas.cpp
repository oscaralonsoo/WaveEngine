#include "ComponentCanvas.h"
#include <glad/glad.h>
#include "GLRenderDevice.h"
#include "Application.h"
#include "Time.h"
#include "NoesisPCH.h"
#include "NsCore/Noesis.h"
#include <NsCore/RegisterComponent.h>
#include <NsCore/Package.h>
#include "NsApp/LocalFontProvider.h"
#include "NsApp/LocalXamlProvider.h"
#include "NsApp/LocalTextureProvider.h"
#include <NsApp/EventTrigger.h>
#include <NsApp/GoToStateAction.h>
#include <NsApp/InvokeCommandAction.h>
#include <NsApp/Interaction.h>
#include "NsGui/IView.h"
#include "NsGui/FrameworkElement.h"
#include "NsGui/IntegrationAPI.h"

ComponentCanvas::ComponentCanvas(GameObject* owner) : Component(owner, ComponentType::CANVAS)
{
    name = "Canvas";
    GenerateFramebuffer(width, height);
    Application::GetInstance().ui->RegisterCanvas(this);
}

ComponentCanvas::~ComponentCanvas()
{
    Application::GetInstance().ui->UnregisterCanvas(this);

    if (view)
    {
        view->GetRenderer()->Shutdown();
        view.Reset();
    }
    device.Reset();

    if (fbo) glDeleteFramebuffers(1, &fbo);
    if (textureID) glDeleteTextures(1, &textureID);
    if (rbo) glDeleteRenderbuffers(1, &rbo);
}

void ComponentCanvas::CleanUp()
{
    if (view)
    {
        view->GetRenderer()->Shutdown();
        view.Reset();
    }
    device.Reset();
}

bool ComponentCanvas::LoadXAML(const char* filename)
{
    Noesis::Ptr<Noesis::FrameworkElement> xaml =
        Noesis::GUI::LoadXaml<Noesis::FrameworkElement>(filename);

    if (!xaml) return false;

    if (view)
    {
        view->GetRenderer()->Shutdown();
        view.Reset();
    }

    view = Noesis::GUI::CreateView(xaml);
    view->SetFlags(Noesis::RenderFlags_PPAA | Noesis::RenderFlags_LCD);
    view->SetSize(width, height);

    device = Application::GetInstance().ui->GetRenderDevice();

    view->GetRenderer()->Init(device);

    currentXAML = filename;
    return true;
}

void ComponentCanvas::Update()
{
    if (!view) return;
    view->Update(Application::GetInstance().time->GetTotalTime());
}

void ComponentCanvas::RenderToTexture()
{
    if (!view) return;


    glPushAttrib(GL_ALL_ATTRIB_BITS);	//Save current OpenGL state

    GLint prevFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    view->GetRenderer()->UpdateRenderTree();
    view->GetRenderer()->RenderOffscreen();
    view->GetRenderer()->Render();

    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

	glPopAttrib(); //Restore previous OpenGL state
}

void ComponentCanvas::Resize(int newWidth, int newHeight)
{
    if (width == newWidth && height == newHeight) return;

    width = newWidth;
    height = newHeight;

    if (view) view->SetSize(width, height);

    GenerateFramebuffer(width, height);
}

void ComponentCanvas::GenerateFramebuffer(int w, int h)
{
    if (fbo) glDeleteFramebuffers(1, &fbo);
    if (textureID) glDeleteTextures(1, &textureID);
    if (rbo) glDeleteRenderbuffers(1, &rbo);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureID, 0);

    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ComponentCanvas::Serialize(nlohmann::json& componentObj) const
{
    componentObj["xamlPath"] = currentXAML;
}

void ComponentCanvas::Deserialize(const nlohmann::json& componentObj)
{
    if (componentObj.contains("xamlPath"))
    {
        std::string path = componentObj["xamlPath"];
        if (!path.empty())
        {
            LoadXAML(path.c_str());
        }
    }
}