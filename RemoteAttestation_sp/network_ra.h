#ifndef REMOTE_ATTESTATION_NETWORK_RA_H__
#define REMOTE_ATTESTATION_NETWORK_RA_H__

#include <stdint.h>
#include <string>
#include <vector>

struct RaNetworkMessage {
    uint32_t message_type;
    std::vector<uint8_t> payload;
};

class RaNetworkChannel {
public:
    bool send(const RaNetworkMessage& message);
    bool receive(uint32_t expected_message_type, RaNetworkMessage& message);
    const std::string& last_error() const;

private:
    std::string last_error_;
};

#endif
