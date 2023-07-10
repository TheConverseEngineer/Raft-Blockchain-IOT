#ifndef MAC_H
#define MAC_H

#include <stdint.h>
#include <string>
#include <sstream>

/** Represents a 42bit MAC address*/
struct MACAddress {
    uint8_t address[6];

    /** Shorthand for convenience */
    MACAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f) { 
        address[0] = a; address[1] = b; address[2] = c; address[3] = d; address[4] = e; address[5] = f;
    }

    MACAddress() {
        address[0] = 0; address[1] = 0; address[2] = 0; address[3] = 0; address[4] = 0; address[5] = 0;
    }

    bool operator==(const MACAddress& other) const {
        return address[0] == other.address[0]
           and address[1] == other.address[1]
           and address[2] == other.address[2]
           and address[3] == other.address[3]
           and address[4] == other.address[4]
           and address[5] == other.address[5];
    } 

    bool operator<(const MACAddress& other) const {
        return (address[0]==other.address[0])?(
            (address[1]==other.address[1])?(
                (address[2]==other.address[2])?(
                    (address[3]==other.address[3])?(
                        (address[4]==other.address[4])?(
                            (address[5]<other.address[5])
                        ):(address[4]<other.address[4])
                    ):(address[3]<other.address[3])
                ):(address[2]<other.address[2])
            ):(address[1]<other.address[1])
        ):(address[0]<other.address[0]);
    }

    bool isNull() { 
        return address[0] == 0
           and address[1] == 0
           and address[2] == 0
           and address[3] == 0
           and address[4] == 0
           and address[5] == 0;
    }

    std::string print() const { 
        std::stringstream stream;
        for (int i = 0; i < 6; i++) {
            stream << std::hex << +address[i] << std::dec; 
            if (i < 5) stream << "::";
        }
        return stream.str();
    }
};

#endif