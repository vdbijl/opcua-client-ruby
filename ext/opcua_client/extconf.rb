require 'mkmf'

# Suppress warnings from open62541.c (vendored library)
# These warnings are in the upstream open62541 library code
$CFLAGS << ' -Wno-discarded-qualifiers'

create_makefile 'opcua_client/opcua_client'
