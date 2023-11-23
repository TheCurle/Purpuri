/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <unordered_map>
#include <fstream>
#include <vector>
#include <string>
#include <zlib.h>
#include <miniz.h>

/**
 * A Java-compatible archive file.
 * ZIP compression, usually .zip or .jar.
 * Decomposes to a hash table, specifying data pointers for each file and folder in the file.
 */
struct ZipFile {
    size_t Size;
    mz_zip_archive* File;
    std::unordered_map<int, int> Map;
    std::vector<std::string> FileNames;
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
ZipFile* ProcessArchive(const char* path);
// Get a file offset for a file in the given archive.
int FindZipFileOffset(char* folder, ZipFile* zip);
// Get the raw data for a file in the given archive.
char* GetFileInZip(char* file, ZipFile* zip, size_t& size);
int utf8Hash(unsigned char *utf8);