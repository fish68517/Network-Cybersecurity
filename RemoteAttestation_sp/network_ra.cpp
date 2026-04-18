#include "network_ra.h"

bool RaNetworkChannel::send(const RaNetworkMessage& message)
{
    (void)message;
    last_error_ = "placeholder transport";
    return true;
}

bool RaNetworkChannel::receive(uint32_t expected_message_type, RaNetworkMessage& message)
{
    (void)expected_message_type;
    message.message_type = 0;
    message.payload.clear();
    last_error_ = "placeholder transport";
    return false;
}

const std::string& RaNetworkChannel::last_error() const
{
    return last_error_;
}
