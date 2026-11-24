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

## Implementation status

Implemented in the current source:
- Reads and validates INI settings, including a hard cap of 128 shadow slots.
- Calculates a smooth time-of-day fade curve: faint before 06:00, full at 12:00, fades after 18:00, and reaches the configured night strength by midnight.
- Logs loaded settings and fade-curve diagnostic samples.

Still pending:
- Connecting the fade calculation to GTA's in-game clock.
- Finding and classifying trees, lamps, bridges, the player, and pedestrians.
- Creating and drawing stencil shadows through RenderWare/Direct3D 9.
- Loading and binding the `.fx` shader.

This is a real code milestone, but it is **not yet a functioning shadow renderer**. Do not install it expecting visible shadows.
