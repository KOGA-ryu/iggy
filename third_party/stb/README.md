# stb_image

Vendored from `nothings/stb` commit
`31c1ad37456438565541f4919958214b6e762fb4` for deterministic offline PNG
and JPEG decoding.

- Upstream: https://github.com/nothings/stb
- File: `stb_image.h`
- SHA-256: `594c2fe35d49488b4382dbfaec8f98366defca819d916ac95becf3e75f4200b3`
- License: public domain or MIT, as offered in the upstream file

The engine wrapper enables only PNG and JPEG, disables stdio, and applies its
own encoded-size, dimension, and decoded-pixel limits before retaining RGBA8.
