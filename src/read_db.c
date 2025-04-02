#include <ctype.h>
#include <fcntl.h>
#include <ndbm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        printf("[DEBUG FETCH] value.dptr: %p, value.dsize: %d\n", value.dptr, value.dsize);

        // Check if key or value is invalid
        if(key.dptr == NULL || key.dsize <= 0 || value.dptr == NULL || value.dsize <= 0)
        {
            fprintf(stderr, "[WARN] Empty or invalid key/value detected\n");
        }
        else
        {
            // Print the key and value if they are valid
            printf("Key: %.*s\n", key.dsize, key.dptr);
            printf("Value (raw): ");
            for(int i = 0; i < value.dsize; i++)
            {
                printf("%c", isprint(value.dptr[i]) ? value.dptr[i] : '.');
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
