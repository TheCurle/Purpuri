/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <vm/ZipFile.hpp>
#include <vm/Common.hpp>
#include <cstdint>
#include <iostream>
#include <fstream>

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
#define ERROR \
    {             \
        File.close(); \
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
ZipFile* ProcessArchive(char* path) {
    char MagicNumber[SIGNATURE_LENGTH];
    uint8_t* Data, *Ptr;

    int Entries = 0, Length = 0, Offset = 0, HeaderStart = 0;
    int NextFileSignature;
    std::ifstream File;

    ZipFile* Zip;
    std::unordered_map<int, int> Map;
    
    // Open Zip file
    File = std::ifstream(path, std::ios::binary);
    if(!File.is_open())
        return NULL;

    // Verify signature. We always want to start with a local file - the manifest.
    File.read(MagicNumber, SIGNATURE_LENGTH);
    if(ReadIntFromStream(MagicNumber) != LOCAL_FILE_HEADER_SIG)
        ERROR;

    // Find the end of the file.
    File.seekg(std::ios::end);
    Length = File.tellg();

    // Start searching backwards.
    for (Offset = Length - CDR_END_LENGTH; Offset >= 0; ) {
        // Try read the signature
        File.read(MagicNumber, SIGNATURE_LENGTH);
        // Check if the first byte matches, as an optimization
        if (*MagicNumber == (CDR_END_SIGNATURE & 0xff)) {
            // Read the full thing if we think it might be what we want
            if (ReadIntFromStream(MagicNumber) == CDR_END_SIGNATURE)
                // Break from the loop.
                break;
            else
                // We found something that might be a signature, but isn't - check the next signature block instead.
                Offset -= SIGNATURE_LENGTH;
        } 
        // There is definitely not a signature here, so step back 1 byte and try again, in case we landed in the middle of the sig.
        else Offset--;
    }

    // If Offset is <0, we got back to the start without finding the sig. It's possible to find the signature AT 0, though, so we must be careful with this check.
    if (Offset < 0) 
        ERROR;

    // We found the signature, so we know the base of the CDR.
    File.seekg(Offset);
    HeaderStart = File.tellg();
    // Now read in how many entries there are.
    // The scope allows the array to be destroyed from stack.
    READ_SHORT(entries, HeaderStart + CDR_END_OFFSET, Entries)

    // Now seek to the start of the first file in the first folder.
    READ_INT(file, HeaderStart + CDR_END_DIR_OFFSET, Offset)

    if (Offset + SIGNATURE_LENGTH > Length) 
        ERROR;

    // Iterate every entry in the zip file.
    while (Entries--) {
        char* name;
        int pathLength, commentLength, extraLength, found;

        // Sanity check the length.
        if (Offset + CDR_FILE_HEADER_LEN > Length)
            ERROR;

            
        READ_INT(signature, Offset, NextFileSignature)

        // Sanity check the signature. Once we stop reading files, we're out.
        if (NextFileSignature != CDR_FILE_HEADER_SIG) 
            ERROR;
        
        // The length of the path. The path comes immediately after the header.
        READ_SHORT(path, Offset + CDR_FILE_PATHLEN_OFFSET, pathLength)

        // Header fields. We don't want the data, but we want to skip them.
        READ_SHORT(header, Offset + CDR_FILE_EXTRALEN_OFFSET, extraLength)
        READ_SHORT(comment, Offset + CDR_FILE_COMMENTLEN_OFFSET, commentLength)

        // Step forward.
        Offset += CDR_FILE_HEADER_LEN;

        // Read in the file path.
        char* filepath = (char*) malloc(pathLength + 1);
        File.seekg(Offset);
        File.read(filepath, pathLength + 1);
        // Manually terminate the path.
        // When we search for this later on, the terminator will come included, so we must add it to the hash.
        filepath[pathLength] = '\0';
        // Save the location
        found = Offset;

        // Move on with the search
        Offset += pathLength;
        Offset += extraLength;
        Offset += commentLength;

        // Sanity check the zip - there should always be more bytes.
        // TODO: is this leaking that filepath we just allocated?
        if (Offset + SIGNATURE_LENGTH > Length)
            ERROR;

        Map.insert(utf8Hash((unsigned char*) filepath), found);
        
        delete filepath;
    }

    Zip = new ZipFile;
    Zip->File = &File;
    Zip->Size = Length;
    Zip->HashTable = Map;
}

/**
 * Find the offset of a file with the given name inside the zip file.
 * The return value is expected to be a valid input to ifstream::seekg.
*/
int FindZipFileOffset(char* name, ZipFile* zip) {

    int hash = utf8Hash((unsigned char*) name);
    if (zip->HashTable.contains(hash))
        return zip->HashTable.at(hash);
    
    return -1;
}

/**
 * Return the raw (decompressed) bytes for the given file in the zip.
 * Will not decompress anything other than the file you ask for.
 * Is safe to call rapidly.
 * Handles both compressed and uncompressed files gracefully.
*/
char* GetFileInZip(char* name, ZipFile* zip, int& size) {
    int Directory, Offset, extraLen, pathLen, compressedLen, compression;
    char* CompressedData, *DecompressionBuffer;
    
    // Find the file's folder first.
    if ((Directory = FindZipFileOffset(name, zip)) == -1)
        return NULL;

    // Found the file. Let's read more.
    std::ifstream File = std::move(*zip->File);
    READ_INT(dir, Directory + (CDR_FILE_LOCALHDR_OFFSET - CDR_FILE_HEADER_LEN), Offset)
    if (Offset + LOCAL_FILE_HEADER_LEN > zip->Size)
        return NULL;
    
    // Read the length of the extra data
    READ_SHORT(extra, Offset + LOCAL_FILE_EXTRA_OFFSET, extraLen)
    // Read the length of the path length
    READ_SHORT(path, Directory + (CDR_FILE_PATHLEN_OFFSET - CDR_FILE_HEADER_LEN), pathLen);
    // Read the uncompressed file length, pass out
    READ_INT(size, Directory + (CDR_FILE_UNCOMPLEN_OFFSET - CDR_FILE_HEADER_LEN), size)
    // Read the compressed file length
    READ_INT(size, Directory + (CDR_FILE_COMPLEN_OFFSET - CDR_FILE_HEADER_LEN), compressedLen)
    // Read the compression method
    READ_SHORT(compression, Directory + (CDR_FILE_COMPMETH_OFFSET - CDR_FILE_HEADER_LEN), compression)

    // Calculate start of data
    Offset += LOCAL_FILE_HEADER_LEN + pathLen + extraLen;

    // Bounds check
    if (Offset + compressedLen > zip->Size)
        return NULL;
    
    // Read the compressed data.
    CompressedData = new char[compressedLen];
    File.seekg(Offset);
    File.read(CompressedData, compressedLen);


    switch(compression) {
        case COMPRESSION_STORED:
            // No compression. Just return data.
            return CompressedData;
        case COMPRESSION_DEFLATED:
            // zlib compression. Inflate as normal.
            z_stream stream;
            int error;
            
            // Prepare decompression.
            DecompressionBuffer = new char[size];

            stream.next_in = (unsigned char*) CompressedData;
            stream.avail_in = compressedLen;
            stream.next_out = (unsigned char*) DecompressionBuffer;
            stream.avail_out = size;

            stream.zalloc = Z_NULL;
            stream.zfree = Z_NULL;

            if (inflateInit2(&stream, -MAX_WINDOW_BITS) != Z_OK) {
                delete DecompressionBuffer;
                delete CompressedData;
                return NULL;
            }

            error = inflate(&stream, Z_SYNC_FLUSH);
            inflateEnd(&stream);

            // The "proper" return path. This is where valid data leaves.
            // All other exit paths are errors.
            if (error = Z_STREAM_END || (error == Z_OK && stream.avail_in == 0)) {
                delete CompressedData;
                return DecompressionBuffer;
            }
            break;

        default:
            break;

    }

    delete DecompressionBuffer;
    delete CompressedData;
    return NULL;

}