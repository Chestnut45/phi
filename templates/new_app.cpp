#include "new_app.hpp"

NewApp* app = nullptr;

// Application entrypoint
int main(int, char**)
{
    app = new NewApp();
    app->Run();
    delete app;
    return 0;
}

NewApp::NewApp() : App("New App", 4, 6)
{
    // TODO: Initialization logic
}

NewApp::~NewApp()
{
    // TODO: Shutdown logic
}

void NewApp::Update(float delta)
{
    // TODO: Update logic

    // DEBUG: Show app debug window
    ShowDebug();
}

void NewApp::Render()
{
    // TODO: Render logic
}