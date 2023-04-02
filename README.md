# UniqueDing Keyboard

## datycl manuform 5x7 with trackpoint and rgblight

![img](keyboard.jpg)

## make

* put code to qmk_firmware/keyboards/handwired/dactyl_manuform

* `qmk compile -kb handwired/dactyl_manuform/uniqueding_kb -km default`

## flash

* `make handwired/dactyl_manuform/uniqueding_kb:default:avrdude-split-left`
* `make handwired/dactyl_manuform/uniqueding_kb:default:avrdude-split-right`

## reference

* [https://github.com/abstracthat/dactyl-manuform](https://github.com/abstracthat/dactyl-manuform)

* [https://github.com/qmk/qmk_firmware](https://github.com/qmk/qmk_firmware)
