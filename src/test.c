#include <ctype.h>
#include <fcntl.h>
#include <ndbm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_PERMS 0644
#define MYKEY_STR "mykey"
#define MYVAL_STR "myval"

int main(void)
{
    char  db_path[] = "postdata";    // cppcheck-suppress constVariable
    char  key_buf[] = MYKEY_STR;
    char  val_buf[] = MYVAL_STR;
    datum key       = {.dptr = key_buf, .dsize = (int)(strlen(key_buf))};
    datum val       = {.dptr = val_buf, .dsize = (int)(strlen(val_buf))};
    datum result;

    DBM *db = dbm_open(db_path, O_RDWR | O_CREAT, FILE_PERMS);
    if(!db)
    {
        perror("dbm_open (write)");
        return 1;
    }

    if(dbm_store(db, key, val, DBM_REPLACE) != 0)
    {
        perror("dbm_store");
    }

    dbm_close(db);

    db = dbm_open(db_path, O_RDONLY, 0);
    if(!db)
    {
        perror("dbm_open (read)");
        return 1;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waggregate-return"
    result = dbm_fetch(db, key);
#pragma GCC diagnostic pop

    if(result.dptr == NULL)
    {
        printf("Failed to fetch value for key '%s'\n", key.dptr);
    }
    else
    {
        printf("Stored: %.*s = %.*s\n", key.dsize, key.dptr, result.dsize, result.dptr);
    }

    dbm_close(db);
    return 0;
}
