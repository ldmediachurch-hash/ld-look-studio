# LD Look Studio 🎨🎬

[![CI Build](https://github.com/ldmediachurch-hash/ld-look-studio/actions/workflows/push.yaml/badge.svg)](https://github.com/ldmediachurch-hash/ld-look-studio/actions/workflows/push.yaml)
[![Release](https://img.shields.io/github/v/release/ldmediachurch-hash/ld-look-studio?color=white&label=release)](https://github.com/ldmediachurch-hash/ld-look-studio/releases)
[![License: GPL v2](https://img.shields.io/badge/License-GPLv2-blue.svg)](LICENSE)

**LD Look Studio** is a professional video filter and interactive dock plugin for **OBS Studio (macOS, Windows, Ubuntu)** designed by **LD Media Church**.

It brings the visual grading power of **DaVinci Resolve’s Look Gallery** directly into OBS Studio with dedicated color science for **African and melanin-rich skin tones**, iconic cinema looks, real-time A/B split wipe comparison, and upstream filter stack snapshot freeze.

---

## 🌟 Why LD Look Studio?

Standard cinematic LUTs (like extreme orange-and-teal or bleach bypass) were rarely formulated with African skin tones in mind. Under harsh church stage LED fixtures or studio lights, generic LUTs often:
1. Turn warm brown or bronze skin **greenish, olive, or sickly magenta**.
2. Crush rich melanin undertones (IRE 20–55) into **muddy shadow clipping**.
3. Blow out forehead and cheek specular reflections into **chalky, plastic white clips**.

**LD Look Studio solves this with dedicated color science**:
- **Melanin Skin Tone Protection Engine**: Automatically isolates the human skin locus along the $Cr/Cb$ vector and protects natural bronze, gold, and deep mahogany undertones.
- **Upstream Filter Freeze Frame**: Captures a still frame *after* all your prior filters (color corrections, sharpen, crop) have already run, so you can test looks on your exact real-time camera stack.
- **DaVinci Resolve-Style Stills Gallery**: View every single look rendered across live thumbnail cards side by side before sending live.
- **Real-Time A/B Split Wipe**: Compare your un-graded vs. graded camera feed with an interactive draggable divider line.

---

## 🎬 16 Master Cinematic Presets Included

### 🌍 African Cinema & Melanin-Rich Skin
- **Afro Glow**: Warm amber and golden undertones with luminous highlight rolloff.
- **Rich Mahogany**: Deep velvety contrast for darker skin tones, eliminating grey ashen cast.
- **Nollywood Prestige**: Vibrant, punchy modern Nigerian cinema grade with saturated primaries and royal fidelity.
- **Golden Hour Lagos**: Sun-drenched equatorial warmth and honey midtones.

### 🎥 Iconic Motion Picture Looks
- **Wakanda Royal Gold** (*Black Panther*): Opulent regal gold highlights with deep indigo/sapphire shadow contrast.
- **Moonlight Velvet** (*Moonlight* / Barry Jenkins): Deep cyan night-sky shadows with glistening warm velvet skin highlights.
- **Insecure Glow** (*Insecure* / Ava Berkofsky): Radiant moisturized skin lighting, warm bronze tones, and clean lifted shadows.
- **Dune Amber** (*Dune* / Greig Fraser): Desaturated cool tones with warm monochromatic desert amber curves.
- **Kodak 2383 Print**: Hollywood’s most legendary 35mm motion picture print stock with rich subtractive color and cyan shadow rolloff.
- **Matrix Cyber Noir**: Cyberpunk olive-green shadow bias with protected warm skin highlights.
- **Miami Teal & Orange**: Modern complementary harmony balancing deep cyan backgrounds with warm skin tones.

### ⛪ Sanctuary, Stage & Broadcast
- **Sunday Clean Rec.709**: True-to-life broadcast color with soft knee highlight compression for worship livestreams.
- **Warm Pulpit**: Neutralizes cool cyan/blue stage LED spotlights while softening forehead specular reflections.
- **Stage Punch**: Enhanced micro-contrast and saturation for flat or Log camera profiles.

### 🎞️ Fine-Art Black & White
- **Noir Silver Print**: Panchromatic silver gelatin print with red/orange spectral weighting that beautifully accentuates dark skin tone luminance.
- **Soft Sepia Vintage**: Warm antique dual-tone split for acoustic worship and reflective moments.

---

## 📦 Download & Installation

Download the latest installer from [GitHub Releases](https://github.com/ldmediachurch-hash/ld-look-studio/releases/latest):

### 🍏 macOS (Universal: Apple Silicon M1/M2/M3/M4 & Intel)
1. Download `LD-Look-Studio-1.0.0-macos-universal.pkg`.
2. Double-click the installer and follow the on-screen instructions.
3. Restart OBS Studio.

### 🪟 Windows (64-bit)
1. Download `LD-Look-Studio-1.0.0-windows-x64.zip`.
2. Extract into your OBS Studio directory (typically `C:\Program Files\obs-studio\`).
3. Restart OBS Studio.

### 🐧 Ubuntu / Linux (x86_64)
```bash
sudo dpkg -i LD-Look-Studio-1.0.0-x86_64-linux-gnu.deb
```

---

## 🚀 How to Use

1. **Add Filter**: Right-click any camera source or scene in OBS ➔ **Filters** ➔ **+** ➔ Select **LD Look Studio**.
2. **Open Dock**: In OBS top menu, open **Docks** ➔ Check **LD Look Studio**.
3. **Capture Still**: Click **Freeze Frame** in the dock. The dock will instantly grab a high-resolution frame of your source after all prior filters have run.
4. **Choose Your Look**: Browse category chips (*African Cinema*, *Iconic Movies*, *Church & Stage*, *Black & White*).
5. **Click Any Card**: The look is instantly previewed and marked with a **● LIVE** badge.
6. **Adjust Melanin Protection & Mix**: Use the sliders to dial in the perfect balance of skin warmth and cinematic color grade.
7. **Import Custom LUTs**: Click **Import .cube…** to load any 3D LUT from your computer.

---

## 🛠️ Building from Source

### Prerequisites
- CMake 3.28+
- Qt 6 (Widgets, Core, Gui)
- libobs & obs-frontend-api 31.0+

```bash
git clone https://github.com/ldmediachurch-hash/ld-look-studio.git
cd ld-look-studio
cmake --preset macos # or windows-x64, ubuntu-x86_64
cmake --build --preset macos
```

---

## 📄 License
This project is licensed under the **GNU General Public License v2.0** — see [LICENSE](LICENSE) for details.
Crafted with ❤️ by **LD Media Church**.
