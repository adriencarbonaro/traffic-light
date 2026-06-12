# Description

This document describes the different endpoints suported by the firmware.

# Endpoint list

- `/version` (GET): binary, see [version](##version).
- `/version_json` (GET): JSON (the only non-binary response).
- `/get_modes` (GET): binary, see [get_modes](##get_modes).
- `/get_mode` (GET): binary, see [get_mode](##get-mode).
- `/command` (POST): binary, see [commands](##commands).

## Version

`GET /version` returns just the version string, length-prefixed with the same little-endian `[ len : uint16 ] [ bytes ]` encoding used elsewhere.

`GET /version_json` returns the full firmware identity as JSON (the only
non-binary response).

## Get modes

`GET /get_modes` returns every configured mode as a single binary payload.

```txt
[ count ] [[ mode_name_len ] [ mode_name ] [ loop ] [ nb_step ] [[ mask ] [ duration ] ...] ...]
```

Example with two modes (`standard`, `blinking_orange`):

```txt
[ count ] [ name_len ] [ mode_name               ] [ loop ] [ nb_step ] [[ mask ] [ duration     ] [ mask ] [ duration     ] [ mask ] [ duration     ]]
[ 02 00 ] [ 08 00    ] [ 73 74 61 6e 64 61 72 64 ] [ 01   ] [ 03 00   ] [[ 01   ] [ 30 75 00 00  ] [ 02   ] [ b8 0b 00 00  ] [ 04   ] [ 30 75 00 00  ]]
          [ 0f 00    ] [ 62 6c 69 6e 6b ... 65   ] [ 01   ] [ 02 00   ] [[ 02   ] [ f4 01 00 00  ] [ 00   ] [ f4 01 00 00  ]]
```

## Get mode

`GET /get_mode` returns the name of the currently active mode (the one the feu is
playing), length-prefixed with the same little-endian `[ len : uint16 ] [ bytes ]`
encoding. If no mode is active, `name_len` is 0.

```txt
[ name_len ] [ name                    ]
[ 08 00     ] [ 73 74 61 6e 64 61 72 64 ]
```

## Commands

`POST /command` selects the command with a single leading `command_id` byte
(a `command_id_t` enum value), followed by the command payload:

```txt
[ command_id : uint8 ] [ payload ]
```

| command_id | command | payload |
|------------|---------|---------|
| 0 | (get_modes — served via `GET /get_modes`) | — |
| 1 | set    | `[ mode_name_len ] [ mode_name ]` |
| 2 | add    | `[ mode_name_len ] [ mode_name ] [ loop ] [ nb_step ] [[ mask ] [ duration ] ...]` |
| 3 | custom | `[ loop ] [ nb_step ] [[ mask ] [ duration ] ...]` |
| 4 | delete | `[ mode_name_len ] [ mode_name ]` |
| 5 | edit   | `[ mode_name_len ] [ mode_name ] [ loop ] [ nb_step ] [[ mask ] [ duration ] ...]` |

> Field encodings (all little-endian):
> - `mode_name_len`, `nb_step`, `count` : uint16
> - `mask`, `loop` : uint8
> - `duration` : uint32, in milliseconds
>
> `mask` bits (`main/led/led.h`): green = `0x01`, orange = `0x02`,
> red = `0x04`; `0x00` = all off. Combine with bitwise OR.
>
> Limits: `mode_name_len` ≤ `MAX_NAME_LEN` (16), `nb_step` ≤ `MAX_STEPS` (10),
> total modes ≤ `MAX_MODES` (100).
>
> The response echoes the request bytes; the command is applied asynchronously
> (queue tick ≤ 600 ms), so a 200 means *accepted*, not yet *applied* —
> re-`GET /get_modes` to observe the new state.

### set (command_id 1)

Set the active mode.

```txt
[ command_id ] [ mode_name_len ] [ mode_name               ]
[ 01         ] [ 08 00         ] [ 73 74 61 6e 64 61 72 64 ]
```

### add (command_id 2)

Add a new mode.

```txt
[ command_id ] [ mode_name_len ] [ mode_name            ] [ loop ] [ nb_step ] [[ mask ] [ duration     ]  ...]
[ 02         ] [ 07 00         ] [ 65 6e 67 6c 69 73 68 ] [ 01   ] [ 02 00   ] [[ 01   ] [ 30 75 00 00  ] [ 02   ] [ b8 0b 00 00  ]]
```

`30 75 00 00` = 30000 ms, `b8 0b 00 00` = 3000 ms.

### custom (command_id 3)

Set a one-off active sequence (not stored in the mode table).

```txt
[ command_id ] [ loop ] [ nb_step ] [[ mask ] [ duration     ] [ mask ] [ duration     ] [ mask ] [ duration     ] ...]
[ 03         ] [ 01   ] [ 03 00   ] [[ 04   ] [ 30 75 00 00  ] [ 02   ] [ b8 0b 00 00  ] [ 01   ] [ 30 75 00 00  ]]
```

### delete (command_id 4)

Remove an existing mode by name (same payload as `set`).

```txt
[ command_id ] [ mode_name_len ] [ mode_name               ]
[ 04         ] [ 08 00         ] [ 73 74 61 6e 64 61 72 64 ]
```

### edit (command_id 5)

Replace the step config of an existing mode, matched by name (same payload as
`add`).

```txt
[ command_id ] [ mode_name_len ] [ mode_name               ] [ loop ] [ nb_step ] [[ mask ] [ duration ] ...]
[ 05         ] [ 08 00         ] [ 73 74 61 6e 64 61 72 64 ] [ 01   ] [ 02 00   ] [[ 01   ] [ 30 75 00 00 ] [ 04 ] [ 30 75 00 00 ]]
```

