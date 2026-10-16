# Event schedule

A compact **Event Schedule** panel appears automatically near the top-right of
the HUD during gameplay. It lists Blood Castle, Devil Square, Chaos Castle, and Kanturu when those
events are configured by the connected OpenMU server.

Each row has one of these states:

- `Starts in HH:MM:SS` (or `MM:SS`) before the next configured start.
- `Closes in HH:MM:SS` while the entrance window is open.
- `In progress` after entrance closes and while the event is running.

The server is authoritative. The client requests an atomic snapshot through
the OpenMU extension packet group (`F5:02`), ticks the received number of
seconds locally, and refreshes it every 30 seconds or when a countdown reaches
zero. On an older server without schedule support, the panel remains hidden.

## Protocol

The client request is a `C1 F5 02` packet. The server response is a `C2 F5 02`
packet containing a count followed by six-byte entries:

1. mini-game type (`1` Devil Square, `2` Blood Castle, `4` Chaos Castle,
   `7` Kanturu);
2. state (`0` scheduled, `1` entrance open, `2` running);
3. remaining seconds as an unsigned little-endian 32-bit value.

The remaining value is used for scheduled and open states. A running event has
no countdown because these events can finish early and OpenMU does not expose
one reliable end timestamp for all mini-games.

The HUD panel sits below the top-right system buttons and scales with the UI
preference in both themes. It is click-through and does not capture Esc or other
keys. Opening inventory or other windows does not dismiss it. It is hidden during
loading and outside gameplay, and remains hidden if the server has no schedule
support or returns no configured events. There is no separate window or menu entry.
