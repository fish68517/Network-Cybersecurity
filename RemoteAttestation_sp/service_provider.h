#ifndef REMOTE_ATTESTATION_SERVICE_PROVIDER_H__
#define REMOTE_ATTESTATION_SERVICE_PROVIDER_H__

#include <stdint.h>
#include <string>
#include <vector>

#include "attestation_policy.h"

struct AttestationRequest {
    uint32_t message_type;
    std::vector<uint8_t> payload;
};

struct AttestationResponse {
    bool trusted;
    uint32_t message_type;
    std::string detail;
    std::vector<uint8_t> payload;
};

class RemoteAttestationServiceProvider {
public:
    RemoteAttestationServiceProvider();

    void set_policy(const AttestationPolicy& policy);
    AttestationResponse process_request(const AttestationRequest& request) const;
    AttestationResponse build_msg2_response(const AttestationRequest& request) const;
    AttestationResponse build_attestation_result(bool trusted) const;

private:
    AttestationPolicy policy_;
};

#endif
