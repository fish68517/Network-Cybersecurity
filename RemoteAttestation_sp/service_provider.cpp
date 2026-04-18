#include "service_provider.h"

#include "../MedicalSystemDemo/shared_types.h"

#include <string.h>

namespace {

std::vector<uint8_t> text_payload(const char* text)
{
    const char* source = text != NULL ? text : "";
    return std::vector<uint8_t>(source, source + strlen(source));
}

} // namespace

RemoteAttestationServiceProvider::RemoteAttestationServiceProvider()
{
}

void RemoteAttestationServiceProvider::set_policy(const AttestationPolicy& policy)
{
    policy_ = policy;
}

AttestationResponse RemoteAttestationServiceProvider::process_request(const AttestationRequest& request) const
{
    if (request.message_type == MSD_RA_MSG1) {
        return build_msg2_response(request);
    }

    if (request.message_type == MSD_RA_MSG3) {
        return build_attestation_result(true);
    }

    AttestationResponse response;
    response.trusted = false;
    response.message_type = MSD_RA_MSG_NONE;
    response.detail = "service_provider: unsupported message type";
    response.payload = text_payload("FAIL");
    return response;
}

AttestationResponse RemoteAttestationServiceProvider::build_msg2_response(const AttestationRequest& request) const
{
    AttestationResponse response;
    response.trusted = true;
    response.message_type = MSD_RA_MSG2;
    response.detail = "service_provider: generated placeholder Msg2";
    response.payload = text_payload("MSD_RA_MSG2_FROM_SP");
    (void)request;
    return response;
}

AttestationResponse RemoteAttestationServiceProvider::build_attestation_result(bool trusted) const
{
    AttestationResponse response;
    response.trusted = trusted;
    response.message_type = MSD_RA_ATT_RESULT;
    response.detail = trusted ? "service_provider: attestation accepted" : "service_provider: attestation rejected";
    response.payload = trusted ? text_payload("OK") : text_payload("FAIL");
    return response;
}
