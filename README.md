# XImgCopy
Little tool to copy an image and a string (possibly an URL to the same image) to the X clipboard at the same time. It exposes two targets, `UTF8_STRING` and `image/png`.

## Usage
`ximgcopy URL < file.png`

## To do
- [ ] Add a command line option to choose selection
- [x] Fix `TARGETS` output so that `xclip -sel c -o -t TARGETS` actually works
- [ ] (maybe) add some way to use custom data and custom targets using JSON or something
- [x] Implement `INCR` for images bigger than the max property size (how was this not even in the TODO list?)
