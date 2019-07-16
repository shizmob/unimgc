# unimgc

`unimgc` is a tool to decompress HDDGuru [HDD Raw Copy Tool](http://www.hddguru.com/software/HDD-Raw-Copy-Tool/)'s proprietary `.imgc` compression format.

## Technical summary

`.imgc` images consist of a header, followed by a list of blocks. Most of the fields in the header are obvious from first inspection,
and include target disk metadata such as the product name, revision and serial number, creator software metadata and finally image metadata.
Of note is that all strings are stored as Pascal-style strings, padded to always take up `0x100` bytes including the length byte.

Followed by the header is an array of blocks, each with a header describing its type (`char[4]`) and size (`uint32_t`). There are two kind of blocks:

* `"lol!"`: the block contents are compressed using a [custom LZO algorithm](https://github.com/synopse/mORMot/blob/master/SynLZO.pas);
* `"omg!"`: the block contents is a single 64-bit integer indicating how many zeroes to add to the output;

The LZO compression algorithm mostly follows the spec as [implemented in the Linux kernel](https://www.infradead.org/~mchehab/kernel_docs/unsorted/lzo.html),
with a few derivations making the bitstreams incompatible:

* Initial byte encoding affects state differently;
* For instructions ` 0 <= x <= 15` are only defined if the current state is `0`;
* Instructions `64 <= x <= 127` use a different length addition (`1` versus `3`);
* Instructions `128 <= x <= 255` get dropped and treated as `64 <= x <= 127` instead;

## Usage

```
usage: unimgc [-ihvV] [IN] [OUT]
    -h 	 show usage
    -V 	 show version
    -v 	 increase verbosity (level 1: progress, level 2: header dumps)
    -i 	 only show image information, don't decompress
    IN 	 input file; defaults to standard input
   OUT 	 output file; defaults to standard output
```

An example of a common invocation would be `unimgc -v foo.imgc foo.img` to decompress from and to regular files, showing progress while it's working.

You can also use `unimgc` as part of a pipeline: `curl https://my.host/secret/game.imgc | unimgc -v | gzip -9c > game.img.gz`.

## License

`unimgc` is licensed under the [WTFPL](http://www.wtfpl.net/txt/copying/); see the [LICENSE](LICENSE) file for details.
The contents of [endian.h](endian.h) is licensed from Mathias Panzenböck, and released into the public domain.
