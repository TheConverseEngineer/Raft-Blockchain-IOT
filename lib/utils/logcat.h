#ifndef LOGCAT_H
#define LOGCAT_H

#include <string>
#include <iostream>
#include <mutex>
#include "mac.h"
#include "entry.h"

extern bool ext_withTimestamp;
extern long long (*ext_pFunc)();

namespace Log {

    static std::mutex PRIVATE_m;
    
    /** This is the important method (everything else calls this) */
    static void log(const std::string& caller, const std::string& data) {
        PRIVATE_m.lock();
        if (!ext_withTimestamp) std::cout << "[" << caller << "] " << data << "\n";
        else std::cout << "<" << std::to_string(ext_pFunc()) << "> [" << caller << "] " << data << "\n";
        PRIVATE_m.unlock();
    }

    /* ******************** EVERYTHING PAST THIS POINT IS JUST FOR CONVENIANCE ****************************** */
    static inline void log(const std::string& caller, const std::string& label, const std::string& data) {
        log(caller, label + ": " + data);
    }

    static inline void log(const std::string& caller, const std::string& label, const MACAddress& data) {
        log(caller, label + ": " + data.print());
    }

    static inline void log(const std::string& caller, const std::string& label, const int data) {
        log(caller, label + ": " + std::to_string(data));
    }

    static inline void log(const std::string& caller, const std::string& label, const long long data) {
        log(caller, label + ": " + std::to_string(data));
    }

    static inline void log(const std::string& caller, const std::string& label, const unsigned char data) {
        log(caller, label + ": " + std::to_string(int(data)));
    }

    static inline void log(const MACAddress& caller, const std::string& label, const std::string& data) {
        log(caller.print(), label + ": " + data);
    }

    static inline void log(const MACAddress& caller, const std::string& label, const MACAddress& data) {
        log(caller.print(), label + ": " + data.print());
    }

    static inline void log(const MACAddress& caller, const std::string& label, const int data) {
        log(caller.print(), label + ": " + std::to_string(data));
    }

    static inline void log(const MACAddress& caller, const std::string& label, const long long data) {
        log(caller.print(), label + ": " + std::to_string(data));
    }

    static inline void log(const MACAddress& caller, const std::string& label, const unsigned char data) {
        log(caller.print(), label + ": " + std::to_string(int(data)));
    }

    static inline void log(const MACAddress& caller, const std::string& label, Entry data) {
        log(caller.print(), label + ": " + data.storageLocation.print() + " at " + std::to_string(data.time));
    }
}

#endif