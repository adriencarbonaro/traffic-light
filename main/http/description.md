# Description

One POST endpoint -> command (binary in, binary out).

GET endpoints:
- `/version` : binary, see [version](#version).
- `/version_json` : JSON (the only non-binary response).
- `/get_modes` : binary, see [get_modes](#get_modes).

Commands : [ "set", "add", "custom" ]

# Packet

```txt
[ command_name_len ] [ command_name ] [ payload ]
```

# Modes

## set

### Packet

```txt
[ command_id ] [ mode_name_len ] [ mode_name               ]
[ 00         ] [ 08 00         ] [ 73 74 61 6e 64 61 72 64 ]
```

> Field encodings (all little-endian):
> - `mode_name_len`, `nb_step` : uint16
> - `mask` : uint8
> - `duration` : uint32, in milliseconds

## add

### Packet

```txt
[ command_id ] [ mode_name_len ] [ mode_name            ] [ loop ] [ nb_step ] [[ mask ] [ duration     ] [ mask ] [ duration     ] [ mask ] [ duration     ] ...]
[ 01         ] [ 07 00         ] [ 65 6e 67 6c 69 73 68 ] [ 01   ] [ 03 00   ] [[ 01   ] [ 30 75 00 00  ] [ 02   ] [ b8 0b 00 00  ] [ 04   ] [ 30 75 00 00  ]]
```

`30 75 00 00` = 30000 ms, `b8 0b 00 00` = 3000 ms.

## custom

### Packet

```txt
[ command_id ] [ loop ] [ nb_step ] [[ mask ] [ duration     ] [ mask ] [ duration     ] [ mask ] [ duration     ] ...]
[ 02         ] [ 01   ] [ 03 00   ] [[ 04   ] [ 30 75 00 00  ] [ 02   ] [ b8 0b 00 00  ] [ 01   ] [ 30 75 00 00  ]]
```

# version

`GET /version` returns just the version string (`DESCRIBE`), length-prefixed
with the same little-endian `[ len : uint16 ] [ bytes ]` encoding used
elsewhere.

`GET /version_json` returns the full firmware identity as JSON (the only
non-binary response).

### Response

```txt
[ version_len ] [ version ]
```

# get_modes

`GET /get_modes` returns every configured mode as a single binary payload. Each
mode is encoded like an `add` packet body (no `command_id`), prefixed by a
uint16 mode count. Same little-endian field encodings as above.

### Response

```txt
[ count ] [[ mode_name_len ] [ mode_name ] [ loop ] [ nb_step ] [[ mask ] [ duration ] ...] ] ...]
```

Example with two modes (`standard`, `blinking_orange`):

```txt
[ count ] [ name_len ] [ mode_name               ] [ loop ] [ nb_step ] [[ mask ] [ duration     ] [ mask ] [ duration     ] [ mask ] [ duration     ]]
[ 02 00 ] [ 08 00    ] [ 73 74 61 6e 64 61 72 64 ] [ 01   ] [ 03 00   ] [[ 01   ] [ 30 75 00 00  ] [ 02   ] [ b8 0b 00 00  ] [ 04   ] [ 30 75 00 00  ]]
          [ 0f 00    ] [ 62 6c 69 6e 6b ... 65   ] [ 01   ] [ 02 00   ] [[ 02   ] [ f4 01 00 00  ] [ 00   ] [ f4 01 00 00  ]]
```
