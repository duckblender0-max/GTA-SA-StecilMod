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
- Finding and classifying trees, lamps, bridges, the player, and pedestrians.
- Generating new real-time shadows for trees, lamp posts, bridges, and other static geometry.
- Loading and binding the `.fx` shader.

The source now polls the documented GTA SA 1.0 US CClock memory fields (hour at `0xB70153`, minute at `0xB70152`) and calculates fade strength from the live in-game clock. It also uses MinHook to intercept GTA's native `CShadows::StoreShadowToBeRendered` function at `0x00707390`, scaling the intensity of shadows that the game already submits. This is a first integration with the game's existing shadow pipeline; it does not yet generate new tree/bridge/lamp shadows.

Important limitations: `MaxShadows=128` is currently a configuration target only; GTA SA's native shadow storage is fixed-size and has not been expanded. The hook only scales the intensity of shadows the game already submits. It does not yet create new shadows for trees, lamp posts, or bridges. This is not yet a complete custom stencil-shadow mod, and the ASI still needs in-game testing before it should be treated as safe to install.
