/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <unordered_map>
#include <zlib.h>

/**
 * A Java-compatible archive file.
 * ZIP compression, usually .zip or .jar.
 * Decomposes to a hash table, specifying data pointers for each file and folder in the file.
 */
struct ZipFile {
    int Size;
    std::ifstream* File;
    std::unordered_map<int, int> HashTable;
};

/************* ZLIB Defines *************/

#define MAX_WINDOW_BITS 15

#define SIGNATURE_LENGTH           4

/* End of central directory record */
#define CDR_END_SIGNATURE          0x06054b50
#define CDR_END_LENGTH             22
#define CDR_END_OFFSET             8
#define CDR_END_DIR_OFFSET         16

/* Central directory file header */
#define CDR_FILE_HEADER_SIG        0x02014b50
#define CDR_FILE_HEADER_LEN        46
#define CDR_FILE_COMPMETH_OFFSET   10
#define CDR_FILE_COMPLEN_OFFSET    20
#define CDR_FILE_UNCOMPLEN_OFFSET  24
#define CDR_FILE_PATHLEN_OFFSET    28
#define CDR_FILE_EXTRALEN_OFFSET   30
#define CDR_FILE_COMMENTLEN_OFFSET 32
#define CDR_FILE_LOCALHDR_OFFSET   42

/* Local file header */
#define LOCAL_FILE_HEADER_SIG      0x04034b50
#define LOCAL_FILE_HEADER_LEN      30
#define LOCAL_FILE_EXTRA_OFFSET    28

/* Supported compression methods */
#define COMPRESSION_STORED         0
#define COMPRESSION_DEFLATED       8

// Read the data of the ZIP/JAR/WAR file, decompose to a hash table of contents.
ZipFile* ProcessArchive(char* path);
// Get a file offset for a file in the given archive.
int FindZipFileOffset(char* folder, ZipFile* zip);
// Get the raw data for a file in the given archive.
char* GetFileInZip(char* file, ZipFile* zip, int& size);