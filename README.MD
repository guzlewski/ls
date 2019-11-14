# ls
This is my version of ls with implemented flags: -l, -R, -t, -h, --version, --help.
Program is fully working and tested on several different linux distros.


## Compilation
```bash
git clone https://github.com/guzlewski/ls
cd ls
make
```

## Usage

```
./ls.out [OPTIONS] [PATH]
Avaible options:
        -l - display long format
        -R - list subdirectories recursively
        -t - sort files by last modified date, newest first
        -h - print file size in human redable format (K, M, G, T, P)
        --version - print information about version and author
        --help - print avaible commands
Lists current directory by defalut.
```

## License
Copyright (c) guzlewski. All rights reserved.
