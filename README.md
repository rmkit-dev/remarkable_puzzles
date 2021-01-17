# Remarkable Puzzles

[Simon Tatham's Portable Puzzle Collection][stpuzzles] for the reMarkable tablet.


## Building

Builds are done in docker by default.

```sh
# Build the docker toolchain
make docker_build

# Debug build
make debug

# Release build
make release
```

## Testing

Assuming `remarkable` as an alias in ~/.ssh/config, as per
<https://remarkablewiki.com/tech/ssh>:

```
host remarkable
  Hostname 10.11.99.1
  User root
```

Stop xochitl

```sh
ssh remarkable -t systemctl stop xochitl
```

Copy config files

```sh
scp -r config/ remarkable:/opt/etc/puzzles/
```

Copy the executable and run it

```sh
scp build/debug/puzzles remarkable:/opt/bin/
ssh -t remarkable /opt/bin/rm2fb-client /opt/bin/puzzles
```

Restart xochitl

```sh
ssh remarkable -t systemctl start xochitl
```



[stpuzzles]: https://www.chiark.greenend.org.uk/~sgtatham/puzzles/
