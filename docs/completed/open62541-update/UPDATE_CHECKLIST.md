# open62541 Update Checklist: v0.3.0 → v1.4.14

---
## ✅ **STATUS: COMPLETED**

**This checklist has been fully completed!**

See **[OPEN62541_V1.4.14_UPDATE_COMPLETE.md](OPEN62541_V1.4.14_UPDATE_COMPLETE.md)** for the completion report.

**Remaining deployment tasks:**
- ⏳ Test on GitHub Actions (all Ruby versions 2.4-4.0)
- ⏳ Merge to main branch
- ⏳ Release new gem version

---

## Pre-Update

- [ ] Create backup branch: `git checkout -b backup-v0.3.0`
- [ ] Push backup: `git push origin backup-v0.3.0`
- [ ] Create update branch: `git checkout -b update-open62541-v1.4.14`
- [ ] Run baseline tests: `bundle exec rake` (should pass)
- [ ] Document checksums: `md5sum open62541.* > checksums_v0.3.0.txt`

## Download Files

- [ ] Download header: `curl -L -o open62541_v1.4.14.h https://github.com/open62541/open62541/releases/download/v1.4.14/open62541.h`
- [ ] Download source: `curl -L -o open62541_v1.4.14.c https://github.com/open62541/open62541/releases/download/v1.4.14/open62541.c`
- [ ] Verify downloads: `wc -l open62541_v1.4.14.*` (should be ~51k and ~273k lines)

## Backup and Replace

- [ ] Backup old header: `mv open62541.h open62541_v0.3.0.h`
- [ ] Backup old source: `mv open62541.c open62541_v0.3.0.c`
- [ ] Install new header: `mv open62541_v1.4.14.h open62541.h`
- [ ] Install new source: `mv open62541_v1.4.14.c open62541.c`

## Code Updates in opcua_client.c

### Memory Management Functions (13 locations)

- [ ] Line 276: `UA_ReadResponse_deleteMembers(&response)` → `UA_ReadResponse_clear(&response)`
- [ ] Line 286: `UA_ReadResponse_deleteMembers(&response)` → `UA_ReadResponse_clear(&response)`
- [ ] Line 294: `UA_ReadResponse_deleteMembers(&response)` → `UA_ReadResponse_clear(&response)`
- [ ] Line 305: `UA_ReadResponse_deleteMembers(&response)` → `UA_ReadResponse_clear(&response)`
- [ ] Line 356: `UA_WriteResponse_deleteMembers(&wResp)` → `UA_WriteResponse_clear(&wResp)`
- [ ] Line 434: `UA_Variant_deleteMembers(&readValues[i])` → `UA_Variant_clear(&readValues[i])`
- [ ] Line 444: `UA_Variant_deleteMembers(&readValues[i])` → `UA_Variant_clear(&readValues[i])`
- [ ] Line 682: `UA_Variant_deleteMembers(&value)` → `UA_Variant_clear(&value)`
- [ ] Line 687: `UA_Variant_deleteMembers(&value)` → `UA_Variant_clear(&value)`
- [ ] Line 808: `UA_String_deleteMembers(&array[i])` → `UA_String_clear(&array[i])`
- [ ] Line 819: `UA_Variant_deleteMembers(&value)` → `UA_Variant_clear(&value)`
- [ ] Line 823: `UA_Variant_deleteMembers(&value)` → `UA_Variant_clear(&value)`
- [ ] Line 992: `UA_Variant_deleteMembers(&value)` → `UA_Variant_clear(&value)`
- [ ] Line 1042: `UA_Variant_deleteMembers(&value)` → `UA_Variant_clear(&value)`
- [ ] Line 1135: `UA_Variant_deleteMembers(&value)` → `UA_Variant_clear(&value)`
- [ ] Line 1140: `UA_Variant_deleteMembers(&value)` → `UA_Variant_clear(&value)`
- [ ] Line 1143: `UA_Variant_deleteMembers(&value)` → `UA_Variant_clear(&value)`

### Client Configuration (if needed)

- [ ] Check line 165: `UA_ClientConfig customConfig = UA_ClientConfig_default;`
- [ ] Verify if config structure changed
- [ ] Update if necessary

### Client State Constants

- [ ] Verify line 1289: `UA_CLIENTSTATE_DISCONNECTED` still exists
- [ ] Verify line 1290: `UA_CLIENTSTATE_CONNECTED` still exists
- [ ] Verify line 1291: `UA_CLIENTSTATE_SECURECHANNEL` still exists
- [ ] Verify line 1292: `UA_CLIENTSTATE_SESSION` still exists
- [ ] Verify line 1293: `UA_CLIENTSTATE_SESSION_RENEWED` still exists

## Compilation

- [ ] Clean build: `bundle exec rake clean`
- [ ] Compile: `bundle exec rake compile`
- [ ] Fix any compilation errors
- [ ] Fix any warnings (if critical)

## Testing - Data Types

### Scalar Types
- [ ] Test Byte read/write
- [ ] Test SByte read/write
- [ ] Test Int16 read/write
- [ ] Test UInt16 read/write
- [ ] Test Int32 read/write
- [ ] Test UInt32 read/write
- [ ] Test Int64 read/write
- [ ] Test UInt64 read/write
- [ ] Test Float read/write
- [ ] Test Double read/write
- [ ] Test Boolean read/write
- [ ] Test String read/write
- [ ] Test String UTF-8 encoding

### Array Types
- [ ] Test Byte array read/write
- [ ] Test SByte array read/write
- [ ] Test Int16 array read/write
- [ ] Test UInt16 array read/write
- [ ] Test Int32 array read/write
- [ ] Test UInt32 array read/write
- [ ] Test Int64 array read/write
- [ ] Test UInt64 array read/write
- [ ] Test Float array read/write
- [ ] Test Double array read/write
- [ ] Test Boolean array read/write
- [ ] Test String array read/write

### Multi-Operations
- [ ] Test multi-read (read_values)
- [ ] Test multi-write (write_values)

## Testing - Client Operations

- [ ] Test client initialization
- [ ] Test client connect
- [ ] Test client disconnect
- [ ] Test client state
- [ ] Test run_once
- [ ] Test run_once_wait
- [ ] Test error handling
- [ ] Test status codes

## Testing - Subscriptions

- [ ] Test create_subscription
- [ ] Test create_monitored_item
- [ ] Test subscription callbacks
- [ ] Test data change notifications

## Full Test Suite

- [ ] Run: `bundle exec rspec spec/client_integration_spec.rb`
- [ ] Run: `bundle exec rspec spec/server_spec.rb`
- [ ] Run: `bundle exec rake` (all tests)
- [ ] All tests pass ✅

## Validation

- [ ] Test with real OPC UA server (if available)
- [ ] Check memory usage (expect increase)
- [ ] Check performance (should be similar or better)
- [ ] Test on Linux
- [ ] Test on Windows (if applicable)
- [ ] Test with Ruby 2.4
- [ ] Test with Ruby 2.5
- [ ] Test with Ruby 2.6
- [ ] Test with Ruby 2.7
- [ ] Test with Ruby 3.0
- [ ] Test with Ruby 3.1

## Documentation

- [ ] Update README with new open62541 version
- [ ] Update CHANGELOG
- [ ] Document any API changes
- [ ] Update version in gemspec (if needed)

## Finalization

- [ ] Commit changes: `git commit -am "Update open62541 from v0.3.0 to v1.4.14"`
- [ ] Push branch: `git push origin update-open62541-v1.4.14`
- [ ] Create pull request
- [ ] Get code review
- [ ] Merge to main
- [ ] Tag release (if applicable)

## Rollback (if needed)

- [ ] Restore files: `mv open62541_v0.3.0.* open62541.*`
- [ ] Rebuild: `bundle exec rake clean && bundle exec rake compile`
- [ ] Test: `bundle exec rake`
- [ ] Or: `git checkout backup-v0.3.0`

---

## Notes

- Estimated time: 8-16 hours total
- Most critical: Update all `*_deleteMembers()` to `*_clear()`
- Test thoroughly before merging
- Keep backup branch until confident in update

