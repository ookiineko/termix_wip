## Running

To load and run an ELF binary, use `tmixldr` and pass the path to the target ELF program to it:

```shell
tmixldr path/to/file
```

> *Note*
> Termix will and will only run the ELF binary with the same architecture as your host OS,
> because everything is running natively with Termix, trying to run ELF files in a different architecture
> will cause an error.

To also print out debug information, pass `-d` to `timrldr`.
