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

/*
    Commander.cpp

    A simple command line processor and callback for a serial based shell
*/

#include "commander.h"
#include <algorithm>

#ifndef MBED_CONF_APP_COMMANDER_ISR_BUFFER_LENGTH
#define MBED_CONF_APP_COMMANDER_ISR_BUFFER_LENGTH 16
#endif

Commander::Commander(PinName tx,
                     PinName rx,
                     int baud) : _serial(tx,rx)
{
    //set the prompt up
    _prompt = "> ";

    //create our banner
    _banner += "\n\n\n";
    _banner += "\n";
    _banner += ".___  ___. .______    _______  _______  \n";
    _banner += "|   \\/   | |   _  \\  |   ____||       \\ \n";
    _banner += "|  \\  /  | |  |_)  | |  |__   |  .--.  |\n";
    _banner += "|  |\\/|  | |   _  <  |   __|  |  |  |  |\n";
    _banner += "|  |  |  | |  |_)  | |  |____ |  '--'  |\n";
    _banner += "|__|  |__| |______/  |_______||_______/ \n";
    _banner += "\n";
    _banner += "    Created by: The ARM Red Team\n";
    _banner += "\n\n\n";

    //set the line rate of the serial
    _serial.baud(baud);

    //hook up our help
    add("help",
        "Get help about the available commands.",
        callback(this, &Commander::help));
}

Commander::~Commander()
{

}

void Commander::help(vector<string>& params)
{
    map<string, Command>::const_iterator it;

    printf("Help:\n");

    //walk our commands and print the cmd and it's description for help
    for (it = _cmds.begin(); it != _cmds.end(); ++it) {
        printf("%-12s - %s\n",
               it->second.strname.c_str(),
               it->second.strdesc.c_str());
    }
}

int Commander::add(string strname, string strdesc, pFuncCB pcallback)
{
    int nreturn = 0;

    Command mycmd;

    mycmd.strname = strname;
    mycmd.strdesc = strdesc;
    mycmd.pCB     = pcallback;

    _cmds[strname] = mycmd;

    return nreturn;
};

void Commander::banner()
{
    //print our welcome banner
    printf(_banner.c_str());
}

void Commander::input_handler()
{
    //push our input into the buffer.
    //avoid increasing the size of the vector since this
    //function runs in interrupt context.
    if (_buffer.size() < _buffer.capacity()) {
        _buffer.push_back(_serial.getc());
    }

    //walk the callbacks and call them one at a time
    for (size_t n = 0; n < _vready.size(); n++) {
        _vready[n]();
    }
}

void Commander::on_ready(pFuncReady cb)
{
    //push it real good
    _vready.push_back(cb);
}

void Commander::del_ready(pFuncReady cb)
{
    //find our callback
    std::vector<pFuncReady>::iterator it = std::find(_vready.begin(), _vready.end(), cb);

    //if this is it then kill it
    if (it != _vready.end()) {
        _vready.erase(it);
    }
}

void Commander::init()
{
    //reserve space for characters read from the serial interrupt routine
    //so that it doesn't need to allocate memory.
    _buffer.reserve(MBED_CONF_APP_COMMANDER_ISR_BUFFER_LENGTH);

    //hook up our serial input handler to the serial interrupt
    _serial.attach(callback(this, &Commander::input_handler));

    //print the prompt!
    printf(_prompt.c_str());
};

bool Commander::pump()
{
    bool breturn = false;

    //did the user press a key?
    if (_buffer.size() > 0) {

        //signal we got data
        breturn = true;

        //get the the key and echo it back
        int nInput = _buffer.front();

        _buffer.erase(_buffer.begin());

        //if this is enter then process!
        if (nInput == 13) {
            //do we have a blank command?
            if (_strcommand.length() == 0) {
                printf("\n");
            } else {
                process(_strcommand);
                //clear the command
                _strcommand = "";
            }

            //print the prompt!
            printf(_prompt.c_str());

        } else if (nInput == 8 && _strcommand.length() > 0) {
            //if this is delete then truncate our string!
            //remove last char in our command string
            _strcommand = _strcommand.substr(0, _strcommand.length() - 1);

            //print the output to the backspace
            _serial.putc(nInput);
            _serial.putc(' ');
            _serial.putc(nInput);

        } else {
            // we are adding to our string
            if (nInput > 31 && nInput < 127) {
                _strcommand += (char)nInput;
                //print the output to the serial!
                _serial.putc(nInput);
            }
        }
    }

    return breturn;
};

int Commander::process(string& strcommand)
{
    int nreturn = 0;
    map<string, Command>::const_iterator it;

    //where our cracked string goes
    vector<string> lsresults;

    //break the command apart
    tokenize(strcommand, lsresults);

    //if we got anything
    if (lsresults.size() > 0) {
        printf("\n");

        //get our element
        it = _cmds.find(lsresults[0]);
        if (it != _cmds.end()) {
            //call our guy
            it->second.pCB(lsresults);
        } else {
            //we have an unknown command
            printf("Error Unknown Command!\n");
            for (vector<string>::iterator it = lsresults.begin();
                 it != lsresults.end();
                 ++it) {
                printf(it->c_str());
                printf("\n");
            }
        }
    }

    return nreturn;
}

void Commander::tokenize(string& strcmd, vector<string>& lsresult)
{
    //our temp string builder
    std::string strtemp;

    //lets walk our string and crack it!
    for (unsigned int n = 0; n < strcmd.length(); n++) {
        //do we have our delimiter?
        if (strcmd[n] == ' ') {
            //add it to our list
            lsresult.push_back(strtemp);
            strtemp = "";
        } else {
            //if not add to our string!
            strtemp += strcmd[n];
        }
    }

    //add our last string
    lsresult.push_back(strtemp);
}
