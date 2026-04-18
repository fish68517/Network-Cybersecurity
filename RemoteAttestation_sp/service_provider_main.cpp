#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "../MedicalSystemDemo/shared_types.h"
#include "service_provider.h"

namespace {

bool read_message_file(const char* path, AttestationRequest& request)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }

    uint32_t message_type = 0;
    uint32_t payload_size = 0;
    input.read(reinterpret_cast<char*>(&message_type), sizeof(message_type));
    input.read(reinterpret_cast<char*>(&payload_size), sizeof(payload_size));
    if (!input || payload_size > MSD_RA_MAX_MESSAGE_SIZE) {
        return false;
    }

    request.message_type = message_type;
    request.payload.assign(payload_size, 0);
    if (payload_size > 0) {
        input.read(reinterpret_cast<char*>(&request.payload[0]), payload_size);
    }

    return input.good() || input.eof();
}

bool write_message_file(const char* path, const AttestationResponse& response)
{
    const uint32_t payload_size = static_cast<uint32_t>(response.payload.size());
    if (payload_size > MSD_RA_MAX_MESSAGE_SIZE) {
        return false;
    }

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        return false;
    }

    output.write(reinterpret_cast<const char*>(&response.message_type), sizeof(response.message_type));
    output.write(reinterpret_cast<const char*>(&payload_size), sizeof(payload_size));
    if (payload_size > 0) {
        output.write(reinterpret_cast<const char*>(&response.payload[0]), payload_size);
    }

    return output.good();
}

} // namespace

int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cerr << "[RA-SP] Usage: RemoteAttestation_sp.exe <request_file> <response_file>" << std::endl;
        return 1;
    }

    AttestationRequest request;
    if (!read_message_file(argv[1], request)) {
        std::cerr << "[RA-SP] Failed to read request file: " << argv[1] << std::endl;
        return 2;
    }

    RemoteAttestationServiceProvider provider;
    AttestationResponse response = provider.process_request(request);

    if (!write_message_file(argv[2], response)) {
        std::cerr << "[RA-SP] Failed to write response file: " << argv[2] << std::endl;
        return 3;
    }

    std::cout << "[RA-SP] Processed message type: " << request.message_type << " -> " << response.message_type << std::endl;
    std::cout << "[RA-SP] Result: " << response.detail << std::endl;
    return 0;
}
