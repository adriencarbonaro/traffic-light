# Description

Only one POST endpoint -> command.

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

## add

### Packet

```txt
[ command_id ] [ mode_name_len ] [ mode_name            ] [ loop ] [ nb_step ] [[ mask ] [ duration    ] [ mask ] [ duration    ] [ mask ] [ duration    ] ...]
[ 01         ] [ 08 00         ] [ 65 6e 67 6c 69 73 68 ] [ 01   ] [ 03 00   ] [[ 01   ] [ 30 75 00 00 ] [ 03   ] [ 88 13 00 00 ] [ 01   ] [ 4e 20 00 00 ]]
```

## custom

### Packet

```txt
[ command_id ] [ loop ] [ nb_step ] [[ mask ] [ duration    ] [ mask ] [ duration    ] [ mask ] [ duration    ] ...]
[ 02         ] [ 01   ] [ 03 00   ] [[ 04   ] [ 30 75 00 00 ] [ 02   ] [ 88 13 00 00 ] [ 01   ] [ e8 03 00 00 ]]
```
