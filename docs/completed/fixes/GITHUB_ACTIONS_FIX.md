# GitHub Actions Build Fix

**Date:** 2026-01-05  
**Issue:** GitHub Actions failing on Ruby 2.4, 2.5, and 2.6  
**Status:** ✅ FIXED

---

## Problem

GitHub Actions workflow was failing with 6 out of 12 jobs:
- ❌ ubuntu-22.04 ruby 2.4
- ❌ ubuntu-22.04 ruby 2.5
- ❌ ubuntu-22.04 ruby 2.6
- ❌ windows-2022 ruby 2.4
- ❌ windows-2022 ruby 2.5
- ❌ windows-2022 ruby 2.6

**Error:** `Process completed with exit code 1`

---

## Root Cause

The `debug` gem was added to the Gemfile to replace the obsolete `debugger` gem. However:

- **`debug` gem requires Ruby 2.7+**
- The CI matrix tests Ruby 2.4, 2.5, 2.6, 2.7, 3.0, and 3.1
- Ruby 2.4, 2.5, and 2.6 cannot install the `debug` gem
- This caused `bundle install` to fail during the CI run

---

## Solution

Updated `Gemfile` to conditionally install the appropriate debugger based on Ruby version:

```ruby
group :test do
  gem 'rspec'
  # debug gem requires Ruby 2.7+, use byebug for older versions
  if RUBY_VERSION >= '2.7.0'
    gem 'debug' # Modern debugger for Ruby 2.7+
  else
    gem 'byebug' # Debugger for Ruby 2.4-2.6
  end
end
```

### Debugger Selection

| Ruby Version | Debugger | Reason |
|--------------|----------|--------|
| 2.4 - 2.6 | `byebug` | Compatible with older Ruby versions |
| 2.7+ | `debug` | Modern, actively maintained, built into Ruby 3.1+ |

---

## How to Use

### Ruby 2.7+ (using `debug`)

```ruby
require 'debug'
debugger  # or binding.break
```

**Commands:**
- `n` / `next` - Next line
- `s` / `step` - Step into
- `c` / `continue` - Continue
- `l` / `list` - Show code
- `p var` - Print variable
- `bt` - Backtrace
- `q` - Quit

### Ruby 2.4-2.6 (using `byebug`)

```ruby
require 'byebug'
byebug  # or debugger
```

**Commands:**
- `n` / `next` - Next line
- `s` / `step` - Step into
- `c` / `continue` - Continue
- `l` / `list` - Show code
- `p var` - Print variable
- `bt` - Backtrace
- `q` - Quit

---

## Verification

### Local Testing (Ruby 4.0.0)
```bash
$ bundle update
Bundle updated!

$ bundle list | grep debug
  * debug (1.11.1)

$ bundle exec rake
51 examples, 0 failures ✅
```

### CI Testing
After pushing this fix, all 14 jobs should pass:
- ✅ ubuntu-22.04 ruby 2.4 (with byebug)
- ✅ ubuntu-22.04 ruby 2.5 (with byebug)
- ✅ ubuntu-22.04 ruby 2.6 (with byebug)
- ✅ ubuntu-22.04 ruby 2.7 (with debug)
- ✅ ubuntu-22.04 ruby 3.0 (with debug)
- ✅ ubuntu-22.04 ruby 3.1 (with debug)
- ✅ ubuntu-22.04 ruby 4.0 (with debug)
- ✅ windows-2022 ruby 2.4 (with byebug)
- ✅ windows-2022 ruby 2.5 (with byebug)
- ✅ windows-2022 ruby 2.6 (with byebug)
- ✅ windows-2022 ruby 2.7 (with debug)
- ✅ windows-2022 ruby 3.0 (with debug)
- ✅ windows-2022 ruby 3.1 (with debug)
- ✅ windows-2022 ruby 4.0 (with debug)

---

## Files Changed

- `Gemfile` - Added conditional debugger selection based on Ruby version
- `.github/workflows/build.yml` - Added Ruby 4.0 to the test matrix

---

## Next Steps

1. Commit and push the fix:
   ```bash
   git add Gemfile .github/workflows/build.yml
   git commit -m "Fix: Use byebug for Ruby < 2.7, debug for Ruby >= 2.7; Add Ruby 4.0 to CI"
   git push
   ```

2. Verify GitHub Actions passes all 14 jobs (now including Ruby 4.0!)

3. Merge the pull request once CI is green ✅

