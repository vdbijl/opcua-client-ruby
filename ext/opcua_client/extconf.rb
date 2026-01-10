require 'mkmf'

# open62541 v1.4.14 requires mbedtls for cryptographic operations
have_library('mbedtls') or abort "mbedtls library not found"
have_library('mbedx509') or abort "mbedx509 library not found"
have_library('mbedcrypto') or abort "mbedcrypto library not found"

create_makefile 'opcua_client/opcua_client'
