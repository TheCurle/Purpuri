/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <vm/Class.hpp>

#include <iostream>
#include <fstream>
#include <list>
#include <algorithm>
#include <string>
#include <limits>

/**
 * This file implements the Class class, declared in Common.hpp.
 *
 * This is the class that actually performs classloading; the act of reading bytecode files,
 *  parsing their useful information, and storing the code contents of each method ready for execution.
 *
 * Classloading is a rather involved, multiple-step process, so it'll be explained in detail atop each function.
 * If you're looking for where it all comes together, see ParseFullClass.
 *
 * To load a class through this method, use LoadFromFile.
 * If you have the bytecode representation already, it's okay to directly call ParseFullFile.
 *
 * Code is currently read byte by byte from a char* array.
 * To facilitate reading integers, shorts, etc from the array, there are a few macros.
 *
 * ReadShortFromStream:
 *  (uint16_t) ( ((uint16_t)((Stream)[0]) << 8 & 0xFF00) | (((uint16_t)(Stream)[1]) & 0x00FF) )
 *
 * ReadIntFromStream:
 *  (uint32_t)( ((uint32_t)((Stream)[0]) << 24 & 0xFF000000) | ((uint32_t)((Stream)[1]) << 16 & 0x00FF0000) \
 *      | ((uint32_t)((Stream)[2]) << 8 & 0x0000FF00) | ((uint32_t)((Stream)[3]) & 0x000000FF) )
 *
 * These macros are hard-coded to be little-endian.
 * A future improvement would be to move this whole system to a std container type.
 */

// A safe default for when we try to request a class from the heap that is not loaded.
// Stored here so that we can access it during our checks.
std::string ClassHeap::UnknownClass("Unknown Value");

Class::Class() : ClassFile(), BytecodeLength(0), Code(nullptr), _ClassHeap(nullptr), FieldsCount(0) {
    LoadedLocation = (size_t)(size_t*) this;
}

// TODO: Delete EVERYTHING
Class::~Class() = default;

/**
 * A simple wrapper that reads all the bytes in a file and loads it as a Java class.
 *
 * If the file does not exist, false is returned to indicate an error.
 *
 * Once the file is read correctly, SetCode is called to parse the bytecode.
 * @param Filename the name of the Class file to load.
 * @return true if the file was loaded successfully, false otherwise.
 */

bool Class::LoadFromFile(const char* Filename) {
    char* LocalCode;

    // Start the file at-the-end (ate) so that we can skip a stat
    std::ifstream File(Filename, std::ios::binary | std::ios::ate);

    // Make sure the file exists.
    if(!File.is_open()) {
        printf("Unable to open class file: %s\n", Filename);
        return false;
    }

    // Read the size of the file.
    std::streamsize FileSize = File.tellg();
    // Return to the start so that we can actually read the data.
    File.seekg(0);

    // tellg returns -1 if something went wrong reading the file data, so we need to handle that.
    if(FileSize == -1) {
        puts("Class file is empty or corrupt");
        return false;
    }

    // The bytecode is stored on-heap. Allocate it, and read from the file.
    LocalCode = (char*) new char[FileSize + 2];
    File.read(&LocalCode[0], FileSize);

    File.close();

    // If the data was read correctly, we need to call out and parse the bytecode.
    LocalCode[FileSize] = 0;
    BytecodeLength = FileSize;
    SetCode(LocalCode);

    return true;
}

/**
 * A simple wrapper that overrides the bytecode stored in the current class.
 * @param p_Code the new bytecode to read
 */
void Class::SetCode(const char* p_Code) {
    // Code is stored on-heap, so make sure to free the old array when we load a new one
    delete Code;
    Code = p_Code;

    if(Code) ParseFullClass();
}

/**
 * The core method that takes a Class' bytecode and parses it out into the ClassFile struct representation.
 *
 * A class' structure is as follows:
 *
 * ┌────────────────────────┐
 * │                        │
 * │  ┌──────────────────┐  │
 * │  │                  │  │
 * │  │  Magic Number:   │  │   - The Magic Number takes the first four bytes of the Class file, and exists as
 * │  │    0xCAFEBABE    │  │      a safe, defined way to determine whether it's actually a class file that
 * │  │                  │  │      is about to be read.
 * │  └──────────────────┘  │
 * │                        │
 * │  ┌──────────────────┐  │
 * │  │                  │  │
 * │  │  Bytecode Ver:   │  │   - The Bytecode Version is stored in two parts; major and minor.
 * │  │      Minor       │  │      It's a signature left behind by the compiler, which tells the JVM
 * │  │                  │  │      which version of Java it should expect to read.
 * │  └──────────────────┘  │
 * │                        │
 * │  ┌──────────────────┐  │
 * │  │                  │  │
 * │  │  Bytecode Ver:   │  │
 * │  │      Major       │  │
 * │  │                  │  │
 * │  └──────────────────┘  │
 * │                        │
 * │  ┌──────────────────┐  │
 * │  │                  │  │    - The Constants Pool is an array of constants, of known size.
 * │  │  Constants Pool  │  │       It's contents are accessed by index, and may have many different types.
 * │  │       Size       │  │       See Constants.cpp for how the Constants Pool is read, parsed, and its
 * │  │                  │  │        contents managed.
 * │  └──────────────────┘  │
 * │                        │
 * │  ┌─ ── ── ── ── ── ─┐  │
 * │  │                  │  │
 * │     Constants Pool     │
 * │  │       Data       │  │
 * │  │                  │  │
 * │  └─ ── ── ── ── ── ─┘  │
 * │                        │
 * │  ┌──────────────────┐  │
 * │  │                  │  │    - Java classes may have varying access levels.
 * │  │   Class Access   │  │       These include: public, package-private, private, sealed.
 * │  │                  │  │       This entry is a simple flag that tells the JVM which access level to store it as.
 * │  └──────────────────┘  │
 * │                        │
 * │  ┌──────────────────┐  │
 * │  │                  │  │    - Each loaded class knows its' own name.
 * │  │  Constants Pool  │  │       This is achieved by storing an index into the Constants Pool.
 * │  │    Class Name    │  │       The Constant resolves to a Utf8String that holds the class package & name.
 * │  │                  │  │
 * │  └──────────────────┘  │
 * │                        │
 * │  ┌──────────────────┐  │
 * │  │                  │  │    - Each loaded class also knows its' super class' name.
 * │  │  Constants Pool  │  │       Every class extends Object by default, so unless the class is Object itself, this
 * │  │    Super Name    │  │        is either java/lang/Object or the explicitly declared superclass.
 * │  │                  │  │
 * │  └──────────────────┘  │
 * │                        │
 * │  ┌──────────────────┐  │
 * │  │                  │  │    - Classes may implement interfaces, but this is not required.
 * │  │    Interfaces    │  │       If it does implement an interface, or a super class does, it is recorded here.
 * │  │      Count       │  │       This facilitates searching interfaces for default method implementations, etc.
 * │  │                  │  │
 * │  └──────────────────┘  │
 * │                        │
 * │  ┌─ ── ── ── ── ── ─┐  │
 * │  │                  │  │
 * │       Interfaces       │
 * │  │       Data       │  │
 * │  │                  │  │
 * │  └─ ── ── ── ── ── ─┘  │
 * │                        │
 * │  ┌──────────────────┐  │
 * │  │                  │  │    - Java classes store data in the form of fields.
 * │  │   Fields Count   │  │       They're effectively dumb containers for a Variable.
 * │  │                  │  │       See the Variable declaration in Common.hpp for more information on that.
 * │  └──────────────────┘  │
 * │                        │
 * │  ┌─ ── ── ── ── ── ─┐  │
 * │  │                  │  │
 * │         Fields         │
 * │  │       Data       │  │
 * │  │                  │  │
 * │  └─ ── ── ── ── ── ─┘  │
 * │                        │
 * │  ┌──────────────────┐  │
 * │  │                  │  │    - Java classes actually perform calculations and run code via methods.
 * │  │     Methods      │  │       They're collections of bytecode that the JVM interprets.
 * │  │      Count       │  │       They operate on the Member Stack and can call & return other Methods.
 * │  │                  │  │       They can also read + write to Fields.
 * │  └──────────────────┘  │
 * │                        │
 * │  ┌─ ── ── ── ── ── ─┐  │
 * │  │                  │  │
 * │      Methods Data      │
 * │  │                  │  │
 * │  └─ ── ── ── ── ── ─┘  │
 * │                        │
 * │  ┌──────────────────┐  │
 * │  │                  │  │     - I'm still not sure what Class-level Attributes do.
 * │  │    Attributes    │  │        They store annotations, which are interfaces that can also be
 * │  │      Count       │  │         implementd by individual methods and fields.
 * │  │                  │  │
 * │  └──────────────────┘  │
 * │                        │
 * │  ┌─ ── ── ── ── ── ─┐  │
 * │  │                  │  │
 * │       Attributes       │
 * │  │       Data       │  │
 * │  │                  │  │
 * │  └─ ── ── ── ── ── ─┘  │
 * │                        │
 * └────────────────────────┘
 *
 * The data represented by dotted boxes will repeat according to the number in the data block above them.
 * In this way, parsing Java classes is completely deterministic.
 *
 * @return whether the class was parsed correctly.
 */
bool Class::ParseFullClass() {
    // The Java specification dictates that there must be 20 bytes of metadata for attributes, code, etc.
    // We can use this as a simple safeguard to prevent us trying to load invalid data.
    if(Code == nullptr || BytecodeLength < sizeof(ClassFile) + 20)
        return false;

    puts("Reading a new class.");

    MagicNumber = ReadIntFromStream(Code);
    Code += 4;

    if(MagicNumber != 0xCAFEBABE) {
        printf("Magic number 0x%x does not match 0xCAFEBABE\n", MagicNumber);
        return false;
    }

    printf("Magic: 0x%x\n", MagicNumber);

    BytecodeVersionMinor = ReadShortFromStream(Code); Code += 2;
    BytecodeVersionMajor = ReadShortFromStream(Code); Code += 2;

    printf("Bytecode Version %d.%d\n", BytecodeVersionMajor, BytecodeVersionMinor);

    ConstantCount = ReadShortFromStream(Code); Code += 2;

    if(ConstantCount > 0)
        ParseConstants(Code);

    ClassAccess = ReadShortFromStream(Code); Code += 2;

    printf("Class has access %d.\r\n", ClassAccess);
    This = ReadShortFromStream(Code); Code += 2;
    Super = ReadShortFromStream(Code); Code += 2;

    InterfaceCount = ReadShortFromStream(Code); Code += 2;

    if(InterfaceCount > 0)
        ParseInterfaces(Code);

    FieldsCount = ReadShortFromStream(Code); Code += 2;

    ClassStatics = new Variable[FieldsCount];

    if(FieldsCount > 0) {
        ParseFields(Code);
    }

    MethodCount = ReadShortFromStream(Code); Code += 2;

    if(MethodCount > 0)
        ParseMethods(Code);

    AttributeCount = ReadShortFromStream(Code); Code += 2;

    if(AttributeCount > 0)
        ParseAttribs(Code);

    // This is a big no-no!
    // Java's classloader normally only attempts to load a class when it is actually required.
    // This way, it saves on class heap space in large programs.

    // Purpuri chooses to search through all classes in this class' Constants Pool, and load them all.
    // This is extremely expensive, and wasteful!
    // However, I'm still unsure how to do it the "proper" way, so this remains as a permanently temporary hack.

    ClassloadReferents();

    return true;
}

/**
 * As mentioned directly above, we go against Java Specification here.
 * We should only be classloading as soon as they're referenced.
 * There's no easy way to do that, primarily because of interfaces.
 *
 * Instead, I iterate the constants pool and search for all references to another Class.
 * This is, as mentioned, extremely wasteful.
 *
 * This whole section should be refactored, ideally removed.
 */
void Class::ClassloadReferents() {

    printf("Searching for unloaded classes referenced by the current.\n");

    // We need to make sure we load String if there are any interned strings stored in this class.
    bool NeedsString = false;

    // For each constant stored..
    for(int i = 1; i < ConstantCount; i++) {
        if(Constants == nullptr) return;
        if(Constants[i] == nullptr) continue;

        // Check if it's a string. If so, we need to classload the String class so that we can use it later.
        if(Constants[i]->Tag == TypeString)
            NeedsString = true;

        // Check if it's a Class. If so, we need to check if it's loaded, and load it if it isn't already.
        else if(Constants[i]->Tag == TypeClass) {
            // There's no good way to read an int from a char*, so we need to temporarily store it and ReadShort.
            auto ClassInd = ReadShortFromStream((char*)Constants[i] + 1);

            // GetStringConstant is implemented in Constants.cpp.
            std::string ClassName = GetStringConstant(ClassInd);

            // We need to NOT try to classload, if we're looking at an array type.
            if(ClassName.find_first_of('[') == 0) continue;

            printf("\tName %s", ClassName.c_str());

            // We can guarantee that the Super Class and This Class are already loaded.
            // We also don't want to try to load classes twice, so we check the Class Cache in the Class Heap.
            // The Class Cache isn't set while a class is loading, so if we find ourself in a dependency loop,
            //  we need to also check the ClassHeap itself for nullptr values.
            // If it's nullptr, we're in the process of loading it (through that class' ClassloadReferents) so skip it.
            if(i != Super && i != This && (!this->_ClassHeap->ClassExists(ClassName) && this->_ClassHeap->GetClass(ClassName) == nullptr)) {
                printf("\tClass is not loaded - invoking the classloader\n");
                auto* Class = new class Class();

                // Note that this is potentially re-entrant; LoadClass calls LoadFromFile, which eventually calls ClassloadReferents.
                if(!this->_ClassHeap->LoadClass(ClassName.c_str(), Class)) {
                    printf("Classloading referenced class %s failed. Fatal error.\n", ClassName.c_str());
                    exit(6);
                }
            } else {
                printf(" - clear\n");
            }
        }
    }

    
    if(NeedsString) {
        if(!this->_ClassHeap->LoadClass("java/lang/String", new Class)) {
            printf("Classloading referenced class %s failed. Fatal error.\n", "java/lang/String");
            exit(6);
        }
    }
}

/**
 * Attributes are simple stores of data.
 * Their format is as follows:
 *
 * ┌─────────────────────────────┐
 * │                             │
 * │ ┌─────────────────────────┐ │
 * │ │                         │ │
 * │ │  Attribute Name         │ │
 * │ │   Constants Pool Index  │ │
 * │ │                         │ │
 * │ └─────────────────────────┘ │
 * │                             │
 * │ ┌─────────────────────────┐ │
 * │ │                         │ │
 * │ │  Attribute Data Length  │ │
 * │ │                         │ │
 * │ └─────────────────────────┘ │
 * │                             │
 * │ ┌─ ── ── ── ── ── ── ── ──┐ │
 * │ │                         │ │
 * │        Attribute Data       │
 * │ │                         │ │
 * │ └─ ── ── ── ── ── ── ── ──┘ │
 * │                             │
 * └─────────────────────────────┘
 *
 * As always, the dotted data box is as big as the data box above it.
 *
 * @param ClassCode the bytecode of the class, starting at the first bytes of the Attributes section.
 * @return whether the attributes were parsed properly.
 */
bool Class::ParseAttribs(const char *&ClassCode) {
    Attributes = new AttributeData* [AttributeCount];
    puts("Reading class Attributes");

    for(int j = 0; j < AttributeCount; j++) {
        printf("\tParsing attribute %d\n", j);
        // The AttributeData struct is guaranteed to match up with the format of the attribute bytecode data.
        // After this, we just need to wind past this attribute to get to the next one, for the next iteration.
        Attributes[j] = (AttributeData*) ClassCode;

        // We don't just skip past the attribute, we also take some data from it for the print output.
        auto AttrName = ReadShortFromStream(ClassCode); ClassCode += 2;
        size_t AttrLength = ReadIntFromStream(ClassCode); ClassCode += 4;
        ClassCode += AttrLength;

        printf("\tAttribute has id %d, length " PrtSizeT "\n", AttrName, AttrLength);
    }

    return true;
}

/**
 * Interfaces are special classes that can not be instantiated on their own.
 * Classes must implement them to take on their methods, and multiple classes can be stored under the same
 *  interface type.
 *
 * Interfaces are stored in the bytecode as an array of Constant Pool indexes.
 * They index Class references.
 *
 * @param ClassCode the bytecode of the class, starting at the first bytes of the Interfaces section.
 * @return whether the interfaces were parsed properly.
 */

bool Class::ParseInterfaces(const char* &ClassCode) {
    Interfaces = new uint16_t[InterfaceCount];
    printf("Reading %d interfaces\n", InterfaceCount);

    // For each interface...
    for(int i = 0; i < InterfaceCount; i++) {
        // Read the class index.
        Interfaces[i] = ReadShortFromStream(ClassCode);
        ClassCode += 2; // We're now ready for the next iteration, but let's log something.

        // Read the name from the Class data.
        auto NameInd = ReadShortFromStream((char*) Constants[Interfaces[i]] + 1);

        // Print out the name of the interface.
        printf("\tThis class implements interface %s\n", GetStringConstant(NameInd).c_str());
    }

    return true;
}

/**
 * Methods have two main components; metadata and codepoints.
 *
 * Method metadata has the following structure:
 * ┌───────────────────────────────┐
 * │ ┌──────────────────────────┐  │
 * │ │                          │  │    - Methods, like classes, have access levels.
 * │ │       Access Flags       │  │       These include: public, package-private, private.
 * │ │                          │  │       This is a simple flag that determines which access level this method has.
 * │ └──────────────────────────┘  │
 * │                               │
 * │ ┌──────────────────────────┐  │
 * │ │                          │  │    - The method knows its' own name.
 * │ │  Name                    │  │       This is saved in the class' Constants Pool, and this index points to the
 * │ │    Constants Pool Index  │  │        name of the method.
 * │ │                          │  │
 * │ └──────────────────────────┘  │
 * │                               │
 * │ ┌──────────────────────────┐  │
 * │ │                          │  │    - The method's parameters and return value are dictated by the Descriptor.
 * │ │  Descriptor              │  │       The descriptor has the form (PARAMETERS)RETURN.
 * │ │    Constants Pool Index  │  │       For example, a method that takes no parameters and returns void
 * │ │                          │  │        will have the descriptor of ()V.
 * │ └──────────────────────────┘  │
 * │                               │
 * │ ┌──────────────────────────┐  │
 * │ │                          │  │    - Like Classes, methods have attributes.
 * │ │        Attributes        │  │       However, these are actually useful this time.
 * │ │          Count           │  │       They encode any annotations and compile-time data the JVM should know.
 * │ │                          │  │       They also encode the method's bytecode.
 * │ └──────────────────────────┘  │
 * │                               │
 * │ ┌── ── ── ── ── ── ── ── ──┐  │
 * │ │                          │  │     - Attributes have a name, a length, and data.
 * │          Attributes           │        The data encoded should be parsed based on the name.
 * │            Data               │        For example, the Attribute named MethodCode contains the bytecode.
 * │ │                          │  │
 * │ └── ── ── ── ── ── ── ── ──┘  │
 * └───────────────────────────────┘
 *
 * As always, the dotted box's length depends on the contents of the Data box above it.
 *
 * The part of the Method we're interested in is the Code Point, which exists inside the MethodCode attribute as
 *  described above.
 *
 * However, this attribute does not exist on method declarations, ie. in interfaces.
 * Therefore, some extra checks need to happen before we try to parse the Code Points.
 *
 * For that reason, parsing of the Code Point itself happens in another function.
 *
 * @param ClassCode the bytecode of the class, starting at the first bytes of the Methods section.
 * @return whether the methods were parsed properly.
 */

bool Class::ParseMethods(const char *&ClassCode) {
    Methods = new Method[MethodCount];
    if(Methods == nullptr) return false;

    // We need to parse multiple methods, so for each method the class stores...
    for(int i = 0; i < MethodCount; i++) {
        // Save some crucial data.
        // First off, save a reference to the start of all this data.
        Methods[i].Data = (MethodData*) ClassCode;

        // Access, Name, Descriptor and Attribute Count are all easily parsed out.
        Methods[i].Access = ReadShortFromStream(ClassCode); ClassCode += 2;
        Methods[i].Name = ReadShortFromStream(ClassCode); ClassCode += 2;
        Methods[i].Descriptor = ReadShortFromStream(ClassCode); ClassCode += 2;
        Methods[i].AttributeCount = ReadShortFromStream(ClassCode); ClassCode += 2;

        // Retrieve some data from the class for debug printing.
        std::string Name = GetStringConstant(Methods[i].Name);
        std::string Descriptor = GetStringConstant(Methods[i].Descriptor);
        printf("Parsing method %s%s with access %d, %d attributes\n", Name.c_str(), Descriptor.c_str(), Methods[i].Access, Methods[i].AttributeCount);

        // This may be a declaration, not a definition. Set the Code to a nullptr so we don't try to access it
        // prematurely.
        Methods[i].Code = nullptr;

        // Declarations (with no annotations, at least) have no attributes.
        // Safeguard everything behind a check.
        if(Methods[i].AttributeCount > 0) {
            // We don't need any of this, it's just good for debug printing.
            // So, we can skip it all if we're in quiet mode.
            if (!Engine::QuietMode)
                for(int j = 0; j < Methods[i].AttributeCount; j++) {
                    printf("\tParsing attribute %d\n", j);
                    auto AttrNameInd = ReadShortFromStream(ClassCode); ClassCode += 2;
                    size_t AttrLength = ReadIntFromStream(ClassCode); ClassCode += 4;
                    ClassCode += AttrLength; // Skip to the next Attribute for the next loop

                    std::string AttrName = GetStringConstant(AttrNameInd);
                    printf("\tAttribute has name %s, length " PrtSizeT "\n", AttrName.c_str(), AttrLength);
                }

            // We need to spin out to another method to parse the Code Point,
            // in the sake of code cleanliness and linearity.
            Methods[i].Code = new CodePoint;
            ParseMethodCodePoints(i, Methods[i].Code);
        }
    }

    return true;
}


/**
 * Method Code Points are the Attributes that contain the bytecode and all associated metadata for execution.
 *
 * A Code Point's structure is as follows:
 *
 * ┌──────────────────┐
 * │ ┌──────────────┐ │
 * │ │     Name     │ │    - A Constants Pool index that points to the name of this Attribute.
 * │ └──────────────┘ │
 * │                  │
 * │ ┌──────────────┐ │
 * │ │    Length    │ │    - The total length of the Attribute data that follows.
 * │ └──────────────┘ │
 * │                  │
 * │ ┌──────────────┐ │
 * │ │  Stack Size  │ │    - The maximum depth that the execution of this function will bring the stack down by.
 * │ └──────────────┘ │
 * │                  │
 * │ ┌──────────────┐ │
 * │ │    Locals    │ │    - The maximum number of local variables that will be allocated and returned during
 * │ │      Size    │ │       execution of this method.
 * │ └──────────────┘ │
 * │                  │
 * │ ┌──────────────┐ │
 * │ │   Bytecode   │ │    - The bytecode data itself.
 * │ │     Length   │ │       This is what the JVM actually interprets to perform calculations.
 * │ └──────────────┘ │
 * │                  │
 * │ ┌── ── ── ── ──┐ │
 * │     Bytecode     │
 * │       Data       │
 * │ └── ── ── ── ──┘ │
 * │                  │
 * │ ┌──────────────┐ │
 * │ │  Exceptions  │ │    - Exception metadata is contained here too.
 * │ │    Count     │ │       It includes the exceptions that are thrown from the current method, as well as
 * │ └──────────────┘ │        all data required to handle exceptions that are thrown from lower in the stack.
 * │                  │
 * │ ┌── ── ── ── ──┐ │
 * │    Exceptions    │
 * │      Data        │
 * │ └── ── ── ── ──┘ │
 * │                  │
 * │ ┌──────────────┐ │
 * │ │  Attributes  │ │    - Code Point Attributes are not currently used by Purpuri.
 * │ │    Count     │ │
 * │ └──────────────┘ │
 * │                  │
 * │ ┌── ── ── ── ──┐ │
 * │    Attributes    │
 * │      Data        │
 * │ └── ── ── ── ──┘ │
 * └──────────────────┘
 *
 * As always, the dotted box's length depends on the contents of the Data box above it.
 *
 *
 * @param Method the Method that we're trying to load the CodePoints of.
 * @param MethodCode the Code Point reference to save our data into.
 * @return true if the Code Point was parsed properly, false otherwise.
 */
bool Class::ParseMethodCodePoints(int Method, CodePoint *MethodCode) {
    if(Methods == nullptr || Method > MethodCount) return false;

    // We've already scanned the Class code past all of this, so we return with the Data pointer we saved earlier.
    char* CodeBase = (char*)Methods[Method].Data;
    CodeBase += 6; // Skip Access, Name, Descriptor

    // We need to check how many attributes there are again.
    int AttribCount = ReadShortFromStream(CodeBase); CodeBase += 2;
    if(AttribCount > 0) {

        // For each attribute..
        for(int i = 0; i < AttribCount; i++) {
            // Fetch and read the name.
            auto NameInd = ReadShortFromStream(CodeBase); CodeBase += 2;
            std::string AttribName = GetStringConstant(NameInd);

            // If we're not looking at the Method Code, keep looking.
            if(AttribName != "MethodCode" && AttribName != "Code") continue;

            // If we get here, we must be looking at MethodCode - the Code Point data.
            // So we read all of the easy-to-read static data.
            char* AttribCode = CodeBase;
            MethodCode->Name = NameInd;
            MethodCode->Length = ReadIntFromStream(AttribCode); AttribCode += 4;
            MethodCode->StackSize = ReadShortFromStream(AttribCode); AttribCode += 2;
            MethodCode->LocalsSize = ReadShortFromStream(AttribCode); AttribCode += 2;
            MethodCode->CodeLength = ReadIntFromStream(AttribCode); AttribCode += 4;

            // It's possible for Method declarations to have a MethodCode with no CodeLength or Bytecode.
            // This is technically malformed classes, but the spec changed with Java 8 to make this
            // javac bug part of the specification.
            // Oracle.
            if((AttribName == "MethodCode" && MethodCode->CodeLength > 0)) {
                // Allocate and copy the data out of the class file so that we can use it later.
                MethodCode->Code = new uint8_t[MethodCode->CodeLength];
                memcpy(MethodCode->Code, AttribCode, MethodCode->CodeLength);
            } else if ((AttribName == "Code" && MethodCode->Length > 0)) {
                // Allocate and copy the data out of the class file so that we can use it later.
                MethodCode->Code = new uint8_t[MethodCode->Length];
                memcpy(MethodCode->Code, AttribCode, MethodCode->Length);
            } else {
                MethodCode->Code = nullptr;
            }

            AttribCode += MethodCode->CodeLength;

            // Exceptions are not currently handled in Purpuri, but they're easy to parse so we do it anyway.
            // Once we use them, this will be moved into yet another function and explained in detail.
            MethodCode->ExceptionCount = ReadShortFromStream(AttribCode);
            if(MethodCode->ExceptionCount > 0) {
                MethodCode->Exceptions = new Exception[MethodCode->ExceptionCount];

                for(int Exc = 0; Exc < MethodCode->ExceptionCount; Exc++) {
                    MethodCode->Exceptions[Exc].PCStart = ReadShortFromStream(AttribCode); AttribCode += 2;
                    MethodCode->Exceptions[Exc].PCEnd = ReadShortFromStream(AttribCode); AttribCode += 2;
                    MethodCode->Exceptions[Exc].PCHandler = ReadShortFromStream(AttribCode); AttribCode += 2;
                    MethodCode->Exceptions[Exc].CatchType = ReadShortFromStream(AttribCode); AttribCode += 2;
                }
            }

            auto Length = ReadIntFromStream(CodeBase); CodeBase += 4;
            CodeBase += Length;
        }
    }

    return true;
}

/**
 * Fields store Variable data in a way that can be accessed from the class or, if public, any class.
 *
 * Fields have the following layout:
 *
 * ┌──────────────────┐
 * │ ┌──────────────┐ │    - Fields, like classes, have access levels.
 * │ │ Access Flags │ │       These include: public, package-private, private.
 * │ └──────────────┘ │       This is a simple flag that determines which access level this field has.
 * │                  │
 * │ ┌──────────────┐ │
 * │ │     Name     │ │    - A Constants Pool index that points to the name of this Field.
 * │ └──────────────┘ │
 * │                  │
 * │ ┌──────────────┐ │
 * │ │  Descriptor  │ │    - A Constants Pool index that points to the descriptor of this Field.
 * │ └──────────────┘ │
 * │                  │
 * │ ┌──────────────┐ │
 * │ │  Attributes  │ │    - Field Attributes are not currently used by Purpuri.
 * │ │    Count     │ │       They store things like Annotations.
 * │ └──────────────┘ │
 * │                  │
 * │ ┌── ── ── ── ──┐ │
 * │    Attributes    │
 * │      Data        │
 * │ └── ── ── ── ──┘ │
 * └──────────────────┘
 *
 * As always, the dotted box's length depends on the contents of the Data box above it.
 *
 * @param ClassCode the bytecode of the class, starting at the first bytes of the Fields section.
 * @return whether the fields were parsed properly.
 */
bool Class::ParseFields(const char* &ClassCode) {
    Fields = new FieldData*[FieldsCount];
    if(Fields == nullptr) return false;

    // For each Field..
    for(size_t i = 0; i < FieldsCount; i++) {
        // Store an index to the data - it's guaranteed to be valid.
        Fields[i] = (FieldData*) ClassCode;

        // Populate some member variables so we have an excuse to increase the code pointer
        Fields[i]->Access = ReadShortFromStream(ClassCode); ClassCode += 2;
        Fields[i]->Name = ReadShortFromStream(ClassCode); ClassCode += 2;
        Fields[i]->Descriptor = ReadShortFromStream(ClassCode); ClassCode += 2;
        Fields[i]->AttributeCount = ReadShortFromStream(ClassCode); ClassCode += 2;

        // Let's handle static fields here.
        // 0x8 is ACCESS_STATIC. If this is set, the field is global, and not bound to any instance of the class.
        if(Fields[i]->Access & 0x8) {
            // Store that we have a static field here.
            StaticFieldIndexes.emplace_back(i);
            // Initialize the field to Null - it's the default.
            PutStatic(i, Variable(ObjectHeap::Null));
        }

        // Handle attributes too, even though we don't use them.
        // We need to tick over the ClassCode pointer to get to the next Field.
        if(Fields[i]->AttributeCount > 0) {
            for(int attr = 0; attr < Fields[i]->AttributeCount; attr++) {
                ClassCode += 2;
                size_t Length = ReadIntFromStream(ClassCode); ClassCode += 4;
                ClassCode += Length;
            }
        }
    }

    // After all iteration is finished, we know of all static fields in this class.
    StaticFieldCount = StaticFieldIndexes.size();

    return true;
}

/**
 * Due to the Object-Oriented nature of Java, it's possible for a class' method to not be in the method.
 * It may be in a super class, higher in the super class chain, or in an interface.
 *
 * When attempting to figure out which method to call, we need to ensure that we check all possible locations
 * of a method.
 *
 * Thus, this wrapper exists to search a Class' inheritance tree for a given method with a given Descriptor.
 *
 * @param MethodName the name of the Method to search for
 * @param Descriptor the descriptor (parameters, return value) of the Method to search for
 * @param ClassName the name of the Class we expect to find it in
 * @param pClass the Class to search for the method in
 * @return the index of the method, invokable
 */

uint32_t Class::GetMethodFromDescriptor(const char *MethodName, const char *Descriptor, const char* ClassName, Class *&pClass) {
    // Some code paths call this method early.
    // Notably, where errors occured during classloading and we try to find <clinit> or EntryPoint.
    // This block should be removed once the VM stabilizes.
    if(Methods == nullptr) {
        puts("GetMethodFromDescriptor called too early! Class not initialised yet!");
        return std::numeric_limits<uint32_t>::max();
    }

    // We need to recursively search up the class hierarchy to see where the method we want exists.
    // The simplest way to do that is to store a reference and update it each time we move up a class.
    class Class* CurrentClass = pClass;
    std::string ClassNameStr(ClassName);

    while(CurrentClass) {
        std::string MethodClass = CurrentClass->GetClassName();
        printf("Searching class %s for %s%s\n", MethodClass.c_str(), MethodName, Descriptor);

        // We also need to make sure we check the interfaces that each class implements - we can find default
        // implementations there.
        // Therefore, we quickly round up every interface that is implemented.
        std::list<std::string> InterfaceNames;
        if(CurrentClass->InterfaceCount > 0) {
            for(size_t iface = 0; iface < CurrentClass->InterfaceCount; iface++) {
                uint16_t interface = CurrentClass->Interfaces[iface];

                auto NameInd = ReadShortFromStream((char*) CurrentClass->Constants[interface] + 1);

                std::string IfaceName = GetStringConstant(NameInd);
                InterfaceNames.emplace_back(IfaceName);
            }
        }

        if(!InterfaceNames.empty()) {
            printf("Class implements interfaces ");
            PrintList(InterfaceNames);
            printf("\n");
        }

        // We can now search the methods in the class.
        for(int i = 0; i < CurrentClass->MethodCount; i++) {
            // Some quick debug printing
            std::string CurrentName = CurrentClass->GetStringConstant(CurrentClass->Methods[i].Name);
            std::string CurrentDescriptor = CurrentClass->GetStringConstant(CurrentClass->Methods[i].Descriptor);
            printf("\t\tExamining class %s for %s%s, access %d\r\n", MethodClass.c_str(), CurrentName.c_str(), CurrentDescriptor.c_str(), CurrentClass->ClassAccess);

            // We have the method implementation if:
            //  - the method name and method descriptor match, in the class we expected to search
            //  - the method name and method descriptor match, and the class we expected to search was an interface
            // This handles the case where the implementation is in ThingImpl, but we wanted the method from IThing.

            if(((CurrentName == MethodName) && (CurrentDescriptor == Descriptor)) &&
                ((MethodClass == ClassName) || (std::find(InterfaceNames.begin(), InterfaceNames.end(), ClassNameStr) != InterfaceNames.end()) )) {
                if(pClass) pClass = CurrentClass;

                printf("Found at index %d\n", i);
                return i;
            }
        }

        // If all of that searching showed nothing, we need to try again with the method's super.
        // This while loop will terminate when pClass is null - like when we try to get java/lang/Object's super.
        if(pClass != nullptr)
            CurrentClass = CurrentClass->GetSuper();
        else
            break;

    }

    return std::numeric_limits<uint32_t>::max();
}

/**
 * A simple wrapper to determine how many bytes it will take to store all the data in this class.
 * The bytes required = the number of fields in the hierarchy * the size of a single Variable
 * Note that if you wish to use this to allocate a class, you must add one Variable for the class pointer.
 * @return the size in bytes of all the fields this Class can store
 */
uint32_t Class::GetClassSize() {
    return GetClassFieldCount() * sizeof(Variable);
}

/**
 * A simple wrapper to determine how many fields are in this class and all of its supers.
 * This is recursive, so carefulness is recommended.
 * Issues have been encountered with incorrect classloader behavior leading to an infinite recursion depth.
 * @return the number of fields in the class
 */
uint32_t Class::GetClassFieldCount() {
    uint32_t Count = FieldsCount;

    Class* Super = GetSuper();

    uint32_t SuperSize = 0;
    if(Super)
        SuperSize = Super->GetClassFieldCount();
    
    Count += SuperSize;
    return Count;
}

/**
 * Return the super class.
 * If this class is not java/lang/Object, there is guaranteed to be a Super.
 * If this class is java/lang/Object, there is guaranteed to not be a Super.
 *
 * With these axioms, it's simple.
 * @return a pointer to the super class
 */
Class* Class::GetSuper() {
    if(this->GetStringConstant(This) == "java/lang/Object")
        return nullptr;

    return _ClassHeap->GetClass(GetSuperName());
}

/**
 * A simple wrapper to fetch the name of the Super class
 * @return the name of the class that this class extends
 */
std::string Class::GetSuperName() {
    return this->GetName(Super);
}

/**
 * A simple wrapper to fetch the name of This class.
 * Takes the form package/ClassName.
 *
 * @return the name of this class, including the package
 */
std::string Class::GetClassName() {
    return this->GetName(This);
}

/**
 * A method that takes a Class index to the Constants Pool and returns the name of that Class.
 * @param Obj a Constants Pool index, pointing to a Class type
 * @return the name of the Class type pointed, or UnknownClass if Obj is not a Class.
 */
std::string Class::GetName(uint16_t Obj) {
    if(Obj < 1 || (Obj != Super && Obj != This)) return ClassHeap::UnknownClass;

    auto NameInd = ReadShortFromStream((char*)Constants[Obj] + 1);

    return GetStringConstant(NameInd);
}

/**
 * A simple wrapper to create an object of the given type.
 * Referenced types are stored in the Constants Pool, so it makes sense to have this in the Class.
 *
 * @param Index the Index into the Constants Pool referencing the class to be instantiated
 * @param ObjectHeap the Object Heap to store the new Object Reference in
 * @return the new Object Reference of the given Class
 */
Object Class::CreateObject(uint16_t Index, ObjectHeap* ObjectHeap) {
    auto* ConstantType = (uint8_t*) this->Constants[Index];

    if(ConstantType[0] != TypeClass)
        return ObjectHeap::Null;

    auto NameInd = ReadShortFromStream(&ConstantType[1]);
    std::string Name = GetStringConstant(NameInd);

    Class* NewClass = this->_ClassHeap->GetClass(Name);
    if(NewClass == nullptr) return ObjectHeap::Null;

    printf("Creating new object from class %s\n", Name.c_str());
    return ObjectHeap->CreateObject(NewClass);
}

/**
 * A simple wrapper to create an array of objects of the given type.
 * Referenced types are stored in the Constants Pool, so it makes sense to have this in the Class.
 *
 * @param Index the Index into the Constants Pool referencing the class to be instantiated
 * @param ObjectHeap the Object Heap to store the new Object Reference Array in
 * @return the new Array of Object References of the given Class
 */
bool Class::CreateObjectArray(uint16_t Index, uint32_t Count, ObjectHeap ObjectHeap, Object &pObject) {
    std::string ClassName = GetStringConstant(Index);

    printf("Creating array of objects from class %s\n", ClassName.c_str());

    Class* newClass = this->_ClassHeap->GetClass(ClassName);
    if(newClass == nullptr) return false;

    pObject = ObjectHeap.CreateObjectArray(newClass, Count);

    return true;
}

/**
 * A simple internal function that prints all Strings in the given list.
 * Primarily used for debugging.
 * @param list the list to iterate
 */
void PrintList(std::list<std::string> &list) {
    for (auto const& i: list) {
        puts(i.c_str());
    }
}