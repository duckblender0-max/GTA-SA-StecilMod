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
- Reads INI settings and clamps the requested `MaxShadows` value to 1–128; this does not resize GTA's native shadow arrays.
- Calculates a smooth time-of-day fade curve: faint before 06:00, full at 12:00, fades after 18:00, and reaches the configured night strength by midnight.
- Provides three experimental hooks controlled independently in `[ExperimentalHooks]`; all are OFF by default after the reported in-game crash.
- When explicitly enabled, the intensity hook scales native shadow submissions; the pedestrian hook gates player/pedestrian shadow submissions; the pole hook gates GTA's existing pole-shadow routine.
- The player/pedestrian identity check uses `CWorld::PlayerInFocus`; if identity cannot be resolved while the target switches differ, the hook fails closed for that call.

Still pending:
- Tree and bridge shadow generation; those INI switches are currently reserved and do not create new shadows.
- Expanding native shadow storage beyond GTA's default fixed capacity; 128 remains a configuration ceiling, not an allocated capacity.
- Loading and binding the `.fx` shader.

The source now polls the documented GTA SA 1.0 US CClock memory fields (hour at `0xB70153`, minute at `0xB70152`) and calculates fade strength from the live in-game clock. It also uses MinHook to intercept GTA's native `CShadows::StoreShadowToBeRendered` function at `0x00707390`, scaling the intensity of shadows that the game already submits. This is a first integration with the game's existing shadow pipeline; it does not yet generate new tree/bridge/lamp shadows.

For cautious testing, enable only one `[ExperimentalHooks]` option at a time, then launch the game and inspect `StencilShadows.log`. Leave the other two at `0`. The default package disables all three hooks; it only checks the GTA clock and calculates the fade strength.

Important limitations: `MaxShadows=128` is currently a configuration target only; GTA SA's native shadow storage is fixed-size and has not been expanded. The hooks currently adjust or gate GTA routines that already exist; they do not synthesize new shadow geometry. The pole hook controls GTA's existing pole-shadow routine, not arbitrary new lamp shadows. This is not yet a complete custom stencil-shadow mod, and the ASI still needs in-game testing before it should be treated as safe to install.
