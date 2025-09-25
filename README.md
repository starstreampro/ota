# Arduino Giga R1 WiFi - OTA Firmware Repository

**FIRMWARE ONLY REPOSITORY** - Contains only compiled firmware binaries for OTA updates.

## 📁 Repository Contents

```
firmware/
├── ssp-streamboxpro-v2.ota      # Compressed firmware binary (88KB)
├── ssp-streamboxpro-v2.version  # Version identifier
└── ssp-streamboxpro-v2.md5      # Integrity checksum
```

## 🔄 OTA Update Process

Arduino devices automatically check this repository via GitHub Releases API:

1. Query: `https://api.github.com/repos/starstreampro/ota/releases/latest`
2. Download firmware from release assets
3. Verify integrity using MD5 checksum
4. Install and activate new firmware

## 📋 Current Release

- **Version**: 2.2.69
- **Size**: 88KB (LZSS compressed)
- **Target**: Arduino Giga R1 WiFi

## ⚠️ Repository Policy

**This repository contains ONLY firmware files for OTA distribution.**

- ❌ No source code
- ❌ No development tools
- ❌ No documentation
- ❌ No build scripts
- ✅ Only compiled firmware binaries

---

**Secure OTA Firmware Distribution for Arduino Giga R1 WiFi**