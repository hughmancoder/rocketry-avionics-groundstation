#ifndef __TELEMETRY_MOCK_H
#define __TELEMETRY_MOCK_H

#include "telemetry_packet.h"

/**
 * @brief Generates mock telemetry sensor data (simulating IMU, Temp, Altitude).
 * @param pkt Pointer to TelemetryPacket structure to populate.
 */
void Telemetry_GenerateMockPacket(TelemetryPacket *pkt);

#endif /* __TELEMETRY_MOCK_H */
