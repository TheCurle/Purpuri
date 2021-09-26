/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <stddef.h>
#include <thread>
#include <vm/Class.hpp>

/**
 * Handles the debugger window.
 * Contains the following panels:
 * 
 *  - Stack; Shows a view of the Frame Stack and the Stack Members at any given time.
 *  - Bytecode; Shows a view of the bytecode currently running, as well as what instruction is currently being executed.
 *  - Classes; Shows a list of all the classes loaded, and all the data about them.
 *  - Engine; Shows the state of the Execution engine and what is currently occuring.
 *  - Objects; Shows the Object Heap and all of the objects currently loaded.
 * 
 *
 * These tools allow you to fully debug a program running on the Purpuri VM.
 * 
 * It also turns out to be useful for debugging the VM itself.
 * 
 */

class Debugger {
    public:

    // Is Debugging Enabled?
    static bool Enabled;

    static std::thread Thread;

    /**
     * Setup the main window.
     * Performs all of the GL and SDL setup, ready for rendering.
     */

    static void SetupWindow();

    /**
     * Cleans up for shutdown.
     */

    static void ShutdownWindow();

    /**
     * Create the Stack panel.
     */
    static void CreateStack();

    /**
     * Create the Bytecode panel.
     */
    static void CreateBytecode();

    /**
     * Create the Class panel.
     */
    static void CreateClass();

    /**
     * Create the Objects panel.
     */
    static void CreateObjects();

    /*********************************
     ******* Signal Management *******
     *********************************/

    // Change the set of currently executing bytecode
    static void SetBytecode(const char* Code);

    // Change the location of the current instruction
    static void SetBytecodeInstr(const size_t InstructionPointer);

    // Change the currently executing class
    static void SetClass(Class* NewClass);

    // Update the representation of the Frame Stack (to track function calls)
    static void SetFrameStack(StackFrame* NewFrame);

    // Update the representation of the Stack (to track variables)
    static void SetStack(StackFrame* NewFrame);

    /*************************
     ******* Rendering *******
     *************************/

    // Gather all necessary data and update everything to render a new frame.
    static void RenderFrame();


    /********************************
     ******* Event Management *******
     ********************************/

    // Poll events and make sure the window should still be open.
    static bool VerifyOpen();

    
    /*********************************
     ******* Thread Management *******
     *********************************/

    // Spin a new thread and run the window concurrently.
    static void SpinOff();

    // Join the spawning thread and close the window.
    static void Rejoin();   

};