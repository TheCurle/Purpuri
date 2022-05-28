/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <vm/debug/Debug.hpp>
#include <vm/Stack.hpp>

// imgui is very particular about not allowing you to use clang with msys?
#if defined(__clang__)
    #undef __MINGW32__
#endif

#include <stdio.h>

#ifdef VISUAL_DEBUGGER
    #include <vm/debug/imgui/imgui.h>
    #include <vm/debug/imgui/imgui_memedit.h>
    #include <vm/debug/imgui/imgui_impl_sdl.h>
    #include <vm/debug/imgui/imgui_impl_opengl3.h>
    #include <vm/debug/SDL2/SDL.h>
    
    #if defined(IMGUI_IMPL_OPENGL_ES2)
        #include <vm/debug/SDL2/SDL_opengles2.h>
    #else
        #include <vm/debug/SDL2/SDL_opengl.h>
    #endif
#endif

/** Main Thread Variables **/

// Synchronization
bool Debugger::Enabled = false;
bool Debugger::ShouldStep = false;
std::thread Debugger::Thread;
std::mutex Debugger::Locker;
std::condition_variable Debugger::Notifier;
size_t Debugger::Pipe;

// Internal
Debugger::TypeFlags flags;
bool updated = false;
StackFrame* stack = nullptr;
size_t stackpointer = 0;

/** Main Thread Functions **/

void Synchronize(size_t Ptr, Debugger::TypeFlags Type) {
    std::lock_guard<std::mutex> lock(Debugger::Locker);
    Debugger::Pipe = Ptr;
    flags = Type;
    updated = true;
    Debugger::Notifier.notify_all();
}

void Debugger::SetStack(StackFrame* Vars) {
    TypeFlags flag;
    flag.Stack = true;
    flag.StackPtr = false;
    Synchronize((size_t) Vars, flag);
}

void Debugger::SpinOff() {
    Thread = std::thread([]() {
        SetupWindow();
        while(VerifyOpen()) {
            RenderFrame();
        }
        ShutdownWindow();
    });
}

void Debugger::Rejoin() {
    Debugger::Thread.join();
}

#ifndef VISUAL_DEBUGGER

void Debugger::SetupWindow() {}
void Debugger::ShutdownWindow() {}
void Debugger::CreateBytecode() {}
void Debugger::CreateStack() {}
bool Debugger::VerifyOpen() { return false; }
void Debugger::RenderFrame() {}

#else

/** Debugger Thread Variables **/
SDL_WindowFlags window_flags;
SDL_Window* window;
SDL_GLContext gl_context;
ImGuiIO* io;
ImVec4 clear_color;

/** Debugger Thread Functions **/

void Debugger::SetupWindow() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return;
    }
    
    clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    window = SDL_CreateWindow("Purpuri Visual Debugger", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = &ImGui::GetIO();

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);
}

void Debugger::ShutdownWindow() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Debugger::CreateBytecode() {
    std::unique_lock<std::mutex> lock(Locker);

    ImGui::Begin("Bytecode Viewer");

    static BytecodeListing listing;

    ImGui::Text("Function: %s", stack->_Class->GetStringConstant(stack->_Method->Name).c_str());
    listing.DrawContents(stack->_Method->Code->Code, stack->_Method->Code->CodeLength, stack->ProgramCounter);

    ImGui::End();

}

void Debugger::CreateStack() {
    std::unique_lock<std::mutex> lock(Locker);
    
    if(updated) {
        if(flags.Stack)
            stack = (StackFrame*) Pipe;
        updated = false;
    }

    ImGui::Begin("Stack Viewer");

    ImGui::BeginListBox("Stack View");

    for (uint16_t i = 0; i < stack->StackPointer; i++) {
        Variable item = stack->Stack[i];
        ImGui::Text("Stack Index %d", i);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Char: %d\nShort: %d\nInt: %d\nLong: " PrtSizeT "\nFloat: %.6f\nDouble: %.6f\nObject: " PrtSizeT "\n",
                         item.charVal, item.shortVal, item.intVal, item.pointerVal, item.floatVal, item.doubleVal, item.object.Heap);
    }

    ImGui::EndListBox();

    if(ImGui::Button("Dump Stack")) {
        for (uint16_t i = 0; i < stack->StackPointer; i++) {
            Variable item = stack->Stack[i];
            printf("\t[DEBUG] Stack index %d:\n\t\tInt: %d, Long: " PrtSizeT ", Float: %.6f, Double: %.6f, Object: " PrtSizeT " / %d\n",
                                           i,           item.intVal, 
                                                                  item.pointerVal, 
                                                                            item.floatVal,  item.doubleVal, 
                                                                                                          item.object.Heap, 
                                                                                                                item.object.Type);
        }
    }
    
    lock.unlock();

    ImGui::End();
}

bool Debugger::VerifyOpen() {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
            return false;
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
            return false;
    }

    return true;
}

void Debugger::RenderFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    bool yes = true;
    ImGui::ShowDemoWindow(&yes);

    CreateStack();
    CreateBytecode();

    ImGui::Render();
    glViewport(0, 0, (int) (*io).DisplaySize.x, (int) (*io).DisplaySize.y);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
}

#endif