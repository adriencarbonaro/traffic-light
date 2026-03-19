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
[ command_name_len ] [ command_name ] [ mode_name_len ] [ mode_name               ]
[ 03 00            ] [ 73 65 74     ] [ 08 00         ] [ 73 74 61 6e 64 61 72 64 ]
```

### JSON

```json
{
    "command_name": "set",
    "data": "standard"
}
```

## add

### Packet

```txt
[ command_name_len ] [ command_name ] [ mode_name_len ] [ mode_name            ] [ nb_seq ] [[ red        ] [ orange      ] [ green       ] [ repeat_time ] ...]
[ 03 00            ] [ 61 64 64     ] [ 08 00         ] [ 65 6e 67 6c 69 73 68 ] [ 01 00  ] [ 30 75 00 00 ] [ 88 13 00 00 ] [ e8 03 00 00 ] [ ff          ]
```

### JSON

```json
{
    "command_name": "add",
    "data": {
        "name": "english",
        "leds": [
            {
                "red": 30000,
                "orange": 5000,
                "green": 1000,
                "repeat_time": -1,
            }
        ]
    }
}
```

## custom

### Packet

```txt
[ command_name_len ] [ command_name      ] [ nb_seq ] [[ red        ] [ orange      ] [ green       ] [ repeat_time ] ...]
[ 06 00            ] [ 63 75 73 74 6f 6d ] [ 01 00  ] [ 30 75 00 00 ] [ 88 13 00 00 ] [ e8 03 00 00 ] [ ff          ]
```

### JSON

```json
{
    "command_name": "custom",
    "data": [
        {
            "red": 30000,
            "orange": 5000,
            "green": 1000,
            "repeat_time": -1,
        }
    ]
}
```
