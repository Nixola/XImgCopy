# XImgCopy
Little tool to copy an URL and an image to the X clipboard at the same time.

## Usage
`cat file.png | ximgcopy URL & disown`

## To do
- [ ] Add a command line option to choose selection
- [x] Fix `TARGETS` output so that `xclip -sel c -o -t TARGETS` actually works
- [ ] (maybe) add some way to use custom data and custom targets using JSON or something
