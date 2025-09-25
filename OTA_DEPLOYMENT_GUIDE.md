# Arduino Giga R1 WiFi OTA Deployment Guide

## Overview

Your Arduino project now includes a complete Over-The-Air (OTA) update system that uses GitHub Releases for stable, production-ready firmware distribution. This guide covers the complete workflow from building firmware to deploying updates.

## 🔧 System Architecture

### New GitHub Releases Approach
- **GitHub API Integration**: Uses GitHub's REST API to fetch latest release information
- **Stable Release URLs**: Downloads from GitHub Releases (immutable, tagged versions)
- **Automatic Asset Discovery**: Finds .ota, .version, and .md5 files in release assets
- **No Redirect Issues**: Direct API calls avoid GitHub's redirect problems

### Key Components
1. **build_ota.sh** - Automated OTA file generation script
2. **GitHub API Integration** - Fetches release info via REST API
3. **ManualOTAManager** - Updated to use stable release URLs
4. **Dynamic URL Resolution** - Constructs download URLs from API response

## 🚀 Complete Deployment Workflow

### Step 1: Generate OTA Files

First, compile your Arduino sketch and export the binary:

```bash
# In Arduino IDE:
# 1. Select "Arduino Giga R1 WiFi" board
# 2. Go to Sketch > Export Compiled Binary
# 3. Find the .bin file in your sketch directory
```

Then run the OTA build script:

```bash
cd /path/to/your/sketch
./build_ota.sh FullUI_with_WiFi_selector_Claude_IMU.ino.GIGA.bin
```

This will generate:
- `firmware.ota` - Compressed, signed OTA file
- `firmware.version` - Version file with current version number
- `firmware.md5` - MD5 checksum file

### Step 2: Create GitHub Release

1. **Tag a new release** in your GitHub repository:
   ```bash
   git tag v2.1.70
   git push origin v2.1.70
   ```

2. **Create the release** on GitHub:
   - Go to your repository on GitHub
   - Click "Releases" → "Create a new release"
   - Select your tag (e.g., v2.1.70)
   - Add release notes
   - Upload the generated files as release assets:
     - `firmware.ota`
     - `firmware.version`
     - `firmware.md5`

### Step 3: Test OTA Update

Your Arduino will now:
1. Query GitHub API: `GET /repos/starstreampro/ota/releases/latest`
2. Parse the JSON response to find the latest version and download URLs
3. Compare versions and offer update if newer version available
4. Download the .ota file directly from GitHub's release assets

## 🔧 Technical Details

### OTA File Generation Process

The `build_ota.sh` script follows the official Arduino process:

1. **LZSS Compression**: `lzss.py --encode input.bin compressed.lzss`
2. **OTA File Creation**: `bin2ota.py GIGA compressed.lzss output.ota`
3. **Magic Number**: Uses `0x23410266` for Arduino GIGA
4. **Checksum Generation**: MD5 hash for integrity verification

### GitHub API Integration

The Arduino code now uses these endpoints:
- **API Host**: `api.github.com`
- **Release Info**: `/repos/starstreampro/ota/releases/latest`
- **Asset Downloads**: Direct URLs from GitHub's CDN

### Arduino Code Changes

Key improvements in your Arduino code:

```cpp
// GitHub API configuration
constexpr const char *GITHUB_API_HOST = "api.github.com";
constexpr const char *GITHUB_REPO_OWNER = "starstreampro";
constexpr const char *GITHUB_REPO_NAME = "ota";

// Dynamic URL resolution
String dynamic_ota_download_url = "";
String dynamic_version = "";
String dynamic_checksum = "";

// New methods
bool fetchLatestReleaseInfo();
bool parseReleaseResponse(const String& response);
bool fetchChecksumFromUrl(const String& url);
```

## 📦 File Structure

```
your-sketch/
├── FullUI_with_WiFi_selector_Claude_IMU.ino
├── build_ota.sh                    # OTA build script
├── lzss.py                         # LZSS compression tool
├── bin2ota.py                      # Binary to OTA converter
├── lzss.dylib                      # LZSS library (macOS)
├── ota-env/                        # Python virtual environment
└── build/                          # Arduino IDE build files
    └── *.bin                       # Compiled binaries
```

## 🎯 Benefits of This Approach

### Production Ready
- **Immutable Releases**: Tagged versions can't be accidentally modified
- **Rollback Support**: Previous releases remain available
- **Change Tracking**: GitHub releases include changelogs and version history

### Reliable Downloads
- **No Redirects**: Direct API calls avoid 302 redirect issues
- **Stable URLs**: GitHub's CDN provides reliable, fast downloads
- **Automatic Discovery**: Finds correct files in release assets

### Development Friendly
- **Automated Build**: One script handles the complete OTA generation process
- **Version Management**: Automatic version extraction from source code
- **Integrity Checking**: MD5 checksums ensure download integrity

## 🔍 Troubleshooting

### Common Issues

1. **"Could not fetch release information"**
   - Check internet connection
   - Verify GitHub repository exists and is public
   - Check API rate limits

2. **"Incomplete release information"**
   - Ensure .ota file is uploaded to the release
   - Verify file naming matches expected pattern

3. **"LZSS compression failed"**
   - Check Python virtual environment is activated
   - Verify lzss.dylib exists and is executable
   - Ensure sufficient disk space

### Debug Mode

Enable verbose debugging in Arduino Serial Monitor to see:
- GitHub API responses
- JSON parsing details
- Download progress
- Memory usage statistics

## 📈 Next Steps

1. **Automatic Updates**: Add scheduled update checks
2. **Delta Updates**: Implement differential patching for smaller downloads
3. **Staged Rollouts**: Deploy to subset of devices first
4. **Update Notifications**: Add UI notifications for available updates

## 🔒 Security Considerations

- GitHub releases are served over HTTPS by default
- MD5 checksums verify download integrity
- Consider implementing digital signatures for enhanced security
- Monitor GitHub API rate limits for production deployments

---

Your Arduino Giga R1 WiFi OTA system is now production-ready with stable GitHub Releases integration!