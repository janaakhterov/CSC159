#include "k-const.h"
#include "k-data.h"
#include "sys-call.h"

// Phase3: Request/alloc vid_mux using the new service MuxCreateCall().
void InitProc(void) {
    int i;
    while(1) {
        // show a dot at upper-left corener of PC
        ShowCharCall(0, 0, '.');
        // wait for about half a second
        for(i=0; i<LOOP/2; i++) asm("inb $0x80");

        // erase dot
        ShowCharCall(0, 0, ' ');
        // wait for about half a second
        for(i=0; i<LOOP/2; i++) asm("inb $0x80");
    }
}

// Phase3: Experiment with MuxOpCall() calls to lock and unlock vid_mux
// in the while(1) loop.
void UserProc(void) {
    // Get my pid from sys call
    int pid = GetPidCall();
    int i;
    int _pid = pid;
    while(1) {
        int length;

        // The maximum length of a 4 byte int is 10 characters.
        // The string that will contain the number we print
        char chars[11] = "0000000000";

        // We need a copy of pid to get the string from
        // because this mutates the variable.
        _pid = pid;
        length = 0;

        // For every digit in the number we put it into the array
        // This moves right to left which means the number actual
        // number would be backwards in the array
        while(_pid / 10 > 0) {
            chars[length] = (char)((_pid % 10) + '0');
            _pid /= 10;
            length++;
        } 

        // Get the last digit in case the number < 10
        chars[length] = (char)((_pid % 10) + '0');
        length++;

        // Print the number one string at a time
        for(i = 0; i < length; i++) {
            ShowCharCall(pid + 1, length - i - 1, chars[i]);
        }

        SleepCall(50);

        // Overwrite the same locations with spaces to effectively "erase" the number
        for(i = 0; i < length; i++) {
            ShowCharCall(pid + 1, length - i - 1, ' ');
        }

        SleepCall(50);
    }
}
