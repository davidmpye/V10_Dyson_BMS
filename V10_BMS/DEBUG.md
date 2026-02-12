# Debugging with openocd/gdb

## Launch openocd:
```
$ openocd -f openocd_samd20.cfg
```


## Launch gdb in another terminal:
```
$ gdb-multiarch  build/samd20_firmware.elf -x gdb_samd20.cfg

```

(Your gdb-multiarch might also be called arm-none-eabi-gdb or something similar)
