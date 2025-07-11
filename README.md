# HLDMClient

A custom Half-Life Deathmatch client built on top of the Half-Life SDK. This client adds many quality-of-life improvements, compatibility fixes, and optional visual enhancements while remaining compatible with both modern and legacy HLDM servers.

---

## 🚀 Features & Changes

### 🎨 HUD Enhancements
- Added support for color tags in HUD strings (`hud_color "255 160 0"`).
- Added timers:
  - Time left until map ends
  - Time elapsed since map start
  - Local personal timer (`hud_timer 1`, `2`, `3`)

### 🔧 Compatibility
- Support for both old and new `+USE` (E key) mechanics.
  - Use `cl_useslowdown 0` for legacy servers
  - Use `cl_useslowdown 1` for modern servers

### 👕 Forced Models
- `cl_forceenmodel "model"` — Replace all enemy models with a specific one.
- `cl_forceteammodel "model"` — Replace all teammates' models.

### 🛠️ Bug Fixes
- Fixed many crashes and bugs from the Half-Life 25th Anniversary SDK.
- Some bugs may still remain — ongoing testing is encouraged.

### 🎯 Custom Crosshair (like in OpenAG)
- `crosshair 0` disables the default sprite crosshair
- Full support for modern crosshair customization:
  - `cl_cross` — Enable/disable custom crosshair
  - `cl_cross_alpha` — Opacity (0–255)
  - `cl_cross_circle_radius` — Radius of surrounding circle (0 to disable)
  - `cl_cross_color "R G B"` — Color of crosshair
  - `cl_cross_dot_size` — Center dot size (0 to disable)
  - `cl_cross_dot_color` — Dot color
  - `cl_cross_gap` — Gap size
  - `cl_cross_size` — Size/length of lines
  - `cl_cross_thickness` — Thickness of lines
  - `cl_cross_outline` — Outline thickness (0 to disable)
  - `cl_cross_{top,bottom,left,right}_line` — Per-line length customization

### 🏆 Scoreboard & Info
- Added OpenAG-style score display:
  - `cl_scores 2` — shows top 2 players
  - `cl_scores_pos "x y"`
- Scoreboard now shows each player's model name

### 📈 Speedometer
- Toggle with `hud_speedometer 1` , `0`

### 🌟 Extended Sprites
- Support for larger sprites like HL25
- Use `cl_sprites 640` to load high-res sprites
> ⚠️ Warning: If a sprite is missing at that resolution, the game may crash.

---

## 🧪 Tested On
- Latest **Steam** version of Half-Life as of July 2025

---

## 🛠 Build Instructions

1. Open the solution (`.sln`) in **Visual Studio 2022**
2. Make sure required components (`MSVC`, Windows SDK) are installed
3. Set configuration to `Release`
4. Build the client

Or build via command line:
```bash
msbuild path\to\your\project.sln /p:Configuration=Release
```
---

Half Life 1 SDK LICENSE
======================

Half Life 1 SDK Copyright © Valve Corp.

THIS DOCUMENT DESCRIBES A CONTRACT BETWEEN YOU AND VALVE CORPORATION (“Valve”).  PLEASE READ IT BEFORE DOWNLOADING OR USING THE HALF LIFE 1 SDK (“SDK”). BY DOWNLOADING AND/OR USING THE SOURCE ENGINE SDK YOU ACCEPT THIS LICENSE. IF YOU DO NOT AGREE TO THE TERMS OF THIS LICENSE PLEASE DON’T DOWNLOAD OR USE THE SDK.

You may, free of charge, download and use the SDK to develop a modified Valve game running on the Half-Life engine.  You may distribute your modified Valve game in source and object code form, but only for free. Terms of use for Valve games are found in the Steam Subscriber Agreement located here: https://store.steampowered.com/subscriber_agreement/ 

You may copy, modify, and distribute the SDK and any modifications you make to the SDK in source and object code form, but only for free.  Any distribution of this SDK must include this license.txt and third_party_licenses.txt.  
 
Any distribution of the SDK or a substantial portion of the SDK must include the above copyright notice and the following: 

DISCLAIMER OF WARRANTIES.  THE SOURCE SDK AND ANY OTHER MATERIAL DOWNLOADED BY LICENSEE IS PROVIDED “AS IS”.  VALVE AND ITS SUPPLIERS DISCLAIM ALL WARRANTIES WITH RESPECT TO THE SDK, EITHER EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, TITLE AND FITNESS FOR A PARTICULAR PURPOSE.  

LIMITATION OF LIABILITY.  IN NO EVENT SHALL VALVE OR ITS SUPPLIERS BE LIABLE FOR ANY SPECIAL, INCIDENTAL, INDIRECT, OR CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS) ARISING OUT OF THE USE OF OR INABILITY TO USE THE ENGINE AND/OR THE SDK, EVEN IF VALVE HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.  
 
 
If you would like to use the SDK for a commercial purpose, please contact Valve at sourceengine@valvesoftware.com.
