/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <vm/ZipFile.hpp>
#include <vm/Common.hpp>
#include <cstdint>
#include <iostream>
#include "Mapping.h"

/*
 *  Implements all of the functions required to read zip files, such as jars.
 *  A lot of macros are used for convenience, see the comments for an explanation as to what is going on in each.
 *  
 *  ZipFile is the core of these functions - it stores a file handle in the form of ifstream, as well as its' size.
 *  See its' comments for more details.
 * 
 *  These functions allow you to:
 *   - create a ZipFile object from a file on disk
 *   - search for a specific file or folder in the zip file
 *   - read a specific file from the Zip without decompressing the whole thing
 *   - read disparate files sequentially with little performance loss
 *  
 *  This implements enough functionality to load classes from a jar lazily.
 * 
 **/


// Read a short (2 bytes) from the active File, from the offset, setting the data into the provided target variable.
#define READ_SHORT(subject, offset, target)             \
    {                                                   \
        char subject##data[2];                          \
        File.seekg(offset);                             \
        File.read(subject##data, 2);                    \
        target = ReadShortFromStream(subject##data);    \
    }                                                   \
// Read an int (4 bytes) from the active File, from the offset, setting the data into the provided target variable.
#define READ_INT(subject, offset, target)           \
    {                                               \
        char subject##data[4];                      \
        File.seekg(offset);                         \
        File.read(subject##data, 4);                \
        target = ReadIntFromStream(subject##data);  \
    }                                               \

// A short macro to deal with an error condition while processing a jar; close the file handle and return null.
#define ERROR(x) \
    {             \
        std::cout << x << std::endl; \
        mz_zip_reader_end(zip); \
        return NULL;  \
    }

// Hash a single UTF8 character from the input stream.
#define GET_UTF8_CHAR(ptr, c)                         \
{                                                     \
    int x = *ptr++;                                   \
    if(x & 0x80) {                                    \
        int y = *ptr++;                               \
        if(x & 0x20) {                                \
            int z = *ptr++;                           \
            c = ((x&0xf)<<12)+((y&0x3f)<<6)+(z&0x3f); \
        } else                                        \
            c = ((x&0x1f)<<6)+(y&0x3f);               \
    } else                                            \
        c = x;                                        \
}

// Hash a UTF8 string as an integer.
int utf8Hash(unsigned char *utf8) {
    int hash = 0;

    while(*utf8) {
        short c;
        GET_UTF8_CHAR(utf8, c);
        hash = hash * 37 + c;
    }

    return hash;
}

/**
 * Process a ZIP archive.
 * Will walk the file dictionary, hashing the name of every file it comes across, and storing that into the map.
 * At the end, a fully formed and valid ZipFile is created.
 * This ZipFile will have all necessary data filled in.
*/

#ifndef MINE
ZipFile* ProcessArchive(const char* path) {
    mz_zip_archive* zip;
    mz_bool result;
    size_t length;

    std::vector<std::string> Names;
    std::unordered_map<int, int> Map;
    ZipFile* zfile;

    zip = new mz_zip_archive();

    result = mz_zip_reader_init_file(zip, path, 0);
    if (!result)
        ERROR("Unable to read ZIP file because " << mz_zip_get_error_string(zip->m_last_error));
    length = mz_zip_get_archive_size(zip);

    for (int i = 0; i < (int) mz_zip_reader_get_num_files(zip); i++) {
        mz_zip_archive_file_stat stat;

        if (!mz_zip_reader_file_stat(zip, i, &stat))
            ERROR("Unable to read ZIP file contents at index " << i);

        //std::cout << "Filename: " << stat.m_filename << ", Compressed Size: " << stat.m_comp_size << ", is folder: " << (stat.m_is_directory ? "true" : "false") << std::endl;
        int hash = utf8Hash((unsigned char*) stat.m_filename);
        if (!stat.m_is_directory) {
            std::cout << "Emplacing file " << stat.m_filename << " with hash " << std::hex << hash << std::dec << " at index " << i << std::endl;
            Names.emplace_back(std::string(stat.m_filename));
            Map.insert_or_assign(hash, i);
        }
    }

    zfile = new ZipFile();
    zfile->File = zip;
    zfile->Size = length;
    zfile->FileNames = Names;
    zfile->Map = Map;

    return zfile;

}
#else
ZipFile* ProcessArchive(const char* path) {
    char MagicNumber[SIGNATURE_LENGTH];

    int Entries = 0, Length = 0, Offset = 0;
    long HeaderStart = 0;
    int NextFileSignature;
    std::ifstream File;

    ZipFile* Zip;
    std::unordered_map<int, int> Map;
    std::vector<std::string> Names;

    // Open Zip file
    File = std::ifstream(path, std::ios::binary);
    if(!File.is_open()) {
        std::cout << "Unable to read file " << path << std::endl;
        return NULL;
    }

    // Verify signature. We always want to start with a local file - the manifest.
    File.read(MagicNumber, SIGNATURE_LENGTH);
    if(*reinterpret_cast<unsigned int*>(MagicNumber) != LOCAL_FILE_HEADER_SIG)
        ERROR("Start of Zip file has invalid magic: " << *reinterpret_cast<unsigned int*>(MagicNumber) << "(" << ReadIntFromStream(MagicNumber) << ")" << " expected to see " << LOCAL_FILE_HEADER_SIG);

    // Find the end of the file.
    File.seekg(0, std::ios::end);
    Length = File.tellg();

    std::cout << "File length is 0x" << std::hex << Length << std::endl;
    std::cout << "Searching for file end header 0x" << std::hex << CDR_END_SIGNATURE << std::endl;

    // Start searching backwards.
    for (Offset = Length - CDR_END_LENGTH; Offset >= 0; ) {
        // Try read the signature
        File.seekg(Offset);
        File.read(MagicNumber, SIGNATURE_LENGTH);
        std::cout << "Offset is 0x" << std::hex << Offset << ", magic is 0x" << std::hex << *reinterpret_cast<unsigned int*>(MagicNumber) << std::endl;
        fflush(stdout);
        // Check if the first byte matches, as an optimization
        if (*MagicNumber == (CDR_END_SIGNATURE & 0xff)) {
            // Read the full thing if we think it might be what we want
            if (*reinterpret_cast<unsigned int*>(MagicNumber)== CDR_END_SIGNATURE) {
                std::cout << "Found header match." << std::endl;
                // Break from the loop.
                break;
            } else
                // We found something that might be a signature, but isn't - check the next signature block instead.
                Offset -= SIGNATURE_LENGTH;
        }
        // There is definitely not a signature here, so step back 1 byte and try again, in case we landed in the middle of the sig.
        else Offset--;
    }

    // If Offset is <0, we got back to the start without finding the sig. It's possible to find the signature AT 0, though, so we must be careful with this check.
    if (Offset < 0)
        ERROR("Unable to find dictionary header in zip file.");

    // We found the signature, so we know the base of the CDR.
    File.seekg(Offset);
    HeaderStart = File.tellg();
    std::cout << "Header starts at offset 0x" << std::hex << HeaderStart << std::endl;
    // Now read in how many entries there are.
    READ_SHORT(entries, HeaderStart + CDR_END_OFFSET, Entries)
    std::cout << "Found " << std::dec << Entries << " files in the final folder." << std::endl;
    std::cout << "Header starts at 0x" << std::hex << HeaderStart << std::endl;

    // Now seek to the start of the first file in the first folder.
    READ_INT(file, HeaderStart + CDR_END_DIR_OFFSET, Offset)
    std::cout << "Header says the final directory is at 0x" << std::hex << Offset << std::endl;

    if (Offset + SIGNATURE_LENGTH > Length)
        ERROR("Malformed zip file - signature is longer than the file.");

    std::cout << "Expecting header 0x" << std::hex << CDR_FILE_HEADER_SIG << std::endl;

    // Iterate every entry in the zip file.
    while (Entries--) {
        int pathLength, commentLength, extraLength, found;
        std::cout << "Folder index " << Entries << std::endl;

        // Sanity check the length.
        if (Offset + CDR_FILE_HEADER_LEN > Length)
            ERROR("Malformed zip file - file header is longer than the file.");


        READ_INT(signature, Offset, NextFileSignature)
        std::cout << "The signature of the next file is 0x" << std::hex << NextFileSignature << std::endl;

        // Sanity check the signature. Once we stop reading files, we're out.
        //if (NextFileSignature != CDR_FILE_HEADER_SIG)
        //    ERROR("Malformed zip file - sequential headers are not file signatures.");

        // The length of the path. The path comes immediately after the header.
        READ_SHORT(path, Offset + CDR_FILE_PATHLEN_OFFSET, pathLength)

        // Header fields. We don't want the data, but we want to skip them.
        READ_SHORT(header, Offset + CDR_FILE_EXTRALEN_OFFSET, extraLength)
        READ_SHORT(comment, Offset + CDR_FILE_COMMENTLEN_OFFSET, commentLength)

        // Step forward.
        Offset += CDR_FILE_HEADER_LEN;

        // Read in the file path.
        char* filepath = new char[pathLength + 1];
        File.seekg(Offset);
        File.read(filepath, pathLength + 1);
        // Manually terminate the path.
        // When we search for this later on, the terminator will come included, so we must add it to the hash.
        filepath[pathLength] = '\0';
        // Add the path to the list we save
        Names.emplace_back(std::string(filepath));
        std::cout << "Name is " << filepath << std::endl;
        // Save the location
        found = Offset;

        // Move on with the search
        Offset += pathLength;
        Offset += extraLength;
        Offset += commentLength;

        // Sanity check the zip - there should always be more bytes.
        // TODO: is this leaking that filepath we just allocated?
        if (Offset + SIGNATURE_LENGTH > Length)
            ERROR("Malformed zip file - the end of a file reaches EOF without extra bytes.");

        Map.emplace(utf8Hash((unsigned char*) filepath), found);

        delete[] filepath;
    }

    Zip = new ZipFile;
    Zip->File = &File;
    Zip->Size = Length;
    Zip->HashTable = Map;
    Zip->FileNames = Names;

    return Zip;
}
#endif

/**
 * Return the raw (decompressed) bytes for the given file in the zip.
 * Will not decompress anything other than the file you ask for.
 * Is safe to call rapidly.
 * Handles both compressed and uncompressed files gracefully.
*/
char* GetFileInZip(char* name, ZipFile* zip, size_t& size) {
    void* data;
    int hash = utf8Hash((unsigned char*) name);
    std::cout << "Attempting to find file " << name << " in zip." << std::endl;
    std::cout << "Path hash is " << std::hex << hash << std::dec << ", contained: " << (zip->Map.contains(hash) ? "true" : "false") << std::endl;
    data = mz_zip_reader_extract_to_heap(zip->File, zip->Map.at(hash), &size, 0);
    return (char*) data;

}