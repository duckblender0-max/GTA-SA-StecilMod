# GTA-SA-StecilMod

A custom stencil-shadow ASI project for **GTA San Andreas 1.0 US (32-bit)**.

## Requested configuration

- Performance preset
- Maximum 128 shadow slots (configuration target)
- Target categories: trees, street lamps, bridges, player, and pedestrians
- Dynamic strength: starts increasing at 06:00, reaches full strength at 12:00, fades from 18:00, and remains faint at night

## Build without installing Visual Studio

GitHub Actions is configured to build the project as a 32-bit Windows ASI:

1. Open the **Actions** tab.
2. Select **Build StencilShadows ASI**.
3. Choose **Run workflow**, or push a commit to `main`.
4. Download the `StencilShadows-x86-package` artifact from the completed run.

The artifact is intended to contain:
- `StencilShadows.asi`
- `StencilShadows.ini`
- `StencilShadows.fx`
- `StencilShadows.log`

## Important status

This first milestone is only an **ASI bootstrap and build pipeline**. The ASI writes a diagnostic log when loaded. It does **not yet hook GTA San Andreas, enumerate world entities, render stencil shadows, or load/bind the shader**. The time-fade values are configuration only until the renderer is implemented. Do not install this as a working shadow mod yet.
