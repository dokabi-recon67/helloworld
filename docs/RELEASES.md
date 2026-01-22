# HelloWorld Releases

This document explains how releases work and where to find binaries.

## Source Code vs Binaries

### Source Code (GitHub Main Branch)
- ✅ **Always available** in the main branch
- ✅ **Transparent** - anyone can review and verify
- ✅ **Buildable** - instructions in README.md
- ✅ **No sensitive data** - all excluded via .gitignore

### Binaries (GitHub Releases)
- 📦 **Pre-built executables** for easy installation
- 🏷️ **Versioned** - each release has a tag
- 🔒 **Signed** (future) - will be code-signed for security
- 📥 **Downloadable** - no build required

## Current Release Status

Binaries are currently built manually. To create a release:

1. **Build the binaries:**
   ```powershell
   # Windows
   cd client\build
   cmake --build . --config Release
   ```

2. **Create GitHub Release:**
   - Go to GitHub → Releases → Draft a new release
   - Tag: `v1.0.0` (or next version)
   - Upload: `client\build\Release\helloworld.exe`
   - Add release notes

3. **Users download from Releases page**

## Future: Automated Releases

We plan to add:
- GitHub Actions for automatic builds
- Code signing for Windows/macOS
- Automatic release creation on tags

## Security

- ✅ Source code is public and auditable
- ✅ Binaries match source (reproducible builds)
- ✅ No credentials or keys in repository
- ✅ All sensitive data excluded via .gitignore

