/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <stddef.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vm/Class.hpp>

/**
 * Handles the Visual Debugger.
 * Only one debugger can be attached to the JVM at a time.
 * For this reason, all of the fields and methods on the class are static.
 * 
 * The Visual Debugger contains the following panels:
 * 
 *  - Stack; Shows a view of the Frame Stack and the Stack Members at any given time.
 *  - Bytecode; Shows a view of the bytecode currently running, as well as what instruction is currently being executed.
 *  - Classes; Shows a list of all the classes loaded, and all the data about them.
 *  - Engine; Shows the state of the Execution engine and what is currently occuring.
 *  - Objects; Shows the Object Heap and all of the objects currently loaded.
 * 
 * These tools allow you to fully debug a program running on the Purpuri VM.
 * 
 * It also turns out to be useful for debugging the VM itself.
 * 
 * The Debugger runs as a separate thread - thus, there are functions here that are called 
 *  by the new thread, and functions that are called by the parent thread.
 * 
 */

class Debugger {
    public:

    struct TypeFlags {
        bool Stack;
        bool StackPtr;
    };
    
    /** Main thread **/

    // Is Debugging Enabled?
    static bool Enabled;

    // The thread instance, for re-joining after spawning.
    static std::thread Thread;


    // The Mutex used to communicate with the thread
    static std::mutex Locker;

    // The Condition Variable used to notify the thread of new data
    static std::condition_variable Notifier;

    // The data to be transferred to the thread. Usually a pointer.
    static size_t Pipe;

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

    /*********************************
     ******* Thread Management *******
     *********************************/

    // Spin a new thread and run the window concurrently.
    static void SpinOff();

    // Join the spawning thread and close the window.
    static void Rejoin();   



    /** Debugger Thread **/


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

};