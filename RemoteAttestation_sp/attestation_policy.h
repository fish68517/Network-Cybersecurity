#ifndef REMOTE_ATTESTATION_POLICY_H__
#define REMOTE_ATTESTATION_POLICY_H__

#include <string>

struct AttestationPolicy {
    std::string allowed_identity;
    bool allow_debug_enclave;
    int minimum_isv_svn;

    AttestationPolicy()
        : allowed_identity("service_provider"),
          allow_debug_enclave(true),
          minimum_isv_svn(0)
    {
    }
};

#endif
