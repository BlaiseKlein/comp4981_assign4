#include <ctype.h>
#include <fcntl.h>
#include <ndbm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__APPLE__)                                    // For macOS only
    #define CAST_KEY_DSIZE(key_dsize) (int)(key_dsize)    // On macOS, we cast it to int
#else
    #define CAST_KEY_DSIZE(key_dsize) (key_dsize)    // On Linux, dsize is already int
#endif
#if defined(__APPLE__)    // macOS needs a cast from void* to char*
    #define PRINT_KEY_PTR(ptr) ((char *)(ptr))
#else    // Linux: dptr is already a char*, no cast needed
    #define PRINT_KEY_PTR(ptr) (ptr)
#endif
#ifdef __APPLE__
    #define DBDATA(d) ((char *)(d))
#else
    #define DBDATA(d) (d)
#endif
// Use this to safely cast dsize when needed (only if it's unsigned)
#define DBDSIZE(d) ((int)(d))

int main(void)
{
    DBM  *db;
    datum key;

    char db_path[] = "postdata";    // cppcheck-suppress constVariable

    // Open the database in read-only mode
    db = dbm_open(db_path, O_RDONLY, 0);
    if(!db)
    {
        perror("dbm_open");
        return 1;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waggregate-return"

    // Start iterating through the keys in the database
    key = dbm_firstkey(db);    // Get the first key
    while(key.dptr != NULL)    // Iterate through all keys
    {
        datum value = dbm_fetch(db, key);    // Fetch the value associated with the current key
        // printf("[DEBUG FETCH] value.dptr: %p, value.dsize: %d\n", value.dptr, value.dsize);

        // Check if key or value is invalid
        if(key.dptr == NULL || key.dsize <= 0 || value.dptr == NULL || value.dsize <= 0)
        {
            fprintf(stderr, "[WARN] Empty or invalid key/value detected\n");
        }
        else
        {
            const char *val_ptr = DBDATA(value.dptr);
            // Print the key and value if they are valid
            printf("Key: %.*s\n", CAST_KEY_DSIZE(key.dsize), PRINT_KEY_PTR(key.dptr));
            printf("Value (raw): ");
            for(int i = 0; i < DBDSIZE(value.dsize); i++)
            {
                printf("%c", isprint((unsigned char)val_ptr[i]) ? val_ptr[i] : '.');
            }
            printf("\n\n");
        }

        // Move to the next key
        key = dbm_nextkey(db);    // Corrected to call dbm_nextkey without passing the key
    }

#pragma GCC diagnostic pop

    // Close the database
    dbm_close(db);
    return 0;
}
