#ifndef DEFAULT_PARAMETERS_H
#define DEFAULT_PARAMETERS_H

#include <string>
#include <vector>

const std::vector<std::string> defaultParameters = {
    "PARAMETER",
    "0.15",   // kP 5000
    "0.0001", // kI
    "0.001", // kD
    "1.00",  // targetTunnelCurrentnA
    "0.3",   // toleranceTunnelCurrentnA
    "0",     // startX
    "0",     // startY
    "1",     // measure milliseconds
    "0",     // direction
    "199",   // maxX
    "199",   // maxY
    "100"    // multiplicator
};

#endif // DEFAULT_PARAMETERS_H