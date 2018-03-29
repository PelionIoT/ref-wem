/* mbed Microcontroller Library
 * Copyright (c) 2018 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "fs.h"
#include <string>
#include <vector>
#include <map>

/*
    class: keystore

    simple keystore database class to store name-value pairs in on the mBed pal filesystem

    basic ussage:

    int main()
    {
        //create the class
        keystore k;

        //read the existing database
        k.open();

        //set a couple of keys
        k.set("key1", "monkeytoes!");
        k.set("newkey", "donkeytail!");

        //get a keys value
        string val = k.get("testkey");

        //write the changes out
        k.write();

        return 0;
    }
*/
class Keystore
{
public:

    /*
        constructor
    */
    Keystore();

    /*
        destructor
    */
    ~Keystore();

    /*
     * performs initialization of underlying storage
     */
    static int init();

    /*
     * performs de-initialization of underlying storage
     */
    static void shutdown();

    /*
        Function: open

        opens the database and pulls the values internally to this class

        Params:
        none.

        Returns:
        nothing.
    */
    void open();

    /*
        Function: write

        writes the values back to storage.

        Params:
        none.

        Returns:
        nothing.
    */
    void write();

    /*
        Function: close

        closes the database

        Params:
        none.

        Returns:
        nothing.
    */
    void close() { return; };

    /*
        Function: get

        Get the value of the given key

        Params:
        char* szkey - the key to get the value for

        Returns:
        std::string of the value
    */
    std::string get(const char* szkey);

    /*
        Function: set

        Set the value of the given key

        Params:
        char* szkey     - the key to set the value for
        char* szvalue   - the value to set

        Returns:
        nothing
    */
    void set(const char* szkey, const char* szvalue);

    /*
        Function: get

        Get the value of the given key

        Params:
        std::string strkey - the key to get the value for

        Returns:
        std::string of the value
    */
    std::string get(std::string& strkey);

    /*
        Function: set

        Set the value of the given key

        Params:
        std::string strkey      - the key to get the value for
        std::string strvalue    - the key to get the value for

        Returns:
        nothing.
    */
    void set(const std::string& strkey, const std::string& strvalue);

    /*
        Function: del

        Delete the given key

        Params:
        char* szkey- the key to delete

        Returns:
        nothing
    */
    void del(const char* szkey);

    /*
        Function: del

        Delete the given key

        Params:
        std::string strkey- the key to delete

        Returns:
        nothing
    */
    void del(std::string& strkey);

    /*
        Function: exists

        Check if the key exists

        Params:
        char* szkey - the key to check

        Returns:
        true = exists / false = doesn't exist
    */
    bool exists(const char* szkey);

    /*
        Function: exists

        Check if the key exists

        Params:
        std::string strkey - the key to check

        Returns:
        true = exists / false = doesn't exist
    */
    bool exists(std::string& strkey);

    /*
        Function: to_db

        convert the give text string to our internal class DB that can
        be stored by to_file

        Params:
        std::string& strfile - the DB file read from storage

        Returns:
        nothing.
    */
    void to_db(std::string& strfile);

    /*
        Function: keys

        Get all the keys in the keystore

        Params:
        none.

        Returns:
        std::vector<std::string> of all the keys in the DB
    */
    std::vector<std::string> keys();

    /*
        Function: to_file()

        convert the internal DB into a file ready storage format that can be
        read by to_db

        Params:
        none.

        Returns:
        std::string with the text to write to the file in storage
    */
    std::string to_file();

    /*
        Function: tokenize

        Standard String tokenizer similar to strtok, without the
        inplace string shenanigans

        Params:
        std::string& strin                  - input string to break apart
        std::vector<std::string>& lsresult  - the returned array of strings broken by the delimiter
        char token                          - the character to break the string on

        Returns:
        nothing.
    */
    void tokenize(std::string& strin,
                  std::vector<std::string>& lsresult,
                  char token);

    /*
        Function: kill_all

        kill all the value and reset the database to nothing

        Params:
        none.

        Returns:
        nothing.
    */
    void kill_all();

protected:
    /*
        variable: std::map<std::string, std::string> _mapdb

        the key value map of all the data loaded from the keystore file
    */
    std::map<std::string, std::string> _mapdb;

    /*
        variable: std::string _strfilepath

        the absolute path to the database file
    */
    static std::string _strfilepath;

    /*
        variable: std::string _strdir

        the default directory for the database file
    */
    static std::string _strdir;

    static char *mktmp(char *out);
};


